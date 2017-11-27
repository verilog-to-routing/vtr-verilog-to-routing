#include <cstdio>
using namespace std;

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "read_xml_arch_file.h"
#include "rr_types.h"

#define UN_SET -1

/************************** Subroutines local to this module ****************/

static void get_switch_type(
        bool is_from_sb, bool is_to_sb,
        short from_node_switch, short to_node_switch, short switch_types[2]);

static void load_chan_rr_indices(
        const int max_chan_width, const int chan_len,
        const int num_chans, const t_rr_type type,
        const t_chan_details& chan_details,
        t_rr_node_indices& indices, int *index);

static void load_block_rr_indices(
        const DeviceGrid& grid,
        t_rr_node_indices& indices,
        int* index);

static int get_bidir_track_to_chan_seg(
        const std::vector<int> conn_tracks,
        const t_rr_node_indices& L_rr_node_indices, const int to_chan, const int to_seg,
        const int to_sb, const t_rr_type to_type, const t_seg_details * seg_details,
        const bool from_is_sblock, const int from_switch,
        bool * L_rr_edge_done,
        const enum e_directionality directionality,
        t_linked_edge **edge_list);

static int get_unidir_track_to_chan_seg(
        const int from_track, const int to_chan, const int to_seg, const int to_sb,
        const t_rr_type to_type, const int max_chan_width,
        const DeviceGrid& grid, const enum e_side from_side, const enum e_side to_side,
        const int Fs_per_side,
        short ******sblock_pattern, const t_rr_node_indices& L_rr_node_indices,
        const t_seg_details * seg_details, bool * L_rr_edge_done,
        bool * Fs_clipped, t_linked_edge **edge_list);

static int get_track_to_chan_seg(
        const int from_track, const int to_chan, const int to_seg,
        const t_rr_type to_chan_type,
        const e_side from_side, const e_side to_side,
        const t_rr_node_indices&L_rr_node_indices,
        t_sb_connection_map *sb_conn_map,
        bool * L_rr_edge_done,
        t_linked_edge **edge_list);

static int vpr_to_phy_track(
        const int itrack, const int chan_num, const int seg_num,
        const t_seg_details * seg_details,
        const enum e_directionality directionality);

static int *label_wire_muxes(
        const int chan_num, const int seg_num,
        const t_seg_details * seg_details, const int seg_type_index, const int max_len,
        const enum e_direction dir, const int max_chan_width,
        const bool check_cb, int *num_wire_muxes, int *num_wire_muxes_cb_restricted);

static int *label_incoming_wires(
        const int chan_num, const int seg_num,
        const int sb_seg, const t_seg_details * seg_details, const int max_len,
        const enum e_direction dir, const int max_chan_width,
        int *num_incoming_wires, int *num_ending_wires);

static int find_label_of_track(
        int *wire_mux_on_track, int num_wire_muxes,
        int from_track);

void dump_seg_details(
        t_seg_details * seg_details,
        int max_chan_width,
        const char *fname);

//Returns true if the switch block at the specified location is 'inside' a block
//  grid: The device grid
//  x_coord: The grid tile x location
//  y_coord: The grid tile y location
//  chan_type: The channel type
bool is_block_internal_switchblock(const DeviceGrid& grid, int x_coord, int y_coord);

//Returns true if the switch block at the specified location should be created
//  grid: The device grid
//  from_chan_coord: The horizontal or vertical channel index (i.e. x-coord for CHANY, y-coord for CHANX)
//  from_seg_coord: The horizontal or vertical location along the channel (i.e. y-coord for CHANY, x-coord for CHANX)
//  from_chan_type: The from channel type
//  to_chan_type: The to channel type
bool should_create_switchblock(const DeviceGrid& grid, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type);

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
int *get_seg_track_counts(
        const int num_sets, const int num_seg_types,
        const t_segment_inf * segment_inf,
        const bool use_full_seg_groups) {

    int *result;
    double *demand;
    int i, imax, freq_sum, assigned, size;
    double scale, max, reduce;

    result = (int *) vtr::malloc(sizeof (int) * num_seg_types);
    demand = (double *) vtr::malloc(sizeof (double) * num_seg_types);

    /* Scale factor so we can divide by any length
     * and still use integers */
    scale = 1;
    freq_sum = 0;
    for (i = 0; i < num_seg_types; ++i) {
        scale *= segment_inf[i].length;
        freq_sum += segment_inf[i].frequency;
    }
    reduce = scale * freq_sum;

    /* Init assignments to 0 and set the demand values */
    for (i = 0; i < num_seg_types; ++i) {
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
        for (i = 0; i < num_seg_types; ++i) {
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

    /* Free temps */
    if (demand) {
        vtr::free(demand);
        demand = NULL;
    }

    /* This must be freed by caller */
    return result;
}

t_seg_details *alloc_and_load_seg_details(
        int *max_chan_width, const int max_len,
        const int num_seg_types, const t_segment_inf * segment_inf,
        const bool use_full_seg_groups, const bool is_global_graph,
        const enum e_directionality directionality,
        int * num_seg_details) {

    /* Allocates and loads the seg_details data structure.  Max_len gives the   *
     * maximum length of a segment (dimension of array).  The code below tries  *
     * to:                                                                      *
     * (1) stagger the start points of segments of the same type evenly;        *
     * (2) spread out the limited number of connection boxes or switch boxes    *
     *     evenly along the length of a segment, starting at the segment ends;  *
     * (3) stagger the connection and switch boxes on different long lines,     *
     *     as they will not be staggered by different segment start points.     */

    int i, cur_track, ntracks, itrack, length, j, index;
    int arch_wire_switch, arch_opin_switch, fac, num_sets, tmp;
    int group_start, first_track;
    int *sets_per_seg_type = NULL;
    t_seg_details *seg_details = NULL;
    bool longline;

    /* Unidir tracks are assigned in pairs, and bidir tracks individually */
    if (directionality == BI_DIRECTIONAL) {
        fac = 1;
    } else {
        VTR_ASSERT(directionality == UNI_DIRECTIONAL);
        fac = 2;
    }

    if (*max_chan_width % fac != 0) {
        VPR_THROW(VPR_ERROR_ROUTE, "Routing channel width must be divisible by %d (channel width was %d)", fac, *max_chan_width);
    }

    /* Map segment type fractions and groupings to counts of tracks */
    sets_per_seg_type = get_seg_track_counts((*max_chan_width / fac),
            num_seg_types, segment_inf, use_full_seg_groups);

    /* Count the number tracks actually assigned. */
    tmp = 0;
    for (i = 0; i < num_seg_types; ++i) {
        tmp += sets_per_seg_type[i] * fac;
    }
    VTR_ASSERT(use_full_seg_groups || (tmp == *max_chan_width));
    *max_chan_width = tmp;

    seg_details = (t_seg_details *) vtr::malloc(
            *max_chan_width * sizeof (t_seg_details));

    /* Setup the seg_details data */
    cur_track = 0;
    for (i = 0; i < num_seg_types; ++i) {
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
            seg_details[cur_track].type_name_ptr = segment_inf[i].name;

            /* Remember the start track of the current wire group */
            if ((itrack / fac) % length == 0 && (itrack % fac) == 0) {
                group_start = cur_track;
            }

            seg_details[cur_track].length = length;
            seg_details[cur_track].longline = longline;

            /* Stagger the start points in for each track set. The 
             * pin mappings should be aware of this when chosing an
             * intelligent way of connecting pins and tracks.
             * cur_track is used as an offset so that extra tracks
             * from different segment types are hopefully better
             * balanced. */
            seg_details[cur_track].start = (cur_track / fac) % length + 1;

            /* These properties are used for vpr_to_phy_track to determine
             * * twisting of wires. */
            seg_details[cur_track].group_start = group_start;
            seg_details[cur_track].group_size =
                    min(ntracks + first_track - group_start, length * fac);
            VTR_ASSERT(0 == seg_details[cur_track].group_size % fac);
            if (0 == seg_details[cur_track].group_size) {
                seg_details[cur_track].group_size = length * fac;
            }

            seg_details[cur_track].seg_start = -1;
            seg_details[cur_track].seg_end = -1;

            /* Setup the cb and sb patterns. Global route graphs can't depopulate cb and sb
             * since this is a property of a detailed route. */
            seg_details[cur_track].cb = (bool *) vtr::malloc(length * sizeof (bool));
            seg_details[cur_track].sb = (bool *) vtr::malloc((length + 1) * sizeof (bool));
            for (j = 0; j < length; ++j) {
                if (is_global_graph) {
                    seg_details[cur_track].cb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    index = j % segment_inf[i].cb_len;
                    seg_details[cur_track].cb[j] = segment_inf[i].cb[index];
                }
            }
            for (j = 0; j < (length + 1); ++j) {
                if (is_global_graph) {
                    seg_details[cur_track].sb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    index = j % segment_inf[i].sb_len;
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
                seg_details[cur_track].direction =
                        (itrack % 2) ? DEC_DIRECTION : INC_DIRECTION;
            }

            seg_details[cur_track].index = i;

            ++cur_track;
        }
    } /* End for each segment type. */

    /* free variables */
    vtr::free(sets_per_seg_type);

    if (num_seg_details) {
        *num_seg_details = cur_track;
    }
    return seg_details;
}

/* Allocates and loads the chan_details data structure, a 2D array of
   seg_details structures. This array is used to handle unique seg_details
   (ie. channel segments) for each horizontal and vertical channel. */

void alloc_and_load_chan_details(
        const DeviceGrid& grid,
        const t_chan_width* nodes_per_chan,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        const int num_seg_details,
        const t_seg_details* seg_details,
        t_chan_details& chan_details_x,
        t_chan_details& chan_details_y) {

    chan_details_x = init_chan_details(grid, nodes_per_chan,
            num_seg_details, seg_details, SEG_DETAILS_X);
    chan_details_y = init_chan_details(grid, nodes_per_chan,
            num_seg_details, seg_details, SEG_DETAILS_Y);

    /* Obstruct channel segment details based on grid block widths/heights */
    obstruct_chan_details(grid, nodes_per_chan,
            trim_empty_channels, trim_obs_channels,
            chan_details_x, chan_details_y);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(grid, nodes_per_chan,
            chan_details_x, chan_details_y);
}

t_chan_details init_chan_details(
        const DeviceGrid& grid,
        const t_chan_width* nodes_per_chan,
        const int num_seg_details,
        const t_seg_details* seg_details,
        const enum e_seg_details_type seg_details_type) {

    t_chan_details chan_details = vtr::Matrix<t_seg_details*>({grid.width(), grid.height()});

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {

            t_seg_details* p_seg_details = 0;
            p_seg_details = (t_seg_details*) vtr::calloc(nodes_per_chan->max, sizeof (t_seg_details));
            for (int i = 0; i < num_seg_details; ++i) {

                p_seg_details[i].length = seg_details[i].length;
                p_seg_details[i].longline = seg_details[i].longline;

                p_seg_details[i].start = seg_details[i].start;

                p_seg_details[i].group_start = seg_details[i].group_start;
                p_seg_details[i].group_size = seg_details[i].group_size;

                p_seg_details[i].seg_start = -1;
                p_seg_details[i].seg_end = -1;

                if (seg_details_type == SEG_DETAILS_X) {
                    p_seg_details[i].seg_start = get_seg_start(p_seg_details, i, y, x);
                    p_seg_details[i].seg_end = get_seg_end(p_seg_details, i, p_seg_details[i].seg_start, y, grid.width() - 2); //-2 for no perim channels
                }
                if (seg_details_type == SEG_DETAILS_Y) {
                    p_seg_details[i].seg_start = get_seg_start(p_seg_details, i, x, y);
                    p_seg_details[i].seg_end = get_seg_end(p_seg_details, i, p_seg_details[i].seg_start, x, grid.height() - 2); //-2 for no perim channels
                }

                int length = seg_details[i].length;
                p_seg_details[i].cb = (bool*)vtr::malloc(length * sizeof (bool));
                p_seg_details[i].sb = (bool*)vtr::malloc((length + 1) * sizeof (bool));
                for (int j = 0; j < length; ++j) {
                    p_seg_details[i].cb[j] = seg_details[i].cb[j];
                }
                for (int j = 0; j < (length + 1); ++j) {
                    p_seg_details[i].sb[j] = seg_details[i].sb[j];
                }

                p_seg_details[i].Rmetal = seg_details[i].Rmetal;
                p_seg_details[i].Cmetal = seg_details[i].Cmetal;
                p_seg_details[i].Cmetal_per_m = seg_details[i].Cmetal_per_m;

                p_seg_details[i].arch_wire_switch = seg_details[i].arch_wire_switch;
                p_seg_details[i].arch_opin_switch = seg_details[i].arch_opin_switch;

                p_seg_details[i].direction = seg_details[i].direction;

                p_seg_details[i].index = seg_details[i].index;
                p_seg_details[i].type_name_ptr = seg_details[i].type_name_ptr;

                if (seg_details_type == SEG_DETAILS_X) {
                    if (i >= nodes_per_chan->x_list[y]) {
                        p_seg_details[i].length = 0;
                    }
                }
                if (seg_details_type == SEG_DETAILS_Y) {
                    if (i >= nodes_per_chan->y_list[x]) {
                        p_seg_details[i].length = 0;
                    }
                }
            }
            chan_details[x][y] = p_seg_details;
        }
    }
    return chan_details;
}

void obstruct_chan_details(
        const DeviceGrid& grid,
        const t_chan_width* nodes_per_chan,
        const bool trim_empty_channels,
        const bool trim_obs_channels,
        t_chan_details& chan_details_x,
        t_chan_details& chan_details_y) {

    auto& device_ctx = g_vpr_ctx.device();

    /* Iterate grid to find and obstruct based on multi-width/height blocks */
    for (size_t x = 0; x < grid.width() - 1; ++x) {
        for (size_t y = 0; y < grid.height() - 1; ++y) {

            if (!trim_obs_channels)
                continue;

            if (grid[x][y].type == device_ctx.EMPTY_TYPE)
                continue;
            if (grid[x][y].width_offset > 0 || grid[x][y].height_offset > 0)
                continue;
            if (grid[x][y].type->width == 1 && grid[x][y].type->height == 1)
                continue;

            if (grid[x][y].type->height > 1) {
                for (int dx = 0; dx <= grid[x][y].type->width - 1; ++dx) {
                    for (int dy = 0; dy < grid[x][y].type->height - 1; ++dy) {
                        for (int track = 0; track < nodes_per_chan->max; ++track) {
                            chan_details_x[x + dx][y + dy][track].length = 0;
                        }
                    }
                }
            }
            if (grid[x][y].type->width > 1) {
                for (int dy = 0; dy <= grid[x][y].type->height - 1; ++dy) {
                    for (int dx = 0; dx < grid[x][y].type->width - 1; ++dx) {
                        for (int track = 0; track < nodes_per_chan->max; ++track) {
                            chan_details_y[x + dx][y + dy][track].length = 0;
                        }
                    }
                }
            }
        }
    }

    /* Iterate grid again to find and obstruct based on neighboring EMPTY and/or IO types */
    for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            if (!trim_empty_channels)
                continue;

            if (grid[x][y].type == device_ctx.IO_TYPE) {
                if ((x == 0) || (y == 0))
                    continue;
            }
            if (grid[x][y].type == device_ctx.EMPTY_TYPE) {
                if ((x == grid.width() - 2) && (grid[x + 1][y].type == device_ctx.IO_TYPE)) //-2 for no perim channels
                    continue;
                if ((y == grid.height() - 2) && (grid[x][y + 1].type == device_ctx.IO_TYPE)) //-2 for no perim channels
                    continue;
            }

            if ((grid[x][y].type == device_ctx.IO_TYPE) || (grid[x][y].type == device_ctx.EMPTY_TYPE)) {
                if ((grid[x][y + 1].type == device_ctx.IO_TYPE) || (grid[x][y + 1].type == device_ctx.EMPTY_TYPE)) {
                    for (int track = 0; track < nodes_per_chan->max; ++track) {
                        chan_details_x[x][y][track].length = 0;
                    }
                }
            }
            if ((grid[x][y].type == device_ctx.IO_TYPE) || (grid[x][y].type == device_ctx.EMPTY_TYPE)) {
                if ((grid[x + 1][y].type == device_ctx.IO_TYPE) || (grid[x + 1][y].type == device_ctx.EMPTY_TYPE)) {
                    for (int track = 0; track < nodes_per_chan->max; ++track) {
                        chan_details_y[x][y][track].length = 0;
                    }
                }
            }
        }
    }
}

void adjust_chan_details(
        const DeviceGrid& grid,
        const t_chan_width* nodes_per_chan,
        t_chan_details& chan_details_x,
        t_chan_details& chan_details_y) {

    for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels
        for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_x[x][y][0].length > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                    chan_details_x, SEG_DETAILS_X);
        }
    }

    for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_y[x][y][0].length > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                    chan_details_y, SEG_DETAILS_Y);
        }
    }
}

void adjust_seg_details(
        const int x, const int y,
        const DeviceGrid& grid,
        const t_chan_width* nodes_per_chan,
        t_chan_details& chan_details,
        const enum e_seg_details_type seg_details_type) {

    int seg_index = (seg_details_type == SEG_DETAILS_X ? x : y);

    for (int track = 0; track < nodes_per_chan->max; ++track) {

        int lx = (seg_details_type == SEG_DETAILS_X ? x - 1 : x);
        int ly = (seg_details_type == SEG_DETAILS_X ? y : y - 1);
        if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length == 0)
            continue;

        while (chan_details[lx][ly][track].seg_end >= seg_index) {
            chan_details[lx][ly][track].seg_end = seg_index - 1;
            lx = (seg_details_type == SEG_DETAILS_X ? lx - 1 : lx);
            ly = (seg_details_type == SEG_DETAILS_X ? ly : ly - 1);
            if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length == 0)
                break;
        }
    }

    for (int track = 0; track < nodes_per_chan->max; ++track) {

        size_t lx = (seg_details_type == SEG_DETAILS_X ? x + 1 : x);
        size_t ly = (seg_details_type == SEG_DETAILS_X ? y : y + 1);
        if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length == 0) //-2 for no perim channels
            continue;

        while (chan_details[lx][ly][track].seg_start <= seg_index) {
            chan_details[lx][ly][track].seg_start = seg_index + 1;
            lx = (seg_details_type == SEG_DETAILS_X ? lx + 1 : lx);
            ly = (seg_details_type == SEG_DETAILS_X ? ly : ly + 1);
            if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length == 0) //-2 for no perim channels
                break;
        }
    }
}

void free_seg_details(
        t_seg_details * seg_details,
        const int max_chan_width) {

    /* Frees all the memory allocated to an array of seg_details structures. */
    for (int i = 0; i < max_chan_width; ++i) {
        vtr::free(seg_details[i].cb);
        vtr::free(seg_details[i].sb);
    }
    vtr::free(seg_details);
}

void free_chan_details(
        t_chan_details& chan_details_x,
        t_chan_details& chan_details_y,
        const int max_chan_width,
        const DeviceGrid& grid) {

    /* Frees all the memory allocated to an array of chan_details structures. */
    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {

            t_seg_details* p_seg_details = chan_details_x[x][y];
            free_seg_details(p_seg_details, max_chan_width);
        }
    }
    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {

            t_seg_details* p_seg_details = chan_details_y[x][y];
            free_seg_details(p_seg_details, max_chan_width);
        }
    }

    chan_details_x.clear();
    chan_details_y.clear();
}

/* Returns the segment number at which the segment this track lies on        *
 * started.                                                                  */
int get_seg_start(
        const t_seg_details * seg_details, const int itrack,
        const int chan_num, const int seg_num) {

    int seg_start = 0;
    if (seg_details[itrack].seg_start >= 0) {

        seg_start = seg_details[itrack].seg_start;

    } else {

        seg_start = 1;
        if (false == seg_details[itrack].longline) {

            int length = seg_details[itrack].length;
            int start = seg_details[itrack].start;

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

int get_seg_end(const t_seg_details * seg_details, const int itrack, const int istart,
        const int chan_num, const int seg_max) {

    int seg_end = 0;
    if (seg_details[itrack].seg_end >= 0) {

        seg_end = seg_details[itrack].seg_end;

    } else {

        int len = seg_details[itrack].length;
        int ofs = seg_details[itrack].start;

        /* Normal endpoint */
        seg_end = istart + len - 1;

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
    }
    return seg_end;
}

/* Returns the number of tracks to which clb opin #ipin at (i,j) connects.   *
 * Also stores the nodes to which this pin connects in the linked list       *
 * pointed to by *edge_list_ptr.                                             */
int get_bidir_opin_connections(
        const int i, const int j, const int ipin,
        t_linked_edge **edge_list,
        const t_pin_to_track_lookup& opin_to_track_map,
        const int Fc, bool * L_rr_edge_done,
        const t_rr_node_indices& L_rr_node_indices, const t_seg_details * seg_details) {

    int iside, num_conn, tr_i, tr_j, chan, seg;
    int to_track, to_switch, to_node, iconn;
    int is_connected_track;
    t_type_ptr type;
    t_rr_type to_type;

    auto& device_ctx = g_vpr_ctx.device();

    type = device_ctx.grid[i][j].type;
    int width_offset = device_ctx.grid[i][j].width_offset;
    int height_offset = device_ctx.grid[i][j].height_offset;

    num_conn = 0;

    /* [0..device_ctx.num_block_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
    for (iside = 0; iside < 4; iside++) {

        /* Figure out coords of channel segment based on side */
        tr_i = ((iside == LEFT) ? (i - 1) : i);
        tr_j = ((iside == BOTTOM) ? (j - 1) : j);

        to_type = ((iside == LEFT) || (iside == RIGHT)) ? CHANY : CHANX;

        chan = ((to_type == CHANX) ? tr_j : tr_i);
        seg = ((to_type == CHANX) ? tr_i : tr_j);

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

        is_connected_track = false;

        /* Itterate of the opin to track connections */
        for (iconn = 0; iconn < Fc; ++iconn) {
            to_track = opin_to_track_map[type->index][ipin][width_offset][height_offset][iside][iconn];

            /* Skip unconnected connections */
            if (OPEN == to_track || is_connected_track) {
                is_connected_track = true;
                VTR_ASSERT(OPEN == opin_to_track_map[type->index][ipin][width_offset][height_offset][iside][0]);
                continue;
            }

            /* Only connect to wire if there is a CB */
            if (is_cblock(chan, seg, to_track, seg_details)) {
                to_switch = seg_details[to_track].arch_wire_switch;
                to_node = get_rr_node_index(L_rr_node_indices, tr_i, tr_j, to_type, to_track);

                *edge_list = insert_in_edge_list(*edge_list, to_node, to_switch);
                L_rr_edge_done[to_node] = true;
                ++num_conn;
            }
        }
    }

    return num_conn;
}

int get_unidir_opin_connections(
        const int chan, const int seg, int Fc, const int seg_type_index,
        const t_rr_type chan_type, const t_seg_details * seg_details,
        t_linked_edge ** edge_list_ptr,
        vtr::NdMatrix<int, 3>& Fc_ofs,
        bool * L_rr_edge_done, const int max_len,
        const int max_chan_width, const t_rr_node_indices& L_rr_node_indices,
        bool * Fc_clipped) {

    /* Gets a linked list of Fc nodes of specified seg_type_index to connect 
     * to in given chan seg. Fc_ofs is used for the opin staggering pattern. */

    int *inc_muxes = NULL;
    int *dec_muxes = NULL;
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
    inc_muxes = label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
            INC_DIRECTION, max_chan_width, true, &num_inc_muxes, &dummy);
    dec_muxes = label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
            DEC_DIRECTION, max_chan_width, true, &num_dec_muxes, &dummy);


    /* Clip Fc to the number of muxes. */
    if (((Fc / 2) > num_inc_muxes) || ((Fc / 2) > num_dec_muxes)) {
        *Fc_clipped = true;
        Fc = 2 * min(num_inc_muxes, num_dec_muxes);
    }

    /* Assign tracks to meet Fc demand */
    num_edges = 0;
    for (iconn = 0; iconn < (Fc / 2); ++iconn) {
        /* Figure of the next mux to use for the 'inc' and 'dec' connections */
        inc_mux = Fc_ofs[chan][seg][seg_type_index] % num_inc_muxes;
        dec_mux = Fc_ofs[chan][seg][seg_type_index] % num_dec_muxes;
        ++Fc_ofs[chan][seg][seg_type_index];

        /* Figure out the track it corresponds to. */
        VTR_ASSERT(inc_muxes != NULL);
        inc_track = inc_muxes[inc_mux];

        VTR_ASSERT(dec_muxes != NULL);
        dec_track = dec_muxes[dec_mux];

        /* Figure the inodes of those muxes */
        inc_inode_index = get_rr_node_index(L_rr_node_indices, x, y, chan_type, inc_track);
        dec_inode_index = get_rr_node_index(L_rr_node_indices, x, y, chan_type, dec_track);
        /* Add to the list. */
        if (false == L_rr_edge_done[inc_inode_index]) {
            L_rr_edge_done[inc_inode_index] = true;
            *edge_list_ptr = insert_in_edge_list(*edge_list_ptr, inc_inode_index,
                    seg_details[inc_track].arch_opin_switch);
            ++num_edges;
        }
        if (false == L_rr_edge_done[dec_inode_index]) {
            L_rr_edge_done[dec_inode_index] = true;
            *edge_list_ptr = insert_in_edge_list(*edge_list_ptr, dec_inode_index,
                    seg_details[dec_track].arch_opin_switch);
            ++num_edges;
        }
    }

    if (inc_muxes) {
        vtr::free(inc_muxes);
        inc_muxes = NULL;
    }
    if (dec_muxes) {
        vtr::free(dec_muxes);
        dec_muxes = NULL;
    }

    return num_edges;
}

bool is_cblock(const int chan, const int seg, const int track,
        const t_seg_details * seg_details) {

    int length, ofs, start_seg;

    length = seg_details[track].length;

    /* Make sure they gave us correct start */
    start_seg = get_seg_start(seg_details, track, chan, seg);

    ofs = seg - start_seg;

    VTR_ASSERT(ofs >= 0);
    VTR_ASSERT(ofs < length);

    /* If unidir segment that is going backwards, we need to flip the ofs */
    if (DEC_DIRECTION == seg_details[track].direction) {
        ofs = (length - 1) - ofs;
    }

    return seg_details[track].cb[ofs];
}

/* Dumps out an array of seg_details structures to file fname.  Used only   *
 * for debugging.                                                           */
void dump_seg_details(
        const t_seg_details* seg_details,
        int max_chan_width,
        const char *fname) {

    FILE *fp = vtr::fopen(fname, "w");
    if (fp) {
        dump_seg_details(seg_details, max_chan_width, fp);
    }
    fclose(fp);
}

void dump_seg_details(
        const t_seg_details* seg_details,
        int max_chan_width,
        FILE* fp) {

    const char *direction_names[] = {"inc_direction", "dec_direction", "bi_direction"};

    for (int i = 0; i < max_chan_width; i++) {

        fprintf(fp, "track: %d\n", i);
        fprintf(fp, "length: %d  start: %d",
                seg_details[i].length, seg_details[i].start);

        if (seg_details[i].length > 0) {
            if (seg_details[i].seg_start >= 0 && seg_details[i].seg_end >= 0) {
                fprintf(fp, " [%d,%d]",
                        seg_details[i].seg_start, seg_details[i].seg_end);
            }
            fprintf(fp, "  longline: %d  arch_wire_switch: %d  arch_opin_switch: %d",
                    seg_details[i].longline,
                    seg_details[i].arch_wire_switch, seg_details[i].arch_opin_switch);
        }
        fprintf(fp, "\n");

        fprintf(fp, "Rmetal: %g  Cmetal: %g\n",
                seg_details[i].Rmetal, seg_details[i].Cmetal);

        fprintf(fp, "direction: %s\n",
                direction_names[seg_details[i].direction]);

        fprintf(fp, "cb list:  ");
        for (int j = 0; j < seg_details[i].length; j++)
            fprintf(fp, "%d ", seg_details[i].cb[j]);
        fprintf(fp, "\n");

        fprintf(fp, "sb list: ");
        for (int j = 0; j <= seg_details[i].length; j++)
            fprintf(fp, "%d ", seg_details[i].sb[j]);
        fprintf(fp, "\n");

        fprintf(fp, "\n");
    }
}

/* Dumps out an array of seg_details structures to file fname.  Used only   *
 * for debugging.                                                           */
void dump_seg_details(
        t_seg_details * seg_details,
        int max_chan_width,
        const char *fname) {

    FILE *fp = vtr::fopen(fname, "w");
    dump_seg_details(seg_details, max_chan_width, fp);
    fclose(fp);
}

/* Dumps out a 2D array of chan_details structures to file fname.  Used     *
 * only for debugging.                                                      */
void dump_chan_details(
        const t_chan_details& chan_details_x,
        const t_chan_details& chan_details_y,
        int max_chan_width,
        const DeviceGrid& grid,
        const char *fname) {

    FILE *fp = vtr::fopen(fname, "w");
    if (fp) {
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels
            for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_x: [%zu][%zu]\n", x, y);
                fprintf(fp, "========================\n");

                const t_seg_details* seg_details = chan_details_x[x][y];
                dump_seg_details(seg_details, max_chan_width, fp);
            }
        }
        for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels
            for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

                fprintf(fp, "========================\n");
                fprintf(fp, "chan_details_y: [%zu][%zu]\n", x, y);
                fprintf(fp, "========================\n");

                const t_seg_details* seg_details = chan_details_y[x][y];
                dump_seg_details(seg_details, max_chan_width, fp);
            }
        }
    }
    fclose(fp);
}

/* Dumps out a 2D array of switch block pattern structures to file fname. *
 * Used for debugging purposes only.                                      */
void dump_sblock_pattern(
        short ******sblock_pattern,
        int max_chan_width,
        const DeviceGrid& grid,
        const char *fname) {

    FILE *fp = vtr::fopen(fname, "w");
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

                        const char * psz_from_side = "?";
                        switch (from_side) {
                            case 0: psz_from_side = "T";
                                break;
                            case 1: psz_from_side = "R";
                                break;
                            case 2: psz_from_side = "B";
                                break;
                            case 3: psz_from_side = "L";
                                break;
                            default: VTR_ASSERT_MSG(false, "Unrecognized from side");
                                break;
                        }
                        const char * psz_to_side = "?";
                        switch (to_side) {
                            case 0: psz_to_side = "T";
                                break;
                            case 1: psz_to_side = "R";
                                break;
                            case 2: psz_to_side = "B";
                                break;
                            case 3: psz_to_side = "L";
                                break;
                            default: VTR_ASSERT_MSG(false, "Unrecognized to side");
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

static void load_chan_rr_indices(
        const int max_chan_width, const int chan_len,
        const int num_chans, const t_rr_type type,
        const t_chan_details& chan_details,
        t_rr_node_indices& indices, int *index) {

    VTR_ASSERT(indices[type].size() == size_t(num_chans));
    for (int chan = 0; chan < num_chans - 1; ++chan) {

        VTR_ASSERT(indices[type][chan].size() == size_t(chan_len));

        for (int seg = 1; seg < chan_len - 1; ++seg) {
            VTR_ASSERT(indices[type][chan][seg].size() == NUM_SIDES);

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
            t_seg_details * seg_details = chan_details[x][y];

            for (unsigned track = 0; track < indices[type][chan][seg][0].size(); ++track) {

                if (seg_details[track].length <= 0)
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

static void load_block_rr_indices(
        const DeviceGrid& grid,
        t_rr_node_indices& indices,
        int* index) {

    //Walk through the grid assigning indices to SOURCE/SINK IPIN/OPIN
    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            if (grid[x][y].width_offset == 0 && grid[x][y].height_offset == 0) {
                //Process each block from it's root location
                t_type_ptr type = grid[x][y].type;

                //Assign indicies for SINKs and SOURCEs
                // Note that SINKS/SOURCES have no side, so we always use side 0
                for (int iclass = 0; iclass < type->num_class; ++iclass) {
                    auto class_type = type->class_inf[iclass].type;
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
                VTR_ASSERT(indices[SOURCE][x][y][0].size() == size_t(type->num_class));
                VTR_ASSERT(indices[SINK][x][y][0].size() == size_t(type->num_class));

                //Assign indicies for IPINs and OPINs at all offsets from root
                for (int ipin = 0; ipin < type->num_pins; ++ipin) {
                    for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                        int x_tile = x + width_offset;
                        for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                            int y_tile = y + height_offset;
                            for (e_side side : SIDES) {
                                if (type->pinloc[width_offset][height_offset][side][ipin]) {

                                    int iclass = type->pin_class[ipin];
                                    auto class_type = type->class_inf[iclass].type;

                                    if (class_type == DRIVER) {
                                        indices[OPIN][x_tile][y_tile][side].push_back(*index);
                                        indices[IPIN][x_tile][y_tile][side].push_back(OPEN);
                                    } else {
                                        VTR_ASSERT(class_type == RECEIVER);
                                        indices[IPIN][x_tile][y_tile][side].push_back(*index);
                                        indices[OPIN][x_tile][y_tile][side].push_back(OPEN);
                                    }
                                    ++(*index);
                                } else {
                                    indices[IPIN][x_tile][y_tile][side].push_back(OPEN);
                                    indices[OPIN][x_tile][y_tile][side].push_back(OPEN);
                                }
                            }
                        }
                    }
                }


                //Sanity check
                for (int width_offset = 0; width_offset < type->width; ++width_offset) {
                    int x_tile = x + width_offset;
                    for (int height_offset = 0; height_offset < type->height; ++height_offset) {
                        int y_tile = y + height_offset;
                        for (e_side side : SIDES) {
                            VTR_ASSERT(indices[IPIN][x_tile][y_tile][side].size() == size_t(type->num_pins));
                            VTR_ASSERT(indices[OPIN][x_tile][y_tile][side].size() == size_t(type->num_pins));
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

                indices[SOURCE][x][y] = indices[SOURCE][root_x][root_y];
                indices[SINK][x][y] = indices[SINK][root_x][root_y];
            }
        }
    }

}

t_rr_node_indices alloc_and_load_rr_node_indices(
        const int max_chan_width, const DeviceGrid& grid, int *index,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y) {

    /* Allocates and loads all the structures needed for fast lookups of the   *
     * index of an rr_node.  rr_node_indices is a matrix containing the index  *
     * of the *first* rr_node at a given (i,j) location.                       */

    t_rr_node_indices indices;

    /* Alloc the lookup table */
    indices.resize(NUM_RR_TYPES);
    for (t_rr_type rr_type : RR_TYPES) {
        if (rr_type == CHANX) {
            indices[rr_type].resize(grid.height());
            for (size_t y = 0; y < grid.height(); ++y) {
                indices[rr_type][y].resize(grid.width());
                for (size_t x = 0; x < grid.width(); ++x) {
                    indices[rr_type][y][x].resize(NUM_SIDES);
                }
            }
        } else {
            indices[rr_type].resize(grid.width());
            for (size_t x = 0; x < grid.width(); ++x) {
                indices[rr_type][x].resize(grid.height());
                for (size_t y = 0; y < grid.height(); ++y) {
                    indices[rr_type][x][y].resize(NUM_SIDES);
                }
            }
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

std::vector<int> get_rr_node_indices(const t_rr_node_indices& L_rr_node_indices,
                                     int x, int y, t_rr_type rr_type, int ptc) {
    /*
     * Like get_rr_node_index() but returns all matching nodes,
     * rather than just the first. This is particularly useful for getting all instances
     * of a specific IPIN/OPIN at a specific gird tile (x,y) location.
     */
    std::vector<int> indices;

    if (rr_type == IPIN || rr_type == OPIN) {
        //For pins we need to look at all the sides of the current grid tile

        for (e_side side : SIDES) {
            int rr_node_index = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc, side);

            if (rr_node_index >= 0) {
                indices.push_back(rr_node_index);
            }
        }
    } else {
        //Sides do not effect non-pins so there should only be one per ptc
        int rr_node_index = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc);
        indices.push_back(rr_node_index);
    }

    return indices;
}

int get_rr_node_index(const t_rr_node_indices& L_rr_node_indices,
        int x, int y, t_rr_type rr_type, int ptc, e_side side) {
    /* 
     * Returns the index of the specified routing resource node.  (x,y) are 
     * the location within the FPGA, rr_type specifies the type of resource, 
     * and ptc gives the number of this resource.  ptc is the class number,
     * pin number or track number, depending on what type of resource this
     * is.  All ptcs start at 0 and go up to pins_per_clb-1 or the equivalent.
     * There are type->num_class SOURCEs + SINKs, type->num_pins IPINs + OPINs,
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

    t_type_ptr type = device_ctx.grid[x][y].type;

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
            VTR_ASSERT(ptc < type->num_class);
            VTR_ASSERT(type->class_inf[ptc].type == DRIVER);
            break;

        case SINK:
            VTR_ASSERT(ptc < type->num_class);
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
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Bad rr_node passed to get_rr_node_index.\n"
                    "Request for type=%d ptc=%d at (%d, %d).\n",
                    rr_type, ptc, x, y);
    }

    return ((unsigned)ptc < lookup.size() ? lookup[ptc] : -1);
}

int find_average_rr_node_index(
        int device_width, int device_height, t_rr_type rr_type, int ptc,
        const t_rr_node_indices& L_rr_node_indices) {

    /* Find and return the index to a rr_node that is located at the "center" *
     * of the current grid array, if possible.  In the event the "center" of  *
     * the grid array is an EMPTY or IO node, then retry alterate locations.  *
     * Worst case, this function will simply return the 1st non-EMPTY and     *
     * non-IO node.                                                           */

    int inode = get_rr_node_index(L_rr_node_indices, (device_width) / 2, (device_height) / 2,
            rr_type, ptc);

    if (inode == -1) {
        inode = get_rr_node_index(L_rr_node_indices, (device_width) / 4, (device_height) / 4,
                rr_type, ptc);
    }
    if (inode == -1) {
        inode = get_rr_node_index(L_rr_node_indices, (device_width) / 4 * 3, (device_height) / 4 * 3,
                rr_type, ptc);
    }
    if (inode == -1) {

        auto& device_ctx = g_vpr_ctx.device();

        for (int x = 0; x < device_width; ++x) {
            for (int y = 0; y < device_height; ++y) {

                if (device_ctx.grid[x][y].type == device_ctx.EMPTY_TYPE)
                    continue;
                if (device_ctx.grid[x][y].type == device_ctx.IO_TYPE)
                    continue;

                inode = get_rr_node_index(L_rr_node_indices, x, y, rr_type, ptc);
                if (inode != -1)
                    break;
            }
            if (inode != -1)
                break;
        }
    }
    return (inode);
}

int get_track_to_pins(
        int seg, int chan, int track, int tracks_per_chan,
        t_linked_edge ** edge_list_ptr, const t_rr_node_indices& L_rr_node_indices,
        const t_track_to_pin_lookup& track_to_pin_lookup,
        t_seg_details * seg_details,
        enum e_rr_type chan_type, int chan_length, int wire_to_ipin_switch,
        enum e_directionality directionality) {

    /*
     * Adds the fan-out edges from wire segment at (chan, seg, track) to adjacent 
     * blocks along the wire's length
     */

    t_linked_edge *edge_list_head;
    int j, pass, iconn, phy_track, end, max_conn, ipin, x, y, num_conn;
    t_type_ptr type;

    auto& device_ctx = g_vpr_ctx.device();

    /* End of this wire */
    end = get_seg_end(seg_details, track, seg, chan, chan_length);

    edge_list_head = *edge_list_ptr;
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
                if (device_ctx.grid[x][y].type == device_ctx.EMPTY_TYPE)
                    continue;

                /* Move from logical (straight) to physical (twisted) track index 
                 * - algorithm assigns ipin connections to same physical track index
                 * so that the logical track gets distributed uniformly */

                phy_track = vpr_to_phy_track(track, chan, j, seg_details, directionality);
                phy_track %= tracks_per_chan;

                /* We need the type to find the ipin map for this type */
                type = device_ctx.grid[x][y].type;
                int width_offset = device_ctx.grid[x][y].width_offset;
                int height_offset = device_ctx.grid[x][y].height_offset;

                max_conn = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side].size();
                for (iconn = 0; iconn < max_conn; iconn++) {
                    ipin = track_to_pin_lookup[type->index][phy_track][width_offset][height_offset][side][iconn];

                    /* Check there is a connection and Fc map isn't wrong */
                    /*int to_node = get_rr_node_index(L_rr_node_indices, x + width_offset, y + height_offset, IPIN, ipin, side);*/
                    int to_node = get_rr_node_index(L_rr_node_indices, x, y, IPIN, ipin, side);
                    if (to_node >= 0) {
                        edge_list_head = insert_in_edge_list(edge_list_head, to_node, wire_to_ipin_switch);
                        ++num_conn;
                    }
                }
            }
        }
    }

    *edge_list_ptr = edge_list_head;
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
int get_track_to_tracks(
        const int from_chan, const int from_seg, const int from_track,
        const t_rr_type from_type, const int to_seg, const t_rr_type to_type,
        const int chan_len, const int max_chan_width, const DeviceGrid& grid,
        const int Fs_per_side, short ******sblock_pattern,
        t_linked_edge **edge_list,
        const t_seg_details * from_seg_details,
        const t_seg_details * to_seg_details,
        const t_chan_details& to_chan_details,
        const enum e_directionality directionality,
        const t_rr_node_indices& L_rr_node_indices, bool * L_rr_edge_done,
        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
        t_sb_connection_map *sb_conn_map) {

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

    int from_switch = from_seg_details[from_track].arch_wire_switch;

    //The absolute coordinate along the channel where the switch block at the
    //beginning of the current wire segment is located
    int start_sb_seg = from_seg - 1;

    //The absolute coordinate along the channel where the switch block at the
    //end of the current wire segment is lcoated
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

        if (!should_create_switchblock(grid, from_chan, sb_seg, from_type, to_type)) {
            continue; //Do not create an SB here
        }

        /* Get the coordinates of the current SB from the perspective of the destination channel.
           i.e. for segments laid in the x-direction, sb_seg corresponds to the x coordinate and from_chan to the y,
           but for segments in the y-direction, from_chan is the x coordinate and sb_seg is the y. So here we reverse
           the coordinates if necessary */
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
           channel type this function is used; so coordinates are reversed as necessary */
        if (to_type == CHANX) {
            to_seg_details = to_chan_details[to_seg][to_chan];
        } else {
            to_seg_details = to_chan_details[to_chan][to_seg];
        }

        if (to_seg_details[0].length == 0)
            continue;

        /* Figure out whether the switch block at the current sb_seg coordinate is *behind*
           the target channel segment (with respect to VPR coordinate system) */
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
           one of two directions. If we're in CHANX, we can connect to it from the left or
           right, provided we're not at a track endpoint. And similarly for a source track
           in CHANY. */
        /* Do edges going from the right SB side (if we're in CHANX) or top (if we're in CHANY). 
           However, can't connect to right (top) if already at rightmost (topmost) track end */
        if (sb_seg < end_sb_seg) {
            if (custom_switch_block) {
                if (DEC_DIRECTION == from_seg_details[from_track].direction ||
                        BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(from_track, to_chan, to_seg,
                            to_type, from_side_a, to_side, L_rr_node_indices,
                            sb_conn_map, L_rr_edge_done, edge_list);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                       switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_a][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(conn_tracks,
                            L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
                            to_seg_details, from_is_sblock, from_switch, L_rr_edge_done,
                            directionality, edge_list);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the top or right of SB so it
                     * makes the most sense to only get there from DEC_DIRECTION wires. */
                    if ((from_is_sblock)
                            && (DEC_DIRECTION == from_seg_details[from_track].direction)) {
                        num_conn += get_unidir_track_to_chan_seg(
                                from_track, to_chan,
                                to_seg, to_sb, to_type, max_chan_width, grid,
                                from_side_a, to_side, Fs_per_side,
                                sblock_pattern, L_rr_node_indices, to_seg_details,
                                L_rr_edge_done, &Fs_clipped, edge_list);
                    }
                }
            }
        }

        /* Do the edges going from the left SB side (if we're in CHANX) or bottom (if we're in CHANY)
           However, can't connect to left (bottom) if already at leftmost (bottommost) track end */
        if (sb_seg > start_sb_seg) {
            if (custom_switch_block) {
                if (INC_DIRECTION == from_seg_details[from_track].direction ||
                        BI_DIRECTIONAL == directionality) {
                    num_conn += get_track_to_chan_seg(from_track, to_chan, to_seg,
                            to_type, from_side_b, to_side, L_rr_node_indices,
                            sb_conn_map, L_rr_edge_done, edge_list);
                }
            } else {
                if (BI_DIRECTIONAL == directionality) {
                    /* For bidir, the target segment might have an unbuffered (bidir pass transistor)
                       switchbox, so we follow through regardless of whether the current segment has an SB */
                    conn_tracks = switch_block_conn[from_side_b][to_side][from_track];
                    num_conn += get_bidir_track_to_chan_seg(conn_tracks,
                            L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
                            to_seg_details, from_is_sblock, from_switch, L_rr_edge_done,
                            directionality, edge_list);
                }
                if (UNI_DIRECTIONAL == directionality) {
                    /* No fanout if no SB. */
                    /* Also, we are connecting from the bottom or left of SB so it
                     * makes the most sense to only get there from INC_DIRECTION wires. */
                    if ((from_is_sblock)
                            && (INC_DIRECTION == from_seg_details[from_track].direction)) {
                        num_conn += get_unidir_track_to_chan_seg(
                                from_track, to_chan,
                                to_seg, to_sb, to_type, max_chan_width, grid,
                                from_side_b, to_side, Fs_per_side,
                                sblock_pattern, L_rr_node_indices, to_seg_details,
                                L_rr_edge_done, &Fs_clipped, edge_list);
                    }
                }
            }
        }
    }

    return num_conn;
}

static int get_bidir_track_to_chan_seg(
        const std::vector<int> conn_tracks,
        const t_rr_node_indices& L_rr_node_indices, const int to_chan, const int to_seg,
        const int to_sb, const t_rr_type to_type, const t_seg_details * seg_details,
        const bool from_is_sblock, const int from_switch,
        bool * L_rr_edge_done,
        const enum e_directionality directionality,
        t_linked_edge **edge_list) {
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

        /* Skip edge if already done */
        if (L_rr_edge_done[to_node]) {
            continue;
        }

        /* Get the switches for any edges between the two tracks */
        to_switch = seg_details[to_track].arch_wire_switch;

        to_is_sblock = is_sblock(to_chan, to_seg, to_sb, to_track, seg_details,
                directionality);
        get_switch_type(from_is_sblock, to_is_sblock, from_switch, to_switch,
                switch_types);

        /* There are up to two switch edges allowed from track to track */
        for (i = 0; i < 2; ++i) {
            /* If the switch_type entry is empty, skip it */
            if (OPEN == switch_types[i]) {
                continue;
            }

            /* Add the edge to the list */
            *edge_list = insert_in_edge_list(*edge_list, to_node,
                    switch_types[i]);
            /* Mark the edge as now done */
            L_rr_edge_done[to_node] = true;
            ++num_conn;
        }
    }

    return num_conn;
}

/* Figures out the edges that should connect the given wire segment to the given
   channel segment, adds these edges to 'edge_list' and returns the number of 
   edges added .
   See route/build_switchblocks.c for a detailed description of how the switch block
   connection map sb_conn_map is generated. */
static int get_track_to_chan_seg(
        const int from_wire, const int to_chan, const int to_seg,
        const t_rr_type to_chan_type,
        const e_side from_side, const e_side to_side,
        const t_rr_node_indices&L_rr_node_indices,
        t_sb_connection_map *sb_conn_map,
        bool * L_rr_edge_done,
        t_linked_edge **edge_list) {

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
           at a specific coordinate sb_coord */
        vector<t_switchblock_edge> &conn_vector = (*sb_conn_map)[sb_coord];

        /* go through the connections... */
        for (int iconn = 0; iconn < (int) conn_vector.size(); ++iconn) {
            if (conn_vector.at(iconn).from_wire != from_wire) continue;

            int to_wire = conn_vector.at(iconn).to_wire;
            int to_node = get_rr_node_index(L_rr_node_indices, to_x, to_y, to_chan_type, to_wire);

            /* Get the index of the switch connecting the two wires */
            int src_switch = conn_vector[iconn].switch_ind;

            /* Skip edge if already done */
            if (L_rr_edge_done[to_node]) {
                continue;
            }

            *edge_list = insert_in_edge_list(*edge_list, to_node, src_switch);
            L_rr_edge_done[to_node] = true;
            ++edge_count;
        }
    } else {
        /* specified sb_conn_map entry does not exist -- do nothing */
    }
    return edge_count;
}

static int get_unidir_track_to_chan_seg(
        const int from_track, const int to_chan, const int to_seg, const int to_sb,
        const t_rr_type to_type, const int max_chan_width, const DeviceGrid& grid,
        const enum e_side from_side, const enum e_side to_side,
        const int Fs_per_side,
        short ******sblock_pattern, const t_rr_node_indices& L_rr_node_indices,
        const t_seg_details * seg_details, bool * L_rr_edge_done,
        bool * Fs_clipped, t_linked_edge **edge_list) {

    int num_labels = 0;
    int *mux_labels = NULL;

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
    mux_labels = label_wire_muxes(to_chan, to_seg, seg_details, UNDEFINED, max_len,
            to_dir, max_chan_width, false, &num_labels, &dummy);

    /* Can't connect if no muxes. */
    if (num_labels < 1) {
        if (mux_labels) {
            vtr::free(mux_labels);
            mux_labels = NULL;
        }
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
            if (L_rr_edge_done[to_node])
                continue;

            /* Add edge to list. */
            L_rr_edge_done[to_node] = true;
            *edge_list = insert_in_edge_list(*edge_list, to_node, seg_details[to_track].arch_wire_switch);
            ++count;
        }
    }

    if (mux_labels) {
        vtr::free(mux_labels);
        mux_labels = NULL;
    }
    return count;
}

bool is_sblock(const int chan, int wire_seg, const int sb_seg, const int track,
        const t_seg_details * seg_details,
        const enum e_directionality directionality) {

    int length, ofs, fac;

    fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    length = seg_details[track].length;

    /* Make sure they gave us correct start */
    wire_seg = get_seg_start(seg_details, track, chan, wire_seg);

    ofs = sb_seg - wire_seg + 1; /* Ofset 0 is behind us, so add 1 */

    VTR_ASSERT(ofs >= 0);
    VTR_ASSERT(ofs < (length + 1));

    /* If unidir segment that is going backwards, we need to flip the ofs */
    if ((ofs % fac) > 0) {
        ofs = length - ofs;
    }

    return seg_details[track].sb[ofs];
}

static void get_switch_type(
        bool is_from_sblock, bool is_to_sblock,
        short from_node_switch, short to_node_switch, short switch_types[2]) {
    /* This routine looks at whether the from_node and to_node want a switch,  *
     * and what type of switch is used to connect *to* each type of node       *
     * (from_node_switch and to_node_switch).  It decides what type of switch, *
     * if any, should be used to go from from_node to to_node.  If no switch   *
     * should be inserted (i.e. no connection), it returns OPEN.  Its returned *
     * values are in the switch_types array.  It needs to return an array      *
     * because one topology (a buffer in the forward direction and a pass      *
     * transistor in the backward direction) results in *two* switches.        */

    bool forward_pass_trans;
    bool backward_pass_trans;
    int used, min_switch, max_switch;

    auto& device_ctx = g_vpr_ctx.device();

    switch_types[0] = OPEN; /* No switch */
    switch_types[1] = OPEN;
    used = 0;
    forward_pass_trans = false;
    backward_pass_trans = false;

    /* Connect forward if we are a sblock */
    if (is_from_sblock) {
        switch_types[used] = to_node_switch;
        if (false == device_ctx.arch_switch_inf[to_node_switch].buffered()) {
            forward_pass_trans = true;
        }
        ++used;
    }

    /* Check for pass_trans coming backwards */
    if (is_to_sblock) {
        if (false == device_ctx.arch_switch_inf[from_node_switch].buffered()) {
            switch_types[used] = from_node_switch;
            backward_pass_trans = true;
            ++used;
        }
    }

    /* Take the larger pass trans if there are two */
    if (forward_pass_trans && backward_pass_trans) {
        min_switch = min(to_node_switch, from_node_switch);
        max_switch = max(to_node_switch, from_node_switch);

        /* Take the smaller index unless the other 
         * pass_trans is bigger (smaller R). */
        if (used < 2) {
            VPR_THROW(VPR_ERROR_ROUTE, "Used %d (min: %d max: %d)", used, min_switch, max_switch);
	}
        switch_types[used] = min_switch;
        if (device_ctx.arch_switch_inf[max_switch].R < device_ctx.arch_switch_inf[min_switch].R) {
            switch_types[used] = max_switch;
        }
        ++used;
    }
}

static int vpr_to_phy_track(
        const int itrack, const int chan_num, const int seg_num,
        const t_seg_details * seg_details,
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

    group_start = seg_details[itrack].group_start;
    group_size = seg_details[itrack].group_size;

    vpr_offset_for_first_phy_track = (chan_num + seg_num - 1)
            % (group_size / fac);
    vpr_offset = (itrack - group_start) / fac;
    phy_offset = (vpr_offset_for_first_phy_track + vpr_offset)
            % (group_size / fac);
    phy_track = group_start + (fac * phy_offset) + (itrack - group_start) % fac;

    return phy_track;
}

short ******alloc_sblock_pattern_lookup(
        const DeviceGrid& grid, const int max_chan_width) {

    /* loading up the sblock connection pattern matrix. It's a huge matrix because
     * for nonquantized W, it's impossible to make simple permutations to figure out
     * where muxes are and how to connect to them such that their sizes are balanced */

    /* Do chunked allocations to make freeing easier, speed up malloc and free, and
     * reduce some of the memory overhead. Could use fewer malloc's but this way
     * avoids all considerations of pointer sizes and allignment. */

    /* Alloc each list of pointers in one go. items is a running product that increases
     * with each new dimension of the matrix. */

    VTR_ASSERT(grid.width() > 0);
    VTR_ASSERT(grid.height() > 0);

    size_t items = 1;
    items *= (grid.width() - 1);
    short ******i_list = (short ******) vtr::malloc(sizeof (short *****) * items);
    items *= (grid.height() - 1);
    short *****j_list = (short *****) vtr::malloc(sizeof (short ****) * items);
    items *= (4);
    short ****from_side_list = (short ****) vtr::malloc(sizeof (short ***) * items);
    items *= (4);
    short ***to_side_list = (short ***) vtr::malloc(sizeof (short **) * items);
    items *= (max_chan_width);
    short **from_track_list = (short **) vtr::malloc(sizeof (short *) * items);
    items *= (4);
    short *from_track_types = (short *) vtr::malloc(sizeof (short) * items);

    /* Build the pointer lists to form the multidimensional array */
    short ******sblock_pattern = i_list;
    i_list += (grid.width() - 1); /* Skip forward */
    for (size_t i = 0; i < (grid.width() - 1); ++i) {

        sblock_pattern[i] = j_list;
        j_list += (grid.height() - 1); /* Skip forward */
        for (size_t j = 0; j < (grid.height() - 1); ++j) {

            sblock_pattern[i][j] = from_side_list;
            from_side_list += (4); /* Skip forward */
            for (e_side from_side : {TOP, RIGHT, BOTTOM, LEFT}) {

                sblock_pattern[i][j][from_side] = to_side_list;
                to_side_list += (4); /* Skip forward */
                for (e_side to_side : {TOP, RIGHT, BOTTOM, LEFT}) {

                    sblock_pattern[i][j][from_side][to_side] = from_track_list;
                    from_track_list += (max_chan_width); /* Skip forward max_chan_width items */
                    for (int from_track = 0; from_track < max_chan_width; from_track++) {

                        sblock_pattern[i][j][from_side][to_side][from_track] = from_track_types;
                        from_track_types += (4); /* Skip forward */

                        /* Set initial value to be unset */
                        sblock_pattern[i][j][from_side][to_side][from_track][0] = UN_SET; // to_mux
                        sblock_pattern[i][j][from_side][to_side][from_track][1] = UN_SET; // to_track

                        sblock_pattern[i][j][from_side][to_side][from_track][2] = UN_SET; // alt_mux
                        sblock_pattern[i][j][from_side][to_side][from_track][3] = UN_SET; // alt_track
                    }
                }
            }
        }
    }

    /* This is the outer pointer to the full matrix */
    return sblock_pattern;
}

void free_sblock_pattern_lookup(
        short ******sblock_pattern) {

    /* This free function corresponds to the chunked matrix 
     * allocation above and there should only be one free
     * call for each dimension. */

    /* Free dimensions from the inner one, outwards so
     * we can still access them. The comments beside
     * each one indicate the corresponding name used when
     * allocating them. */
    vtr::free(*****sblock_pattern); /* from_track_types */
    vtr::free(****sblock_pattern); /* from_track_list */
    vtr::free(***sblock_pattern); /* to_side_list */
    vtr::free(**sblock_pattern); /* from_side_list */
    vtr::free(*sblock_pattern); /* j_list */
    vtr::free(sblock_pattern); /* i_list */
}

void load_sblock_pattern_lookup(
        const int i, const int j,
        const DeviceGrid& grid,
        const t_chan_width *nodes_per_chan,
        const t_chan_details& chan_details_x, const t_chan_details& chan_details_y,
        const int /*Fs*/, const enum e_switch_block_type switch_block_type,
        short ******sblock_pattern) {

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
     * are labelled first 0,1,2,... p-1, then the incoming passing wires with sblock are labelled 
     * p,p+1,p+2,... k-1 (for total of k). By this convention, one can easily distinguish the ending
     * wires from the passing wires by checking a label against num_ending_wires variable. 
     *
     * After labelling all the incoming wires, this routine labels the muxes on the side we're currently
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
    int *wire_mux_on_track[4];
    int *incoming_wire_label[4];
    int num_incoming_wires[4];
    int num_ending_wires[4];
    int num_wire_muxes[4];

    /* "Label" the wires around the switch block by connectivity. */
    for (e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
        /* Assume the channel segment doesn't exist. */
        wire_mux_on_track[side] = NULL;
        incoming_wire_label[side] = NULL;
        num_incoming_wires[side] = 0;
        num_ending_wires[side] = 0;
        num_wire_muxes[side] = 0;

        /* Skip the side and leave the zero'd value if the
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
                VTR_ASSERT_MSG(false, "Unrecognzied side");
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

        t_seg_details * seg_details = (vert ? chan_details_y[chan][seg] : chan_details_x[seg][chan]);
        if (seg_details[0].length <= 0)
            continue;

        /* Figure out all the tracks on a side that are ending and the
         * ones that are passing through and have a SB. */
        enum e_direction end_dir = (pos_dir ? DEC_DIRECTION : INC_DIRECTION);
        incoming_wire_label[side] = label_incoming_wires(chan, seg, sb_seg,
                seg_details, chan_len, end_dir, nodes_per_chan->max,
                &num_incoming_wires[side], &num_ending_wires[side]);

        /* Figure out all the tracks on a side that are starting. */
        int dummy;
        enum e_direction start_dir = (pos_dir ? INC_DIRECTION : DEC_DIRECTION);
        wire_mux_on_track[side] = label_wire_muxes(chan, seg,
                seg_details, UNDEFINED, chan_len, start_dir, nodes_per_chan->max,
                false, &num_wire_muxes[side], &dummy);
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

                if (incoming_wire_label[side_cw][itrack] != UN_SET) {

                    int mux = get_simple_switch_block_track((enum e_side)side_cw,
                            (enum e_side)to_side,
                            incoming_wire_label[side_cw][ichan],
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

                if (incoming_wire_label[side_ccw][itrack] != UN_SET) {

                    int mux = get_simple_switch_block_track((enum e_side)side_ccw,
                            (enum e_side)to_side,
                            incoming_wire_label[side_ccw][ichan],
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

                           There is no connection from wire segment midpoints to the opposite switch block
                           side, so there's nothing to be done here (at Fs=3, this connection is implicit for passing 
                           wires and at Fs>3 the code in this function seems to create heavily unbalanced 
                           switch patterns). Additionally, the code in build_rr_chan() explicitly skips 
                           connections from wire segment midpoints to the opposide sb side (for switch block patterns
                           generated with this function) so any such assignment to sblock_pattern will be ignored anyway. */
                    }
                }
            }
        }
    }

    for (e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
        if (incoming_wire_label[side]) {
            vtr::free(incoming_wire_label[side]);
        }
        if (wire_mux_on_track[side]) {
            vtr::free(wire_mux_on_track[side]);
        }
    }
}

static int *label_wire_muxes(
        const int chan_num, const int seg_num,
        const t_seg_details * seg_details, const int seg_type_index, const int max_len,
        const enum e_direction dir, const int max_chan_width,
        const bool check_cb, int *num_wire_muxes, int *num_wire_muxes_cb_restricted) {

    /* Labels the muxes on that side (seg_num, chan_num, direction). The returned array
     * maps a label to the actual track #: array[0] = <the track number of the first/lowest mux> 
     * This routine orders wire muxes by their natural order, i.e. track #
     * If seg_type_index == UNDEFINED, all segments in the channel are considered. Otherwise this routine
     * only looks at segments that belong to the specified segment type. */

    int itrack, start, end, num_labels, num_labels_restricted, pass;
    int *labels = NULL;
    bool is_endpoint;

    /* COUNT pass then a LOAD pass */
    num_labels = 0;
    num_labels_restricted = 0;
    for (pass = 0; pass < 2; ++pass) {
        /* Alloc the list on LOAD pass */
        if (pass > 0) {
            labels = (int *) vtr::malloc(sizeof (int) * num_labels);
            num_labels = 0;
        }

        /* Find the tracks that are starting. */
        for (itrack = 0; itrack < max_chan_width; ++itrack) {
            start = get_seg_start(seg_details, itrack, chan_num, seg_num);
            end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

            /* Skip tracks that are undefined */
            if (seg_details[itrack].length == 0) {
                continue;
            }

            /* Skip tracks going the wrong way */
            if (seg_details[itrack].direction != dir) {
                continue;
            }

            if (seg_type_index != UNDEFINED) {
                /* skip tracks that don't belong to the specified segment type */
                if (seg_details[itrack].index != seg_type_index) {
                    continue;
                }
            }

            /* Determine if we are a wire startpoint */
            is_endpoint = (seg_num == start);
            if (DEC_DIRECTION == seg_details[itrack].direction) {
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
                if ((!check_cb) || (seg_details[itrack].cb[0] == true)) {
                    if (pass > 0) {
                        labels[num_labels] = itrack;
                    }
                    ++num_labels;
                }
                if (pass > 0)
                    num_labels_restricted += (seg_details[itrack].cb[0] == true) ? 1 : 0;
            }
        }
    }

    *num_wire_muxes = num_labels;
    *num_wire_muxes_cb_restricted = num_labels_restricted;

    return labels;
}

static int *label_incoming_wires(
        const int chan_num, const int seg_num, const int sb_seg,
        const t_seg_details * seg_details, const int max_len,
        const enum e_direction dir, const int max_chan_width,
        int *num_incoming_wires, int *num_ending_wires) {

    /* Labels the incoming wires on that side (seg_num, chan_num, direction). 
     * The returned array maps a track # to a label: array[0] = <the new hash value/label for track 0>,
     * the labels 0,1,2,.. identify consecutive incoming wires that have sblock (passing wires with sblock and ending wires) */

    int itrack, start, end, i, num_passing, num_ending, pass;
    int *labels;
    bool sblock_exists, is_endpoint;

    /* Alloc the list of labels for the tracks */
    labels = (int *) vtr::malloc(max_chan_width * sizeof (int));
    for (i = 0; i < max_chan_width; ++i) {
        labels[i] = UN_SET; /* crash hard if unset */
    }

    num_ending = 0;
    num_passing = 0;
    for (pass = 0; pass < 2; ++pass) {
        for (itrack = 0; itrack < max_chan_width; ++itrack) {

            /* Skip tracks that are undefined */
            if (seg_details[itrack].length == 0) {
                continue;
            }

            if (seg_details[itrack].direction == dir) {
                start = get_seg_start(seg_details, itrack, chan_num, seg_num);
                end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

                /* Determine if we are a wire endpoint */
                is_endpoint = (seg_num == end);
                if (DEC_DIRECTION == seg_details[itrack].direction) {
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
    return labels;
}

static int find_label_of_track(
        int *wire_mux_on_track, int num_wire_muxes,
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


bool is_block_internal_switchblock(const DeviceGrid& grid, int x_coord, int y_coord) {

    t_type_ptr blk_type = grid[x_coord][y_coord].type;

    //Can only be 'inside' a block if it's width and height are > 1
    if (blk_type->width > 1 && blk_type->height > 1) {
        int width_offset = grid[x_coord][y_coord].width_offset;
        int height_offset = grid[x_coord][y_coord].height_offset;

        VTR_ASSERT(width_offset >= 0);
        VTR_ASSERT(height_offset >= 0);

        //Channles are located to the right and above the associated grid tile.
        //Therefore we must exclude grid tiles along the right/top edges of the
        //block (since thier channels are outside the block).
        if (width_offset < blk_type->width - 1 && height_offset < blk_type->height - 1) {
            return true;
        }
    }
    return false;
}

bool should_create_switchblock(const DeviceGrid& grid, int from_chan_coord, int from_seg_coord, t_rr_type from_chan_type, t_rr_type to_chan_type) {
    //Convert the chan/seg indicies to real x/y coordinates
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

    t_type_ptr blk_type = grid[x_coord][y_coord].type;
    int width_offset = grid[x_coord][y_coord].width_offset;
    int height_offset = grid[x_coord][y_coord].height_offset;

    e_sb_type sb_type = blk_type->switchblock_locations[width_offset][height_offset];
    if (sb_type == e_sb_type::FULL) {
        return true;
    } else if (sb_type == e_sb_type::STRAIGHT) {
        return from_chan_type == to_chan_type;
    } else if (sb_type == e_sb_type::TURNS) {
        return from_chan_type != to_chan_type;
    }

    VTR_ASSERT(sb_type == e_sb_type::NONE);
    return false;
}
