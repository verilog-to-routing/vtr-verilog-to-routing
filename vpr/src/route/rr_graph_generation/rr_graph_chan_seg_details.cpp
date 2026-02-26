#include "rr_graph_chan_seg_details.h"
#include "device_grid.h"
#include "rr_graph_type.h"
#include "globals.h"

static t_chan_details init_chan_details(const t_chan_width& nodes_per_chan,
                                        const std::vector<t_seg_details>& seg_details,
                                        const e_parallel_axis seg_details_type);

static void adjust_chan_details(const t_chan_width& nodes_per_chan,
                                t_chan_details& chan_details_x,
                                t_chan_details& chan_details_y);

static void adjust_seg_details(const int x,
                               const int y,
                               const t_chan_width& nodes_per_chan,
                               t_chan_details& chan_details,
                               const e_parallel_axis seg_parallel_axis);

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

static t_chan_details init_chan_details(const t_chan_width& nodes_per_chan,
                                        const std::vector<t_seg_details>& seg_details,
                                        const e_parallel_axis seg_parallel_axis) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    const int num_seg_details = (int)seg_details.size();
    if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan.x_max);
    } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan.y_max);
    }

    // Gather interposer cuts along the segment dimension so that wires are
    // split at cut boundaries rather than spanning across them.
    std::vector<int> seg_dimension_cuts;
    if (grid.has_interposer_cuts()) {
        const auto& cuts_by_layer = (seg_parallel_axis == e_parallel_axis::X_AXIS)
                                        ? grid.get_vertical_interposer_cuts()
                                        : grid.get_horizontal_interposer_cuts();
        if (!cuts_by_layer.empty()) {
            seg_dimension_cuts = cuts_by_layer[0];
        }
    }

    t_chan_details chan_details({grid.width(), grid.height(), size_t(num_seg_details)});

    const std::vector<int> no_cuts;

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            t_chan_seg_details* p_seg_details = chan_details[x][y].data();
            for (int i = 0; i < num_seg_details; ++i) {
                p_seg_details[i] = t_chan_seg_details(&seg_details[i]);

                int seg_start = -1;
                int seg_end = -1;

                // Compute the natural start and end first (without interposer
                // cuts) so that the endpoint is based on the wire's true span.
                // Then clamp both by the interposer cuts.  We must NOT pass
                // a cut-adjusted start into get_seg_end because its internal
                // formula (istart + len - 1) would produce a wrong endpoint.
                int chan_num = -1;
                int seg_num = -1;
                int seg_max = -1;
                if (seg_parallel_axis == e_parallel_axis::X_AXIS) {
                    chan_num = (int)y;
                    seg_num = (int)x;
                    seg_max = (int)grid.width() - 2; //-2 for no perim channels
                } else if (seg_parallel_axis == e_parallel_axis::Y_AXIS) {
                    chan_num = (int)x;
                    seg_num = (int)y;
                    seg_max = (int)grid.height() - 2; //-2 for no perim channels
                }

                int natural_start = get_seg_start(p_seg_details, i, chan_num, seg_num, no_cuts);
                int natural_end = get_seg_end(p_seg_details, i, natural_start, chan_num, seg_max, no_cuts);

                seg_start = natural_start;
                seg_end = natural_end;
                for (int c : seg_dimension_cuts) {
                    if (c >= seg_start && c < seg_num) {
                        seg_start = c + 1;
                    }
                }
                for (int c : seg_dimension_cuts) {
                    if (c >= seg_start && c < seg_end) {
                        seg_end = c;
                    }
                }

                p_seg_details[i].set_seg_start(seg_start);
                p_seg_details[i].set_seg_end(seg_end);
                p_seg_details[i].set_length(seg_end - seg_start + 1);

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

static void adjust_chan_details(const t_chan_width& nodes_per_chan,
                                t_chan_details& chan_details_x,
                                t_chan_details& chan_details_y) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    for (size_t y = 0; y <= grid.height() - 2; ++y) {    //-2 for no perim channels
        for (size_t x = 0; x <= grid.width() - 2; ++x) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_x[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, nodes_per_chan,
                               chan_details_x, e_parallel_axis::X_AXIS);
        }
    }

    for (size_t x = 0; x <= grid.width() - 2; ++x) {      //-2 for no perim channels
        for (size_t y = 0; y <= grid.height() - 2; ++y) { //-2 for no perim channels

            /* Ignore any non-obstructed channel seg_detail structures */
            if (chan_details_y[x][y][0].length() > 0)
                continue;

            adjust_seg_details(x, y, nodes_per_chan,
                               chan_details_y, e_parallel_axis::Y_AXIS);
        }
    }
}

/* Allocates and loads the chan_details data structure, a 2D array of
 * seg_details structures. This array is used to handle unique seg_details
 * (ie. channel segments) for each horizontal and vertical channel. */
void alloc_and_load_chan_details(const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details_x,
                                 const std::vector<t_seg_details>& seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y) {
    chan_details_x = init_chan_details(nodes_per_chan, seg_details_x, e_parallel_axis::X_AXIS);
    chan_details_y = init_chan_details(nodes_per_chan, seg_details_y, e_parallel_axis::Y_AXIS);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(nodes_per_chan, chan_details_x, chan_details_y);
}

static void adjust_seg_details(const int x,
                               const int y,
                               const t_chan_width& nodes_per_chan,
                               t_chan_details& chan_details,
                               const e_parallel_axis seg_parallel_axis) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

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

std::vector<int> get_chan_seg_interposer_cuts(e_rr_type chan_type) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    if (!grid.has_interposer_cuts()) {
        return {};
    }

    const auto& cuts_by_layer = (chan_type == e_rr_type::CHANX)
                                    ? grid.get_vertical_interposer_cuts()
                                    : grid.get_horizontal_interposer_cuts();
    if (!cuts_by_layer.empty()) {
        return cuts_by_layer[0];
    }
    return {};
}

/* Returns the segment number at which the segment this track lies on
 * started. When interposer cuts are present (seg_dimension_cuts is non-empty),
 * the wire is split at the nearest cut between the natural start and seg_num,
 * so the returned start is on the same side of any cut as seg_num. */
int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num,
                  const std::vector<int>& seg_dimension_cuts) {
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

    // A cut at position c means positions <= c and positions > c are on different
    // sides of the interposer. If any cut lies between seg_start and seg_num
    // (i.e., seg_start <= c < seg_num), the wire must start after the cut.
    for (int c : seg_dimension_cuts) {
        if (c >= seg_start && c < seg_num) {
            seg_start = c + 1;
        }
    }

    return seg_start;
}

int get_seg_end(const t_chan_seg_details* seg_details, const int itrack, const int istart, const int chan_num, const int seg_max, const std::vector<int>& seg_dimension_cuts) {
    int seg_end;

    if (seg_details[itrack].longline()) {
        seg_end = seg_max;
    } else if (seg_details[itrack].seg_end() >= 0) {
        seg_end = seg_details[itrack].seg_end();
    } else {
        int len = seg_details[itrack].length();
        int ofs = seg_details[itrack].start();

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

    // A cut at position c means positions <= c and positions > c are on different
    // sides of the interposer. If any cut lies within the wire span [istart, seg_end)
    // (i.e., istart <= c < seg_end), the wire must end at the cut position.
    for (int c : seg_dimension_cuts) {
        if (c >= istart && c < seg_end) {
            seg_end = c;
        }
    }

    return seg_end;
}
