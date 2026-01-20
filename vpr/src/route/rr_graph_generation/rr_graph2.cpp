#include <cstdio>

#include "physical_types_util.h"
#include "vpr_utils.h"
#include "vtr_util.h"
#include "vtr_assert.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "rr_types.h"

constexpr short UN_SET = -1;
/************************** Subroutines local to this module ****************/

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

/******************** Subroutine definitions *******************************/

std::vector<int> get_seg_track_counts(int num_sets,
                                      const std::vector<t_segment_inf>& segment_inf,
                                      bool use_full_seg_groups) {
    // Scale factor so we can divide by any length and still use integers
    double scale = 1;
    int freq_sum = 0;
    for (const t_segment_inf& seg_inf : segment_inf) {
        scale *= seg_inf.length;
        freq_sum += seg_inf.frequency;
    }
    const double reduce = scale * freq_sum;

    // Init assignments to 0 and set the demand values
    std::vector<int> result(segment_inf.size(), 0);
    std::vector<double> demand(segment_inf.size());
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        demand[i] = scale * num_sets * segment_inf[i].frequency;
        if (use_full_seg_groups) {
            demand[i] /= segment_inf[i].length;
        }
    }

    // Keep assigning tracks until we use them up
    int assigned = 0;
    int imax = 0;
    int size = 0;
    while (assigned < num_sets) {
        // Find current maximum demand
        double max = 0;
        for (size_t i = 0; i < segment_inf.size(); ++i) {
            if (demand[i] > max) {
                imax = i;
                max = demand[i];
            }
        }

        // Assign tracks to the type and reduce the types demand
        size = use_full_seg_groups ? segment_inf[imax].length : 1;
        demand[imax] -= reduce;
        result[imax] += size;
        assigned += size;
    }

    // Undo last assignment if we were closer to goal without it
    if ((assigned - num_sets) > (size / 2)) {
        result[imax] -= size;
    }

    return result;
}

std::vector<t_seg_details> alloc_and_load_seg_details(int* max_chan_width,
                                                      const int max_len,
                                                      const std::vector<t_segment_inf>& segment_inf,
                                                      const bool use_full_seg_groups,
                                                      const e_directionality directionality) {
    /* Allocates and loads the seg_details data structure.  Max_len gives the   *
     * maximum length of a segment (dimension of array).  The code below tries  *
     * to:                                                                      *
     * (1) stagger the start points of segments of the same type evenly;        *
     * (2) spread out the limited number of connection boxes or switch boxes    *
     *     evenly along the length of a segment, starting at the segment ends;  *
     * (3) stagger the connection and switch boxes on different long lines,     *
     *     as they will not be staggered by different segment start points.     */

    // Unidir tracks are assigned in pairs, and bidir tracks individually
    int fac;
    if (directionality == BI_DIRECTIONAL) {
        fac = 1;
    } else {
        VTR_ASSERT(directionality == UNI_DIRECTIONAL);
        fac = 2;
    }

    if (*max_chan_width % fac != 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Routing channel width must be divisible by %d (channel width was %d)", fac, *max_chan_width);
    }

    // Map segment type fractions and groupings to counts of tracks
    const std::vector<int> sets_per_seg_type = get_seg_track_counts((*max_chan_width / fac),
                                                                    segment_inf, use_full_seg_groups);

    // Count the number tracks actually assigned.
    int tmp = std::accumulate(sets_per_seg_type.begin(), sets_per_seg_type.end(), 0) * fac;
    VTR_ASSERT(use_full_seg_groups || (tmp == *max_chan_width));
    *max_chan_width = tmp;

    std::vector<t_seg_details> seg_details(*max_chan_width);

    // Setup the seg_details data
    int cur_track = 0;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        int first_track = cur_track;

        const int num_sets = sets_per_seg_type[i];
        const int ntracks = fac * num_sets;
        if (ntracks < 1) {
            continue;
        }
        /* Avoid divide by 0 if ntracks */
        const bool longline = segment_inf[i].longline;
        const int length = (longline) ? max_len : segment_inf[i].length;

        const int arch_wire_switch = segment_inf[i].arch_wire_switch;
        const int arch_opin_switch = segment_inf[i].arch_opin_switch;
        const int arch_wire_switch_dec = segment_inf[i].arch_wire_switch_dec;
        const int arch_opin_switch_dec = segment_inf[i].arch_opin_switch_dec;
        VTR_ASSERT((arch_wire_switch == arch_opin_switch && arch_wire_switch_dec == arch_opin_switch_dec) || (directionality != UNI_DIRECTIONAL));

        // Set up the tracks of same type
        int group_start = 0;
        for (int itrack = 0; itrack < ntracks; itrack++) {
            // set the name of the segment type this track belongs to
            seg_details[cur_track].type_name = segment_inf[i].name;

            // Remember the start track of the current wire group
            if ((itrack / fac) % length == 0 && (itrack % fac) == 0) {
                group_start = cur_track;
            }

            seg_details[cur_track].length = length;
            seg_details[cur_track].longline = longline;

            /* Stagger the start points in for each track set. The
             * pin mappings should be aware of this when choosing an
             * intelligent way of connecting pins and tracks.
             * cur_track is used as an offset so that extra tracks
             * from different segment types are hopefully better balanced. */
            seg_details[cur_track].start = (cur_track / fac) % length + 1;

            // These properties are used for vpr_to_phy_track to determine twisting of wires.
            seg_details[cur_track].group_start = group_start;
            seg_details[cur_track].group_size = std::min(ntracks + first_track - group_start, length * fac);
            VTR_ASSERT(0 == seg_details[cur_track].group_size % fac);
            if (0 == seg_details[cur_track].group_size) {
                seg_details[cur_track].group_size = length * fac;
            }

            /* Setup the cb and sb patterns. Global route graphs can't depopulate cb and sb
             * since this is a property of a detailed route. */
            seg_details[cur_track].cb = std::make_unique<bool[]>(length);
            seg_details[cur_track].sb = std::make_unique<bool[]>(length + 1);
            for (int j = 0; j < length; ++j) {
                if (seg_details[cur_track].longline) {
                    seg_details[cur_track].cb[j] = true;
                } else {
                    // Use the segment's pattern.
                    int index = j % segment_inf[i].cb.size();
                    seg_details[cur_track].cb[j] = segment_inf[i].cb[index];
                }
            }
            for (int j = 0; j < (length + 1); ++j) {
                if (seg_details[cur_track].longline) {
                    seg_details[cur_track].sb[j] = true;
                } else {
                    /* Use the segment's pattern. */
                    int index = j % segment_inf[i].sb.size();
                    seg_details[cur_track].sb[j] = segment_inf[i].sb[index];
                }
            }

            seg_details[cur_track].Rmetal = segment_inf[i].Rmetal;
            seg_details[cur_track].Cmetal = segment_inf[i].Cmetal;
            //seg_details[cur_track].Cmetal_per_m = segment_inf[i].Cmetal_per_m;

            if (BI_DIRECTIONAL == directionality) {
                seg_details[cur_track].direction = Direction::BIDIR;
            } else {
                VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                seg_details[cur_track].direction = (itrack % 2) ? Direction::DEC : Direction::INC;
            }

            // check for directionality to set the wire_switch and opin_switch
            // if not specified in the architecture file, we will use a same mux for both directions
            if (seg_details[cur_track].direction == Direction::INC || seg_details[cur_track].direction == Direction::BIDIR || arch_wire_switch_dec == -1) {
                seg_details[cur_track].arch_opin_switch = arch_opin_switch;
                seg_details[cur_track].arch_wire_switch = arch_wire_switch;
            } else {
                VTR_ASSERT(seg_details[cur_track].direction == Direction::DEC);
                seg_details[cur_track].arch_opin_switch = arch_opin_switch_dec;
                seg_details[cur_track].arch_wire_switch = arch_wire_switch_dec;
            }

            seg_details[cur_track].index = i;
            seg_details[cur_track].abs_index = segment_inf[i].seg_index;

            ++cur_track;
        }
    } /* End for each segment type. */

    seg_details.resize(cur_track);
    return seg_details;
}

/* Allocates and loads the chan_details data structure, a 2D array of
 * seg_details structures. This array is used to handle unique seg_details
 * (ie. channel segments) for each horizontal and vertical channel. */
void alloc_and_load_chan_details(const DeviceGrid& grid,
                                 const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details_x,
                                 const std::vector<t_seg_details>& seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y) {
    chan_details_x = init_chan_details(grid, nodes_per_chan, seg_details_x, e_parallel_axis::X_AXIS);
    chan_details_y = init_chan_details(grid, nodes_per_chan, seg_details_y, e_parallel_axis::Y_AXIS);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(grid, nodes_per_chan, chan_details_x, chan_details_y);
}

t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details,
                                 const e_parallel_axis seg_parallel_axis) {
    const int num_seg_details = (int)seg_details.size();
    if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan.x_max);
    } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan.y_max);
    }

    t_chan_details chan_details({grid.width(), grid.height(), size_t(num_seg_details)});

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            t_chan_seg_details* p_seg_details = chan_details[x][y].data();
            for (int i = 0; i < num_seg_details; ++i) {
                p_seg_details[i] = t_chan_seg_details(&seg_details[i]);

                int seg_start = -1;
                int seg_end = -1;

                if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
                    seg_start = get_seg_start(p_seg_details, i, y, x);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, y, grid.width() - 2); //-2 for no perim channels
                } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
                    seg_start = get_seg_start(p_seg_details, i, x, y);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, x, grid.height() - 2); //-2 for no perim channels
                }

                p_seg_details[i].set_seg_start(seg_start);
                p_seg_details[i].set_seg_end(seg_end);

                if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
                    if (i >= nodes_per_chan.x_list[y]) {
                        p_seg_details[i].set_length(0);
                    }
                } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
                    if (i >= nodes_per_chan.y_list[x]) {
                        p_seg_details[i].set_length(0);
                    }
                }
            }
        }
    }
    return chan_details;
}

void adjust_chan_details(const DeviceGrid& grid,
                         const t_chan_width& nodes_per_chan,
                         t_chan_details& chan_details_x,
                         t_chan_details& chan_details_y) {
    for (size_t y = 0; y <= grid.height() - 2; ++y) {    //-2 for no perim channels
        for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_x[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                               chan_details_x, e_parallel_axis::X_AXIS);
        }
    }

    for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_y[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, grid, nodes_per_chan,
                               chan_details_y, e_parallel_axis::Y_AXIS);
        }
    }
}

void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width& nodes_per_chan,
                        t_chan_details& chan_details,
                        const e_parallel_axis seg_parallel_axis) {
    int seg_index = (seg_parallel_axis == e_parallel_axis::X_AXIS ? x : y);
    int max_chan_width = 0;
    if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
        max_chan_width = nodes_per_chan.x_max;
    } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
        max_chan_width = nodes_per_chan.y_max;
    } else {
        VTR_ASSERT(seg_parallel_axis == e_parallel_axis::BOTH_AXIS);
        max_chan_width = nodes_per_chan.max;
    }

    for (int track = 0; track < max_chan_width; ++track) {
        int lx = (seg_parallel_axis == e_parallel_axis::X_AXIS ? x - 1 : x);
        int ly = (seg_parallel_axis == e_parallel_axis::X_AXIS ? y : y - 1);
        if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
            continue;

        while (chan_details[lx][ly][track].seg_end() >= seg_index) {
            chan_details[lx][ly][track].set_seg_end(seg_index - 1);
            lx = (seg_parallel_axis == e_parallel_axis::X_AXIS ? lx - 1 : lx);
            ly = (seg_parallel_axis == e_parallel_axis::X_AXIS ? ly : ly - 1);
            if (lx < 0 || ly < 0 || chan_details[lx][ly][track].length() == 0)
                break;
        }
    }

    for (int track = 0; track < max_chan_width; ++track) {
        size_t lx = (seg_parallel_axis == e_parallel_axis::X_AXIS ? x + 1 : x);
        size_t ly = (seg_parallel_axis == e_parallel_axis::X_AXIS ? y : y + 1);
        if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length() == 0) //-2 for no perim channels
            continue;

        while (chan_details[lx][ly][track].seg_start() <= seg_index) {
            chan_details[lx][ly][track].set_seg_start(seg_index + 1);
            lx = (seg_parallel_axis == e_parallel_axis::X_AXIS ? lx + 1 : lx);
            ly = (seg_parallel_axis == e_parallel_axis::X_AXIS ? ly : ly + 1);
            if (lx > grid.width() - 2 || ly > grid.height() - 2 || chan_details[lx][ly][track].length() == 0) //-2 for no perim channels
                break;
        }
    }
}

/* Returns the segment number at which the segment this track lies on
 * started. */
int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num) {
    int seg_start = 0;
    if (seg_details[itrack].seg_start() >= 0) {
        seg_start = seg_details[itrack].seg_start();
    } else {
        seg_start = 1;
        if (!seg_details[itrack].longline()) {
            int length = seg_details[itrack].length();
            int start = seg_details[itrack].start();

            /* Start is guaranteed to be between 1 and length. Hence, adding length to
             * the quantity in brackets below guarantees it will be non-negative. */
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

bool is_cblock(const int chan, const int seg, const int track, const t_chan_seg_details* seg_details) {
    int length = seg_details[track].length();

    // Make sure they gave us correct start
    int start_seg = get_seg_start(seg_details, track, chan, seg);

    int ofs = seg - start_seg;

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
        for (unsigned int i = 0; i < types.size(); i++) {
            if (!track_to_pin_map[i].empty()) {
                for (int track = 0; track < max_chan_width; ++track) {
                    for (int width = 0; width < types[i].width; ++width) {
                        for (int height = 0; height < types[i].height; ++height) {
                            for (int side = 0; side < 4; ++side) {
                                fprintf(fp, "\nTYPE:%s width:%d height:%d \n", types[i].name.c_str(), width, height);
                                fprintf(fp, "\nSIDE:%d TRACK:%d \n", side, track);
                                for (size_t con = 0; con < track_to_pin_map[i][track][width][height][side].size(); con++) {
                                    fprintf(fp, "%d ", track_to_pin_map[i][track][width][height][side][con]);
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

bool is_sblock(const int chan, int wire_seg, const int sb_seg, const int track, const t_chan_seg_details* seg_details, const e_directionality directionality) {
    int fac = 1;
    if (UNI_DIRECTIONAL == directionality) {
        fac = 2;
    }

    int length = seg_details[track].length();

    // Make sure they gave us correct start
    wire_seg = get_seg_start(seg_details, track, chan, wire_seg);

    int offset = sb_seg - wire_seg + 1; // Offset 0 is behind us, so add 1

    VTR_ASSERT(offset >= 0);
    VTR_ASSERT(offset < length + 1);

    // If unidir segment that is going backwards, we need to flip the ofs
    if (offset % fac > 0) {
        offset = length - offset;
    }

    return seg_details[track].sb(offset);
}

t_sblock_pattern alloc_sblock_pattern_lookup(const DeviceGrid& grid,
                                             const t_chan_width& nodes_per_chan) {
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
    VTR_ASSERT(nodes_per_chan.max >= 0);

    t_sblock_pattern sblock_pattern({{
                                        grid.width() - 1,
                                        grid.height() - 1,
                                        4, //From side
                                        4, //To side
                                        size_t(nodes_per_chan.max),
                                        4 //to_mux, to_trac, alt_mux, alt_track
                                    }},
                                    UN_SET);

    /* This is the outer pointer to the full matrix */
    return sblock_pattern;
}

void load_sblock_pattern_lookup(const int i,
                                const int j,
                                const DeviceGrid& grid,
                                const t_chan_width& nodes_per_chan,
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

    /* I identify three types of sblocks in the chip: 1) The core sblock, whose special
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

    static_assert(NUM_2D_SIDES == 4, "Should be 4 sides");
    std::array<std::vector<int>, NUM_2D_SIDES> wire_mux_on_track;
    std::array<std::vector<int>, NUM_2D_SIDES> incoming_wire_label;
    int num_incoming_wires[NUM_2D_SIDES];
    int num_ending_wires[NUM_2D_SIDES];
    int num_wire_muxes[NUM_2D_SIDES];

    /* "Label" the wires around the switch block by connectivity. */
    for (e_side side : TOTAL_2D_SIDES) {
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

    for (e_side to_side : TOTAL_2D_SIDES) {
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
                    itrack = ichan % nodes_per_chan.y_list[i];
                } else if (side_cw == RIGHT || side_cw == LEFT) {
                    itrack = ichan % nodes_per_chan.x_list[j];
                }

                if (incoming_wire_label[side_cw][itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((e_side)side_cw,
                                                            to_side,
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
                    itrack = ichan % nodes_per_chan.y_list[i];
                } else if (side_ccw == RIGHT || side_ccw == LEFT) {
                    itrack = ichan % nodes_per_chan.x_list[j];
                }

                if (incoming_wire_label[side_ccw][itrack] != UN_SET) {
                    int mux = get_simple_switch_block_track((e_side)side_ccw,
                                                            to_side,
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

void label_wire_muxes(const int chan_num,
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
    // COUNT pass then a LOAD pass
    int num_labels = 0;
    int num_labels_restricted = 0;
    for (int pass = 0; pass < 2; ++pass) {
        // Alloc the list on LOAD pass
        if (pass > 0) {
            labels.resize(num_labels);
            std::ranges::fill(labels, 0);
            num_labels = 0;
        }

        // Find the tracks that are starting.
        for (int itrack = 0; itrack < max_chan_width; ++itrack) {
            int start = get_seg_start(seg_details, itrack, chan_num, seg_num);
            int end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

            // Skip tracks that are undefined
            if (seg_details[itrack].length() == 0) {
                continue;
            }

            // Skip tracks going the wrong way
            if (seg_details[itrack].direction() != dir) {
                continue;
            }

            if (seg_type_index != UNDEFINED) {
                // skip tracks that don't belong to the specified segment type
                if (seg_details[itrack].index() != seg_type_index) {
                    continue;
                }
            }

            // Determine if we are a wire startpoint
            bool is_endpoint = (seg_num == start);
            if (Direction::DEC == seg_details[itrack].direction()) {
                is_endpoint = (seg_num == end);
            }

            // Count the labels and load if LOAD pass
            if (is_endpoint) {
                // not all wire endpoints can be driven by OPIN (depending on the <cb> pattern in the arch file)
                // the check_cb is targeting this arch specification:
                // if this function is called by get_unidir_opin_connections(),
                // then we need to check if mux connections can be added to this type of wire,
                // otherwise, this function should not consider <cb> specification.
                if (!check_cb || seg_details[itrack].cb(0) == true) {
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

    /* Alloc the list of labels for the tracks */
    labels.resize(max_chan_width);
    std::ranges::fill(labels, UN_SET);

    int num_ending = 0;
    int num_passing = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (int itrack = 0; itrack < max_chan_width; ++itrack) {
            /* Skip tracks that are undefined */
            if (seg_details[itrack].length() == 0) {
                continue;
            }

            if (seg_details[itrack].direction() == dir) {
                int start = get_seg_start(seg_details, itrack, chan_num, seg_num);
                int end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

                /* Determine if we are a wire endpoint */
                bool is_endpoint = (seg_num == end);
                if (Direction::DEC == seg_details[itrack].direction()) {
                    is_endpoint = (seg_num == start);
                }

                /* Determine if we have a sblock on the wire */
                bool sblock_exists = is_sblock(chan_num, seg_num, sb_seg, itrack,
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
                        if (!is_endpoint && sblock_exists) {
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
    /* Returns the index/label in array wire_mux_on_track whose entry equals from_tracks. If none are
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

inline int get_chan_width(enum e_side side, const t_chan_width& nodes_per_chan) {
    return (side == TOP || side == BOTTOM ? nodes_per_chan.y_max : nodes_per_chan.x_max);
}
