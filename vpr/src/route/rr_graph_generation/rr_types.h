#pragma once

#include <vector>
#include <string_view>

#include "rr_node_types.h"
#include "vtr_ndmatrix.h"

/* AA: This structure stores the track connections for each physical pin. Note that num_pins refers to the # of logical pins for a tile and 
 * we use the relative x and y location (0...width and 0...height of the tile) and the side of that unit tile to locate the physical pin. 
 * If pinloc[ipin][iwidth][iheight][side]==1 it exists there... 
 * The alloc_and_load_pin_to_track_map loads up the tracks that connect to each of the *PHYSICAL* pins. Thus, the last dimension of the matrix
 * goes from [0...Fc-1] where Fc is the actual Fc value for that pin. 
 *
 * The matrix should be accessed as follows as a result after allocation in rr_graph.cpp: alloc_pin_to_track_lookup (used by unidir and bidir)
 * [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width][0..height][0..layer-1][0..3][0..Fc-1] */
typedef std::vector<vtr::NdMatrix<std::vector<int>, 5>> t_pin_to_track_lookup;

/* AA: t_pin_to_track_lookup is alloacted first and is then converted to t_track_to_pin lookup by simply redefining the accessing order.
 * As a result, the matrix should be accessed as follow as a result after allocation in rr_graph.cpp: alloc_track_to_pin_lookup (used by unidir and bidir)
 * [0..device_ctx.physical_tile_types.size()-1][0..max_chan_width-1][0..width][0..height][0..layer-1][0..3]
 * 
 * Note that when we model different channels based on position not axis, we can't use this anymore and need to have a lookup for each grid location. */
typedef std::vector<vtr::NdMatrix<std::vector<int>, 5>> t_track_to_pin_lookup;

/**
 * @brief Lists detailed information about wire segments.  [0 .. W-1].
 */
struct t_seg_details {
    /** @brief Length (in clbs) of the segment. */
    int length = 0;

    /** @brief Index at which a segment starts in channel 0. */
    int start = 0;

    /** @brief True if this segment spans the entire channel. */
    bool longline = false;

    /** @brief [0..length]: true for every channel intersection, relative to the
     *  segment start, at which there is a switch box.
     */
    std::unique_ptr<bool[]> sb;

    /** @brief [0..length-1]: true for every logic block along the segment at
     *  which there is a connection box.
     */
    std::unique_ptr<bool[]> cb;

    /** @brief Index of the switch type that connects other wires to this segment.
     *  Note that this index is in relation to the switches from the architecture
     *  file, not the expanded list of switches that is built at the end of build_rr_graph.
     */
    short arch_wire_switch = 0;

    /** @brief Index of the switch type that connects output pins (OPINs) *to* this segment.
     *  Note that this index is in relation to the switches from the architecture
     *  file, not the expanded list of switches that is built at the end of build_rr_graph.
     */
    short arch_opin_switch = 0;

    /** @brief Index of the switch type that connects output pins (OPINs) *to* this segment
     *  from *another dice*. Note that this index is in relation to the switches from the
     *  architecture file, not the expanded list of switches that is built at the end of
     *  build_rr_graph.
     */
    short arch_inter_die_switch = 0;

    /** @brief Resistance of a routing track, per unit logic block length. */
    float Rmetal = 0;

    /** @brief Capacitance of a routing track, per unit logic block length. */
    float Cmetal = 0;

    /** @brief Whether the segment is twisted. */
    bool twisted = false;

    /** @brief Direction of the segment. */
    Direction direction = Direction::NONE;

    /** @brief Index of the first logic block in the group. */
    int group_start = 0;

    /** @brief Size of the group. */
    int group_size = 0;

    /** @brief index of the segment type used for this track.
     *  Note that this index will store the index of the segment
     *  relative to its **parallel** segment types, not all segments
     *  as stored in device_ctx. Look in rr_graph.cpp: build_rr_graph
     *  for details but here is an example: say our segment_inf_vec in
     *  device_ctx is as follows: [seg_a_x, seg_b_x, seg_a_y, seg_b_y]
     *  when building the rr_graph, static segment_inf_vectors will be
     *  created for each direction, thus you will have the following
     *  2 vectors: X_vec =[seg_a_x,seg_b_x] and Y_vec = [seg_a_y,seg_b_y].
     *  As a result, e.g. seg_b_y::index == 1 (index in Y_vec)
     *  and != 3 (index in device_ctx segment_inf_vec).
     */
    int index = 0;

    /** @brief index is relative to the segment_inf vec as stored in device_ctx.
     *  Note that the above vector is **unifies** both x-parallel and
     *  y-parallel segments and is loaded up originally in read_xml_arch_file.cpp
     */
    int abs_index = 0;

    /** @brief Used for power */
    float Cmetal_per_m = 0;

    /** @brief Name of the segment type. */
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
    short arch_inter_die_switch() const { return seg_detail_->arch_inter_die_switch; }

    Direction direction() const { return seg_detail_->direction; }

    int index() const { return seg_detail_->index; }
    int abs_index() const { return seg_detail_->abs_index; }

    const std::string_view type_name() const {
        return seg_detail_->type_name;
    }

  public: //Modifiers
    void set_length(int new_len) { length_ = new_len; }
    void set_seg_start(int new_start) { seg_start_ = new_start; }
    void set_seg_end(int new_end) { seg_end_ = new_end; }

  private:
    // The only unique information about a channel segment is its start/end
    // and length.  All other information is shared across segment types,
    // so we use a flyweight to the t_seg_details which defines that info.
    // To preserve the illusion of uniqueness we wrap all t_seg_details members
    // so it appears transparent -- client code of this class doesn't need to
    // know about t_seg_details.
    int length_ = -1;
    int seg_start_ = -1;
    int seg_end_ = -1;
    const t_seg_details* seg_detail_ = nullptr;
};

/**
 * @typedef t_chan_details
 * @brief Defines a 3-D array of t_chan_seg_details structures (one for each horizontal and vertical channel).
 *
 * Once allocated in rr_graph2.cpp, it can be accessed as:
 * [0..grid.width()][0..grid.height()][0..num_tracks-1]
 */
typedef vtr::NdMatrix<t_chan_seg_details, 3> t_chan_details;

/* [0..grid.width()-1][0..grid.width()][0..3 (From side)] \
 * [0..3 (To side)][0...max_chan_width][0..3 (to_mux,to_trac,alt_mux,alt_track)]
 * originally initialized to UN_SET until alloc_and_load_sb is called */
typedef vtr::NdMatrix<short, 6> t_sblock_pattern;
