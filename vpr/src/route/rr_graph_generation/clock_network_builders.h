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

/// @brief The topology of a dedicated clock network (see e_clock_type in clock_types.h,
/// which this mirrors after arch parsing resolves all formula strings to concrete values).
enum class ClockType {
    SPINE,
    RIB,
    H_TREE
};

/// @brief Metal layer electrical properties for a clock network's wires.
struct MetalLayer {
    float r_metal = std::numeric_limits<float>::quiet_NaN();
    float c_metal = std::numeric_limits<float>::quiet_NaN();
};

/// @brief The extent of a rib/spine's wire, in device grid coordinates.
struct Wire {
    MetalLayer layer;
    int start = UNDEFINED;    ///< Start coordinate along the wire's own axis.
    int length = UNDEFINED;   ///< Wire length, in tiles.
    int position = UNDEFINED; ///< Coordinate along the wire's perpendicular axis.
};

/// @brief How a rib/spine's wire repeats (tiles) across the device.
struct WireRepeat {
    int x = UNDEFINED;
    int y = UNDEFINED;

    /// @brief Upper bound (in device grid coordinates) for how far this repeat pattern
    /// tiles, in whichever axis is the network's own tiling direction: x_max for a
    /// spine's column-tiling (repeat.x), y_max for a rib's row-tiling (repeat.y).
    /// Sourced from the arch's optional endx (spine)/endy (rib) attribute; defaults to
    /// the device's full width/height, i.e. tile all the way to the device edge.
    int x_max = UNDEFINED;
    int y_max = UNDEFINED;
};

/// @brief Where and how a rib or spine is driven from another clock network. Shared by
/// ClockRib and ClockSpine, whose only difference is which axis "offset" runs along.
struct LinearClockDrive {
    std::string name;
    int offset = UNDEFINED;
    int switch_idx = UNDEFINED;
};

/// @brief Where and how often a rib or spine exposes tap points. Shared by ClockRib and
/// ClockSpine, whose only difference is which axis "offset"/"increment" run along.
struct LinearClockTaps {
    std::string name;
    int offset = UNDEFINED;
    int increment = UNDEFINED;
};

/// @brief Where and how an H-tree is driven. Unlike ClockRib/ClockSpine, offsets are 2D
/// since an H-tree's root moves with every instance of the tree.
struct HtreeDrive {
    std::string name;
    t_physical_tile_loc offset;
    int switch_idx = UNDEFINED;
};

/// @brief Where and how often an H-tree exposes tap points. See HtreeDrive.
struct HtreeTaps {
    std::string name;
    t_physical_tile_loc offset;
    t_physical_tile_loc increment;
};

/// @brief Base class for a dedicated clock network's RR graph generator. One concrete
/// subclass exists per e_clock_type: ClockRib, ClockSpine, ClockSwitchGrid, ClockHTree.
class ClockNetwork {
  protected:
    std::string clock_name_;
    int num_inst_ = UNDEFINED;

  public:
    virtual ~ClockNetwork() {}

    // Getters
    int get_num_inst() const;
    std::string get_name() const;
    virtual ClockType get_network_type() const = 0;

    // Setters
    void set_clock_name(std::string clock_name);
    void set_num_instance(int num_inst);

    // Member functions

    /// @brief Creates the RR nodes for every instance of this clock network's wires and
    /// adds them to the reverse lookup in ClockRRGraphBuilder, which maps the nodes to
    /// their switch point locations.
    void create_rr_nodes_for_clock_network_wires(ClockRRGraphBuilder& clock_graph,
                                                 t_rr_graph_storage* rr_nodes,
                                                 RRGraphBuilder& rr_graph_builder,
                                                 t_rr_edge_info_set* rr_edges_to_create,
                                                 int num_segments);

    /// @brief Appends this network's segment(s) to segment_inf and records their
    /// (unified-space) indices, for later use by create_rr_nodes_and_internal_edges_for_one_instance
    /// and map_relative_seg_indices.
    virtual void create_segments(std::vector<t_segment_inf>& segment_inf) = 0;

    /// @brief Creates the RR nodes and edges for a single instance of this clock network.
    virtual void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                                     t_rr_graph_storage* rr_nodes,
                                                                     RRGraphBuilder& rr_graph_builder,
                                                                     t_rr_edge_info_set* rr_edges_to_create,
                                                                     int num_segments_x) = 0;

    /// @brief Upper-bound estimate of the number of RR nodes this network will add, used
    /// to reserve storage up front.
    virtual size_t estimate_additional_nodes(const DeviceGrid& grid) = 0;

    /// @brief Remaps this network's segment indices (set by create_segments, relative to
    /// the unified segment_inf vector) to their equivalent indices in the axis-specific
    /// segment vectors build_rr_graph splits segment_inf into. Must run after
    /// create_segments and before create_rr_nodes_and_internal_edges_for_one_instance, since
    /// the cost index those RR nodes are tagged with depends on the axis-specific indices.
    virtual void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) = 0;
};

/// @brief A single horizontal clock wire (see ClockType::RIB), driven at one point along
/// its length and reachable from other clock networks/routing at tap points along it.
class ClockRib : public ClockNetwork {
  private:
    Wire x_chan_wire_;
    WireRepeat repeat_;
    LinearClockDrive drive_;
    LinearClockTaps tap_;

    /// @brief Segment indices, relative to the **parallel** (axis-specific) segment
    /// vector. Initially (as set by create_segments) these are relative to the
    /// **unified** segment_inf vector instead; map_relative_seg_indices remaps them once
    /// build_rr_graph has split segment_inf into its per-axis vectors. That remap always
    /// reads from *_seg_idx_unified_ (set once, in create_segments, and never modified
    /// again) rather than from these fields themselves, since build_rr_graph, and
    /// therefore map_relative_seg_indices, can run more than once per VPR invocation
    /// (e.g. once for the placement delay model, once for the real routing resource
    /// graph) on this same long-lived object; remapping an already-remapped parallel
    /// index as though it were still a unified one would silently corrupt it.
    int right_seg_idx_ = UNDEFINED;
    int left_seg_idx_ = UNDEFINED;
    int drive_seg_idx_ = UNDEFINED;
    int right_seg_idx_unified_ = UNDEFINED;
    int left_seg_idx_unified_ = UNDEFINED;
    int drive_seg_idx_unified_ = UNDEFINED;

  public:
    // Getters
    ClockType get_network_type() const override;

    // Setters
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_initial_wire_location(int start_x, int end_x, int y);
    void set_wire_repeat(int repeat_x, int repeat_y, int repeat_y_max);
    void set_drive_location(int offset_x);
    void set_drive_switch(int switch_idx);
    void set_drive_name(std::string name);
    void set_tap_locations(int offset_x, int increment_x);
    void set_tap_name(std::string name);

    // Member functions
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
    void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) override;

    /// @brief Creates a single CHANX RR node spanning [x_start, x_end] at row y, tagged
    /// with the segment matching direction (drive/left/right; see the seg idx fields
    /// above), and adds it to the spatial lookup.
    int create_chanx_wire(int layer,
                          int x_start,
                          int x_end,
                          int y,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder);

    /// @brief Registers this rib's tap point(s); at tap_.offset, repeating every
    /// tap_.increment tiles; in clock_graph's reverse lookup, pointing each tap at
    /// whichever of drive/left/right node it falls on.
    void record_tap_locations(unsigned x_start,
                              unsigned x_end,
                              unsigned drive_x,
                              unsigned y,
                              int drive_node_idx,
                              int left_rr_node_idx,
                              int right_rr_node_idx,
                              ClockRRGraphBuilder& clock_graph);
};

/// @brief A single vertical clock wire (see ClockType::SPINE); the column-wise
/// counterpart of ClockRib.
class ClockSpine : public ClockNetwork {
  private:
    Wire y_chan_wire_;
    WireRepeat repeat_;
    LinearClockDrive drive_;
    LinearClockTaps tap_;

    // Segment indices; see the longer comment on ClockRib's equivalent fields.
    int right_seg_idx_ = UNDEFINED;
    int left_seg_idx_ = UNDEFINED;
    int drive_seg_idx_ = UNDEFINED;
    int right_seg_idx_unified_ = UNDEFINED;
    int left_seg_idx_unified_ = UNDEFINED;
    int drive_seg_idx_unified_ = UNDEFINED;

  public:
    // Getters
    ClockType get_network_type() const override;

    // Setters
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_initial_wire_location(int start_y, int end_y, int x);
    void set_wire_repeat(int repeat_x, int repeat_y, int repeat_x_max);
    void set_drive_location(int offset_y);
    void set_drive_switch(int switch_idx);
    void set_drive_name(std::string name);
    void set_tap_locations(int offset_y, int increment_y);
    void set_tap_name(std::string name);

    // Member functions
    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
    void map_relative_seg_indices(const t_unified_to_parallel_seg_index& index_map) override;

    /// @brief Creates a single CHANY RR node spanning [y_start, y_end] at column x. See
    /// ClockRib::create_chanx_wire.
    int create_chany_wire(int layer,
                          int y_start,
                          int y_end,
                          int x,
                          int ptc_num,
                          Direction direction,
                          t_rr_graph_storage* rr_nodes,
                          RRGraphBuilder& rr_graph_builder,
                          int num_segments);

    /// @brief Registers this spine's tap point(s) in clock_graph's reverse lookup. See
    /// ClockRib::record_tap_locations.
    void record_tap_locations(unsigned y_start,
                              unsigned y_end,
                              unsigned drive_y,
                              unsigned x,
                              int drive_node_idx,
                              int left_node_idx,
                              int right_node_idx,
                              ClockRRGraphBuilder& clock_graph);
};

/// @brief Whether a ClockSwitchGrid <switch_point> is a drive or a tap point.
enum class SwitchGridPointType {
    DRIVE,
    TAP
};

/// @brief A single entry point/exit point into a ClockSwitchGrid, resolved to an absolute
/// device grid location (unlike ClockRib/ClockSpine's along-the-wire offsets).
struct SwitchGridPoint {
    std::string name;
    SwitchGridPointType type = SwitchGridPointType::TAP;
    int x = UNDEFINED;
    int y = UNDEFINED;
    int switch_idx = UNDEFINED; ///< Only meaningful for DRIVE points.
};

/// @brief A single <switch_pattern> entry: which built-in switch-block permutation type
/// applies, and where (matched against ClockSwitchGrid::switch_patterns_ in list order,
/// first match wins). Only used when switch_block_type_ == CUSTOM.
struct ClockSwitchPattern {
    e_switch_block_type switch_block_type = e_switch_block_type::FULL;
    e_sb_location location = e_sb_location::E_EVERYWHERE;
    t_specified_loc specified_loc;     ///< Only meaningful when location == E_XY_SPECIFIED.
    t_permutation_map permutation_map; ///< Only meaningful when switch_block_type == CUSTOM.
};

/// @brief Models a grid of clock switch boxes (see ClockType::SPINE; reused as a
/// placeholder network type, see get_network_type): at every (repeat_x, repeat_y) spaced
/// location, clock wires connect to their adjacent switch boxes' wires according to a
/// configurable switch-block pattern (see switch_block_type_). Locations with a
/// drive/tap switch_point additionally get a dedicated hub node with full-crossbar access
/// to every wire incident to that switch box, since drive/tap points model dedicated
/// clock-network access hardware rather than part of the general switching fabric.
///
/// Hop wires may span more than one switch-box pitch (see length_hops_), in which case
/// they only make wire-to-wire turns at their true endpoints; each track's wires are
/// staggered by (track % length_hops_) pitches so that every switch box still has some
/// tracks truly ending there, matching how alloc_and_load_seg_details staggers general
/// routing segments. A switch_point that lands strictly between a wire's endpoints still
/// gets tap access to that wire directly (see covering_wire_at in the .cpp), even though
/// no turn is available there.
class ClockSwitchGrid : public ClockNetwork {
  private:
    MetalLayer layer_;
    int start_x_ = UNDEFINED;
    int start_y_ = UNDEFINED;
    WireRepeat repeat_;
    int chan_w_ = UNDEFINED;
    int internal_switch_idx_ = UNDEFINED;
    e_switch_block_type switch_block_type_ = e_switch_block_type::FULL;
    std::vector<ClockSwitchPattern> switch_patterns_; ///< Only populated when switch_block_type_ == CUSTOM (see add_switch_pattern).
    int length_hops_ = 1;
    e_directionality directionality_ = BI_DIRECTIONAL;

    /// @brief Segment indices for the horizontal/vertical inter-switch-box wires. See
    /// the longer comment on ClockRib's equivalent fields.
    int x_seg_idx_ = UNDEFINED;
    int y_seg_idx_ = UNDEFINED;
    int x_seg_idx_unified_ = UNDEFINED;
    int y_seg_idx_unified_ = UNDEFINED;

    std::vector<SwitchGridPoint> switch_points_;

  public:
    // Getters
    ClockType get_network_type() const override;

    // Setters
    void set_metal_layer(float r_metal, float c_metal);
    void set_metal_layer(MetalLayer metal_layer);
    void set_grid_start_location(int start_x, int start_y);
    void set_wire_repeat(int repeat_x, int repeat_y);
    void set_chan_width(int chan_w);
    void set_internal_switch(int switch_idx);
    void set_switch_block_type(e_switch_block_type switch_block_type);
    void add_switch_pattern(ClockSwitchPattern pattern);
    void set_length(int length_hops);
    void set_directionality(e_directionality directionality);
    void add_switch_point(std::string name, SwitchGridPointType type, int x, int y, int switch_idx = UNDEFINED);

    // Member functions
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

/// @brief An H-tree clock network (see ClockType::H_TREE).
/// @note Not yet implemented; every member function fatal-errors. Kept as a stub so the
/// arch parser and ClockType enum already have a place for it once it is implemented.
class ClockHTree : public ClockNetwork {
  private:
    Wire x_chan_wire_; ///< Position not needed since it changes with every root of the tree.
    Wire y_chan_wire_;
    WireRepeat repeat_;
    HtreeDrive drive_;
    HtreeTaps tap_;

  public:
    ClockType get_network_type() const override { return ClockType::H_TREE; }

    void create_segments(std::vector<t_segment_inf>& segment_inf) override;
    void create_rr_nodes_and_internal_edges_for_one_instance(ClockRRGraphBuilder& clock_graph,
                                                             t_rr_graph_storage* rr_nodes,
                                                             RRGraphBuilder& rr_graph_builder,
                                                             t_rr_edge_info_set* rr_edges_to_create,
                                                             int num_segments_x) override;
    size_t estimate_additional_nodes(const DeviceGrid& grid) override;
};
