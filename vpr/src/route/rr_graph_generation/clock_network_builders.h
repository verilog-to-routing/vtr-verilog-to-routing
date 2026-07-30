#pragma once

#include <string>
#include <vector>

#include "device_grid.h"
#include "rr_graph_builder.h"
#include "rr_graph_clock.h"
#include "rr_graph_type.h"
#include "switchblock_types.h"

#include "vpr_types.h"

class t_rr_graph_storage;
class ClockRRGraphBuilder;

enum class ClockType {
    SPINE,
    RIB,
    H_TREE
};

struct MetalLayer {
    float r_metal = std::numeric_limits<float>::quiet_NaN();
    float c_metal = std::numeric_limits<float>::quiet_NaN();
};

struct Wire {
    MetalLayer layer;
    int start = UNDEFINED;
    int length = UNDEFINED;
    int position = UNDEFINED;
};

struct WireRepeat {
    int x = UNDEFINED;
    int y = UNDEFINED;
};

struct RibDrive {
    std::string name;
    int offset = UNDEFINED;
    int switch_idx = UNDEFINED;
};

struct RibTaps {
    std::string name;
    int offset = UNDEFINED;
    int increment = UNDEFINED;
};

struct SpineDrive {
    std::string name;
    int offset = UNDEFINED;
    int switch_idx = UNDEFINED;
};

struct SpineTaps {
    std::string name;
    int offset = UNDEFINED;
    int increment = UNDEFINED;
};

struct HtreeDrive {
    std::string name;
    t_physical_tile_loc offset;
    int switch_idx = UNDEFINED;
};

struct HtreeTaps {
    std::string name;
    t_physical_tile_loc offset;
    t_physical_tile_loc increment;
};

class ClockNetwork {
  protected:
    std::string clock_name_;
    int num_inst_ = UNDEFINED;

  public:
    /*
     * Destructor
     */
    virtual ~ClockNetwork() {}

    /*
     * Getters
     */
    int get_num_inst() const;
    std::string get_name() const;
    virtual ClockType get_network_type() const = 0;

    /*
     * Setters
     */
    void set_clock_name(std::string clock_name);
    void set_num_instance(int num_inst);

    /*
     * Member functions
     */
    /* Creates the RR nodes for the clock network wires and adds them to the reverse lookup
     * in ClockRRGraphBuilder. The reverse lookup maps the nodes to their switch point locations */
    void create_rr_nodes_for_clock_network_wires(ClockRRGraphBuilder& clock_graph,
                                                 t_rr_graph_storage* rr_nodes,
                                                 RRGraphBuilder& rr_graph_builder,
                                                 t_rr_edge_info_set* rr_edges_to_create,
                                                 int num_segments);
    virtual void create_segments(std::vector<t_segment_inf>& segment_inf) = 0;
    virtual void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     t_rr_graph_storage* rr_nodes,
                                                                     RRGraphBuilder& rr_graph_builder,
                                                                     t_rr_edge_info_set* rr_edges_to_create,
                                                                     int num_segments_x) = 0;
    virtual size_t estimate_additional_nodes(const DeviceGrid& grid) = 0;
    virtual void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) = 0;
};

class ClockRib : public ClockNetwork {
  private:
    // start and end x and position in the y
    Wire x_chan_wire_;
    WireRepeat repeat_;

    // offset in the x
    RibDrive drive_;

    // offset and incr in the x
    RibTaps tap_;

    // segment indices
    int right_seg_idx_ = UNDEFINED;
    int left_seg_idx_ = UNDEFINED;
    int drive_seg_idx_ = UNDEFINED;

  public:
    /** Constructor**/
    ClockRib() {} // default
    ClockRib(Wire wire1, WireRepeat repeat1, RibDrive drive1, RibTaps tap1)
        : x_chan_wire_(wire1)
        , repeat_(repeat1)
        , drive_(drive1)
        , tap_(tap1) {}
    /*
     * Getters
     */
    ClockType get_network_type() const override;

    /*
     * Setters
     */
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_initial_wire_location(int start_x, int end_x, int y);
    void set_wire_repeat(int repeat_x, int repeat_y);
    void set_drive_location(int offset_x);
    void set_drive_switch(int switch_idx);
    void set_drive_name(std::string name);
    void set_tap_locations(int offset_x, int increment_x);
    void set_tap_name(std::string name);

    /*
     * Member functions
     */
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;

    void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) override;

    int create_chanx_wire(int layer,
                          int x_start,
                          int x_end,
                          int y,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder);
    void record_tap_locations(unsigned x_start,
                              unsigned x_end,
                              unsigned y,
                              int left_rr_node_idx,
                              int right_rr_node_idx,
                              ClockRRGraphBuilder& clock_graph);
};

class ClockSpine : public ClockNetwork {
  private:
    // start and end y and position in the x
    Wire y_chan_wire;
    WireRepeat repeat;

    // offset in the y
    SpineDrive drive;

    // offset and incr in the y
    SpineTaps tap;

    // segment indices
    /* AA:Initially, after loading up these values in device setup, the indices will be relative to the **unified** segment_inf vector which
     * is carried in the device.Arch; The sole purpose of these indices is for calculating the cost index when allocating the drive, left, and 
     * right nodes for the network. We now use segment indices relative to the **parallel** vector of segments to setup the cost index, so these
     * will be remapped later in the map_relative_seg_indices.  */

    int right_seg_idx = UNDEFINED;
    int left_seg_idx = UNDEFINED;
    int drive_seg_idx = UNDEFINED;

  public:
    /*
     * Getters
     */
    ClockType get_network_type() const override;

    /*
     * Setters
     */
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_initial_wire_location(int start_y, int end_y, int x);
    void set_wire_repeat(int repeat_x, int repeat_y);
    void set_drive_location(int offset_y);
    void set_drive_switch(int switch_idx);
    void set_drive_name(std::string name);
    void set_tap_locations(int offset_y, int increment_y);
    void set_tap_name(std::string name);

    /*
     * Member functions
     */
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
    void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) override;
    int create_chany_wire(int layer,
                          int y_start,
                          int y_end,
                          int x,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder,
                          int num_segments);
    void record_tap_locations(unsigned y_start,
                              unsigned y_end,
                              unsigned x,
                              int left_node_idx,
                              int right_node_idx,
                              ClockRRGraphBuilder& clock_graph);
};

enum class SwitchGridPointType {
    DRIVE,
    TAP
};

// A single entry point/exit point into a ClockSwitchGrid, resolved to an absolute
// device grid location (unlike ClockRib/ClockSpine's along-the-wire offsets).
struct SwitchGridPoint {
    std::string name;
    SwitchGridPointType type = SwitchGridPointType::TAP;
    int x = UNDEFINED;
    int y = UNDEFINED;
    int switch_idx = UNDEFINED; // only meaningful for DRIVE points
};

// Models a grid of clock switch boxes: at every (repeat_x, repeat_y) spaced location,
// clock wires connect to their adjacent switch boxes' wires according to a configurable
// switch-block pattern (see switch_block_type_). Locations with a drive/tap switch_point
// additionally get a dedicated hub node with full-crossbar access to every wire incident
// to that switch box, since drive/tap points model dedicated clock-network access
// hardware rather than part of the general switching fabric.
//
// Hop wires may span more than one switch-box pitch (see length_hops_), in which case
// they only make wire-to-wire turns at their true endpoints; each track's wires are
// staggered by (track % length_hops_) pitches so that every switch box still has some
// tracks truly ending there, matching how alloc_and_load_seg_details staggers general
// routing segments. A switch_point that lands strictly between a wire's endpoints still
// gets tap access to that wire directly (see covering_wire_at in the .cpp), even though
// no turn is available there.
class ClockSwitchGrid : public ClockNetwork {
  private:
    MetalLayer layer_;
    int start_x_ = UNDEFINED;
    int start_y_ = UNDEFINED;
    WireRepeat repeat_;
    int chan_w_ = UNDEFINED;
    int internal_switch_idx_ = UNDEFINED;
    e_switch_block_type switch_block_type_ = e_switch_block_type::FULL;
    int length_hops_ = 1;

    // segment indices for the horizontal/vertical inter-switch-box wires
    int x_seg_idx_ = UNDEFINED;
    int y_seg_idx_ = UNDEFINED;

    std::vector<SwitchGridPoint> switch_points_;

  public:
    /*
     * Getters
     */
    ClockType get_network_type() const override;

    /*
     * Setters
     */
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_grid_start_location(int start_x, int start_y);
    void set_wire_repeat(int repeat_x, int repeat_y);
    void set_chan_width(int chan_w);
    void set_internal_switch(int switch_idx);
    void set_switch_block_type(e_switch_block_type switch_block_type);
    void set_length(int length_hops);
    void add_switch_point(std::string name, SwitchGridPointType type, int x, int y, int switch_idx = UNDEFINED);

    /*
     * Member functions
     */
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
    void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) override;

  private:
    int num_grid_locations(const DeviceGrid& grid) const;

    int create_chanx_node(int layer,
                          int x_start,
                          int x_end,
                          int y,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder);
    int create_chany_node(int layer,
                          int y_start,
                          int y_end,
                          int x,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder,
                          int num_segments_x);
};

class ClockHTree : private ClockNetwork {
  private:
    // position not needed since it changes with every root of the tree
    Wire x_chan_wire;
    Wire y_chan_wire;
    WireRepeat repeat;

    HtreeDrive drive;

    HtreeTaps tap;

  public:
    ClockType get_network_type() const override { return ClockType::H_TREE; }
    // TODO: Unimplemented member function
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
};
