#include "vpr_error.h"
#include "rr_graph_reader.h"
#include "rr_graph_writer.h"
#include "arch_util.h"
#include <set>

struct t_seg_details {
    int length = 0;
    int start = 0;
    bool longline = 0;
    std::unique_ptr<bool[]> sb;
    std::unique_ptr<bool[]> cb;
    short arch_wire_switch = 0;
    short arch_opin_switch = 0;
    float Rmetal = 0;
    float Cmetal = 0;
    bool twisted = 0;
    enum Direction direction = Direction::NONE;
    int group_start = 0;
    int group_size = 0;
    int seg_start = 0;
    int seg_end = 0;
    int index = 0;
    int abs_index = 0;
    float Cmetal_per_m = 0; ///<Used for power
    std::string type_name;
};

class t_chan_seg_details {
  public:
    t_chan_seg_details() = default;
    t_chan_seg_details(const t_seg_details* init_seg_details)
        : length_(init_seg_details->length)
        , seg_detail_(init_seg_details) {}

  public:
    int length() const { return length_; }
    int seg_start() const { return seg_start_; }
    int seg_end() const { return seg_end_; }

    int start() const { return seg_detail_->start; }
    bool longline() const { return seg_detail_->longline; }

    int group_start() const { return seg_detail_->group_start; }
    int group_size() const { return seg_detail_->group_size; }

    bool cb(int pos) const { return seg_detail_->cb[pos]; }
    bool sb(int pos) const { return seg_detail_->sb[pos]; }

    float Rmetal() const { return seg_detail_->Rmetal; }
    float Cmetal() const { return seg_detail_->Cmetal; }
    float Cmetal_per_m() const { return seg_detail_->Cmetal_per_m; }

    short arch_wire_switch() const { return seg_detail_->arch_wire_switch; }
    short arch_opin_switch() const { return seg_detail_->arch_opin_switch; }

    Direction direction() const { return seg_detail_->direction; }

    int index() const { return seg_detail_->index; }
    int abs_index() const { return seg_detail_->abs_index; }

    const vtr::string_view type_name() const {
        return vtr::string_view(
            seg_detail_->type_name.data(),
            seg_detail_->type_name.size());
    }

  public: //Modifiers
    void set_length(int new_len) { length_ = new_len; }
    void set_seg_start(int new_start) { seg_start_ = new_start; }
    void set_seg_end(int new_end) { seg_end_ = new_end; }

  private:
    //The only unique information about a channel segment is it's start/end
    //and length.  All other information is shared accross segment types,
    //so we use a flyweight to the t_seg_details which defines that info.
    //
    //To preserve the illusion of uniqueness we wrap all t_seg_details members
    //so it appears transparent -- client code of this class doesn't need to
    //know about t_seg_details.
    int length_ = -1;
    int seg_start_ = -1;
    int seg_end_ = -1;
    const t_seg_details* seg_detail_ = nullptr;
};

typedef vtr::NdMatrix<t_chan_seg_details, 3> t_chan_details;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static t_seg_details* alloc_and_load_seg_details(int* max_chan_width,
                                          const int max_len,
                                          const std::vector<t_segment_inf>& segment_inf,
                                          const bool use_full_seg_groups,
                                          const enum e_directionality directionality,
                                          int* num_seg_details);

static std::unique_ptr<int[]> get_seg_track_counts(const int num_sets,
                                            const std::vector<t_segment_inf>& segment_inf,
                                            const bool use_full_seg_groups);

static t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 const enum e_parallel_axis seg_parallel_axis);

static void adjust_chan_details(const DeviceGrid& grid,
                         const t_chan_width* nodes_per_chan,
                         t_chan_details& chan_details_x,
                         t_chan_details& chan_details_y);

static void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width* nodes_per_chan,
                        t_chan_details& chan_details,
                        const enum e_parallel_axis seg_parallel_axis);

static int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num);

static int get_seg_end(const t_chan_seg_details* seg_details, const int itrack, const int istart, const int chan_num, const int seg_max);

static void load_rr_switch_from_arch_switch(RRGraphBuilder* rr_graph_builder,
                                     t_arch_switch_inf* arch_switch_inf,
                                     int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos);

static t_seg_details* alloc_and_load_global_route_seg_details(const int global_route_switch,
                                                              int* num_seg_details);

static void set_grid_block_type(int priority,
                                const t_physical_tile_type* type,
                                size_t x_root, size_t y_root,
                                vtr::Matrix<t_grid_tile>& grid,
                                vtr::Matrix<int>& grid_priorities,
                                const t_metadata_dict* meta);

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

t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 const enum e_parallel_axis seg_parallel_axis) {
    if (seg_parallel_axis == X_AXIS) {
        VTR_LOG("num_seg_details: %d\n", num_seg_details);
        VTR_LOG("nodes_per_chan: %d\n", nodes_per_chan->x_max);
        VTR_ASSERT(num_seg_details <= nodes_per_chan->x_max);
    } else if (seg_parallel_axis == Y_AXIS) {
        VTR_ASSERT(num_seg_details <= nodes_per_chan->y_max);
    }
    VTR_LOG("chanx1\n");
    VTR_LOG("grid dim: %dx%d\n", grid.width(), grid.height());
    t_chan_details chan_details({grid.width(), grid.height(), size_t(num_seg_details)});
    VTR_LOG("chan_details: %d\n", chan_details.size());
    VTR_LOG("chanx2\n");
    for (size_t x = 0; x < grid.width(); ++x) {
        VTR_LOG("chanx2.1\n");
        for (size_t y = 0; y < grid.height(); ++y) {
            t_chan_seg_details* p_seg_details = chan_details[y][x].data();
            VTR_LOG("chanx2.3\n");
            for (int i = 0; i < num_seg_details; ++i) {
                p_seg_details[i] = t_chan_seg_details(&(seg_details[i]));

                int seg_start = -1;
                int seg_end = -1;

                if (seg_parallel_axis == X_AXIS) {
                    VTR_LOG("chanx3.2\n");
                    seg_start = (p_seg_details, i, y, x);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, y, grid.width() - 2); //-2 for no perim channels
                } else if (seg_parallel_axis == Y_AXIS) {
                    VTR_LOG("chanx3.3\n");
                    seg_start = (p_seg_details, i, x, y);
                    seg_end = get_seg_end(p_seg_details, i, seg_start, x, grid.height() - 2); //-2 for no perim channels
                }
                VTR_LOG("chanx4\n");
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
                VTR_LOG("chanx5\n");
            }
        }
    }
    VTR_LOG("chanx6\n");
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

/* Returns the segment number at which the segment this track lies on        *
 * started.    
                                                              */
static int get_seg_start(const t_chan_seg_details* seg_details,
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

void load_rr_switch_from_arch_switch(RRGraphBuilder* rr_graph_builder,
                                     t_arch_switch_inf* arch_switch_inf,
                                     int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos) {

    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = arch_switch_inf[arch_switch_idx].Tdel(fanin);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].set_type(arch_switch_inf[arch_switch_idx].type());
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].R = arch_switch_inf[arch_switch_idx].R;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].Cin = arch_switch_inf[arch_switch_idx].Cin;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].Cinternal = arch_switch_inf[arch_switch_idx].Cinternal;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].Cout = arch_switch_inf[arch_switch_idx].Cout;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].Tdel = rr_switch_Tdel;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].mux_trans_size = arch_switch_inf[arch_switch_idx].mux_trans_size;

    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].buf_size = arch_switch_inf[arch_switch_idx].buf_size;

    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].name = arch_switch_inf[arch_switch_idx].name;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_type = arch_switch_inf[arch_switch_idx].power_buffer_type;
    rr_graph_builder->rr_switch()[RRSwitchId(rr_switch_idx)].power_buffer_size = arch_switch_inf[arch_switch_idx].power_buffer_size;
}

static void set_grid_block_type(int priority, const t_physical_tile_type* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<int>& grid_priorities, const t_metadata_dict* meta) {
    t_physical_tile_type empty_type = get_empty_physical_type();
    t_physical_tile_type_ptr EMPTY_PHYSICAL_TILE_TYPE = &empty_type;
    
    struct TypeLocation {
        TypeLocation(size_t x_val, size_t y_val, const t_physical_tile_type* type_val, int priority_val)
            : x(x_val)
            , y(y_val)
            , type(type_val)
            , priority(priority_val) {}
        size_t x;
        size_t y;
        const t_physical_tile_type* type;
        int priority;

        bool operator<(const TypeLocation& rhs) const {
            return x < rhs.x || y < rhs.y || type < rhs.type;
        }
    };

    //Collect locations effected by this block
    std::set<TypeLocation> target_locations;
    for (size_t x = x_root; x < x_root + type->width; ++x) {
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            target_locations.insert(TypeLocation(x, y, grid[x][y].type, grid_priorities[x][y]));
        }
    }

    //Record the highest priority of all effected locations
    auto iter = target_locations.begin();
    TypeLocation max_priority_type_loc = *iter;
    for (; iter != target_locations.end(); ++iter) {
        if (iter->priority > max_priority_type_loc.priority) {
            max_priority_type_loc = *iter;
        }
    }

    if (priority < max_priority_type_loc.priority) {
        //Lower priority, do not override
#ifdef VERBOSE
        VTR_LOG("Not creating block '%s' at (%zu,%zu) since overlaps block '%s' at (%zu,%zu) with higher priority (%d > %d)\n",
                type->name, x_root, y_root, max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
                max_priority_type_loc.priority, priority);
#endif
        return;
    }

    if (priority == max_priority_type_loc.priority) {
        //Ambiguous case where current grid block and new specification have equal priority
        //
        //We arbitrarily decide to take the 'last applied' wins approach, and warn the user
        //about the potential ambiguity
        VTR_LOG_WARN(
            "Ambiguous block type specification at grid location (%zu,%zu)."
            " Existing block type '%s' at (%zu,%zu) has the same priority (%d) as new overlapping type '%s'."
            " The last specification will apply.\n",
            x_root, y_root,
            max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
            priority, type->name);
    }

    //Mark all the grid tiles 'covered' by this block with the appropriate type
    //and width/height offsets
    std::set<TypeLocation> root_blocks_to_rip_up;

    for (size_t x = x_root; x < x_root + type->width; ++x) {
        VTR_ASSERT(x < grid.end_index(0));

        size_t x_offset = x - x_root;
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            VTR_ASSERT(y < grid.end_index(1));
            size_t y_offset = y - y_root;

            auto& grid_tile = grid[x][y];
            VTR_ASSERT(grid_priorities[x][y] <= priority);

            if (grid_tile.type != nullptr
                && grid_tile.type != EMPTY_PHYSICAL_TILE_TYPE) {
                //We are overriding a non-empty block, we need to be careful
                //to ensure we remove any blocks which will be invalidated when we
                //overwrite part of their locations

                size_t orig_root_x = x - grid[x][y].width_offset;
                size_t orig_root_y = y - grid[x][y].height_offset;

                root_blocks_to_rip_up.insert(TypeLocation(orig_root_x, orig_root_y, grid[x][y].type, grid_priorities[x][y]));
            }

            grid[x][y].type = type;
            grid[x][y].width_offset = x_offset;
            grid[x][y].height_offset = y_offset;
            grid[x][y].meta = meta;

            grid_priorities[x][y] = priority;
        }
    }

    //Rip-up any invalidated blocks
    for (auto invalidated_root : root_blocks_to_rip_up) {
        //Mark all the grid locations used by this root block as empty
        for (size_t x = invalidated_root.x; x < invalidated_root.x + invalidated_root.type->width; ++x) {
            int x_offset = x - invalidated_root.x;
            for (size_t y = invalidated_root.y; y < invalidated_root.y + invalidated_root.type->height; ++y) {
                int y_offset = y - invalidated_root.y;

                if (grid[x][y].type == invalidated_root.type
                    && grid[x][y].width_offset == x_offset
                    && grid[x][y].height_offset == y_offset) {
                    //This is a left-over invalidated block, mark as empty
                    // Note: that we explicitly check the type and offsets, since the original block
                    //       may have been completely overwritten, and we don't want to change anything
                    //       in that case
                    VTR_ASSERT(EMPTY_PHYSICAL_TILE_TYPE->width == 1);
                    VTR_ASSERT(EMPTY_PHYSICAL_TILE_TYPE->height == 1);

#ifdef VERBOSE
                    VTR_LOG("Ripping up block '%s' at (%d,%d) offset (%d,%d). Overlapped by '%s' at (%d,%d)\n",
                            invalidated_root.type->name, invalidated_root.x, invalidated_root.y,
                            x_offset, y_offset,
                            type->name, x_root, y_root);
#endif

                    grid[x][y].type = EMPTY_PHYSICAL_TILE_TYPE;
                    grid[x][y].width_offset = 0;
                    grid[x][y].height_offset = 0;

                    grid_priorities[x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
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