#pragma once

#include "crr_common.h"

#include "rr_node_types.h"
namespace crrgenerator {

/**
 * @brief Represents a routing resource node in the FPGA
 */
class RRNode {
  public:
    RRNode() = default;
    RRNode(NodeId id, e_rr_type type, int capacity, const std::string& direction, const Location& location, const NodeTiming& timing, const NodeSegmentId& segment_id)
        : id_(id)
        , type_(type)
        , capacity_(capacity)
        , direction_(direction)
        , location_(location)
        , timing_(timing)
        , segment_id_(segment_id) {}

    // Getters
    NodeId get_id() const { return id_; }
    e_rr_type get_type() const { return type_; }
    int get_capacity() const { return capacity_; }
    const std::string& get_direction() const { return direction_; }
    const Location& get_location() const { return location_; }
    const std::string& get_ptc() const { return location_.ptc; }
    const NodeTiming& get_timing() const { return timing_; }
    const NodeSegmentId& get_segment() const { return segment_id_; }

    // Setters
    void set_id(NodeId id) { id_ = id; }
    void set_type(e_rr_type type) { type_ = type; }
    void set_capacity(int capacity) { capacity_ = capacity; }
    void set_direction(const std::string& direction) { direction_ = direction; }
    void set_location(const Location& location) { location_ = location; }
    void set_timing(const NodeTiming& timing) { timing_ = timing; }
    void set_segment(const NodeSegmentId& segment_id) { segment_id_ = segment_id; }

    // Utility methods
    // NodeHash get_hash() const {
    //     return std::make_tuple(type_, location_.ptc, location_.x_low,
    //                            location_.x_high, location_.y_low, location_.y_high);
    // }

    bool is_single_coordinate() const {
        return (location_.x_low == location_.x_high) && (location_.y_low == location_.y_high);
    }

    bool operator<(const RRNode& other) const {
        return id_ < other.id_;
    }

    bool operator==(const RRNode& other) const {
        return id_ == other.id_;
    }

  private:
    NodeId id_{0};
    e_rr_type type_{e_rr_type::NUM_RR_TYPES};
    int capacity_{-1};
    std::string direction_;
    Location location_;
    NodeTiming timing_;
    NodeSegmentId segment_id_;
};

/**
 * @brief Represents a routing edge connecting two nodes
 */
class RREdge {
  public:
    RREdge() = default;
    RREdge(NodeId src_node, NodeId sink_node, SwitchId switch_id)
        : src_node_(src_node)
        , sink_node_(sink_node)
        , switch_id_(switch_id) {}

    // Getters
    NodeId get_src_node() const { return src_node_; }
    NodeId get_sink_node() const { return sink_node_; }
    SwitchId get_switch_id() const { return switch_id_; }

    // Setters
    void set_src_node(NodeId node) { src_node_ = node; }
    void set_sink_node(NodeId node) { sink_node_ = node; }
    void set_switch_id(SwitchId id) { switch_id_ = id; }

    bool operator<(const RREdge& other) const {
        return std::tie(src_node_, sink_node_, switch_id_) < std::tie(other.src_node_, other.sink_node_, other.switch_id_);
    }

    bool operator==(const RREdge& other) const {
        return src_node_ == other.src_node_ && sink_node_ == other.sink_node_ && switch_id_ == other.switch_id_;
    }

  private:
    NodeId src_node_{0};
    NodeId sink_node_{0};
    SwitchId switch_id_{0};
};

/**
 * @brief Represents a list of X or Y coordinates
 */
class XYList {
  public:
    enum class Type { X_LIST,
                      Y_LIST,
                      INVALID_LIST };

    XYList() = default;
    XYList(Type type, int index, int info)
        : type_(type)
        , index_(index)
        , info_(info) {}

    // Getters
    Type get_type() const { return type_; }
    int get_index() const { return index_; }
    int get_info() const { return info_; }

    // Setters
    void set_type(Type type) { type_ = type; }
    void set_index(int index) { index_ = index; }
    void set_info(int info) { info_ = info; }

  private:
    Type type_{Type::INVALID_LIST};
    int index_{-1};
    int info_{-1};
};

/**
 * @brief Represents a channel in the FPGA
 */
class Channel {
  public:
    Channel() = default;
    Channel(int max_width, int x_max, int x_min, int y_max, int y_min)
        : max_width_(max_width)
        , x_max_(x_max)
        , x_min_(x_min)
        , y_max_(y_max)
        , y_min_(y_min) {}

    // Getters
    int get_max_width() const { return max_width_; }
    int get_x_max() const { return x_max_; }
    int get_x_min() const { return x_min_; }
    int get_y_max() const { return y_max_; }
    int get_y_min() const { return y_min_; }

    // Setters
    void set_max_width(int max_width) { max_width_ = max_width; }
    void set_x_max(int x_max) { x_max_ = x_max; }
    void set_x_min(int x_min) { x_min_ = x_min; }
    void set_y_max(int y_max) { y_max_ = y_max; }
    void set_y_min(int y_min) { y_min_ = y_min; }

  private:
    int max_width_{-1};
    int x_max_{-1};
    int x_min_{-1};
    int y_max_{-1};
    int y_min_{-1};
};

/**
 * @brief Represents a pin in the FPGA
 */
class Pin {
  public:
    Pin() = default;
    Pin(const int ptc, const std::string& value)
        : ptc_(ptc)
        , value_(value) {}

    // Getters
    int get_ptc() const { return ptc_; }
    std::string get_value() const { return value_; }

    // Setters
    void set_ptc(const int ptc) { ptc_ = ptc; }
    void set_value(const std::string& value) { value_ = value; }

  private:
    int ptc_;
    std::string value_;
};

/**
 * @brief Represents a pin class in the FPGA
 */
class PinClass {
  public:
    PinClass() = default;
    PinClass(const std::string& type, const std::vector<Pin>& pins)
        : type_(type)
        , pins_(pins) {}

    // Getters
    const std::string& get_type() const { return type_; }
    const std::vector<Pin>& get_pins() const { return pins_; }

    // Setters
    void set_type(const std::string& type) { type_ = type; }
    void set_pins(const std::vector<Pin>& pins) { pins_ = pins; }

  private:
    std::string type_;
    std::vector<Pin> pins_;
};

/**
 * @brief Represents a block type in the FPGA
 */
class BlockType {
  public:
    BlockType() = default;
    BlockType(int id, const std::string& name, int height, int width, const std::vector<PinClass>& pin_classes)
        : id_(id)
        , name_(name)
        , height_(height)
        , width_(width)
        , pin_classes_(pin_classes) {}

    // Getters
    int get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    int get_height() const { return height_; }
    int get_width() const { return width_; }
    const std::vector<PinClass>& get_pin_classes() const { return pin_classes_; }
    // Setters
    void set_id(int id) { id_ = id; }
    void set_name(const std::string& name) { name_ = name; }
    void set_height(int height) { height_ = height; }
    void set_width(int width) { width_ = width; }
    void set_pin_classes(const std::vector<PinClass>& pin_classes) {
        pin_classes_ = pin_classes;
    }

  private:
    int id_{-1};
    std::string name_;
    int height_{-1};
    int width_{-1};
    std::vector<PinClass> pin_classes_;
};

/**
 * @brief Represents a grid in the FPGA
 */
class GridLoc {
  public:
    GridLoc() = default;
    GridLoc(int type_id, int height_offset, int width_offset, int x, int y, int layer)
        : type_id_(type_id)
        , height_offset_(height_offset)
        , width_offset_(width_offset)
        , x_(x)
        , y_(y)
        , layer_(layer) {}

    // Getters
    int get_type_id() const { return type_id_; }
    int get_height_offset() const { return height_offset_; }
    int get_width_offset() const { return width_offset_; }
    int get_x() const { return x_; }
    int get_y() const { return y_; }
    int get_layer() const { return layer_; }

    // Setters
    void set_type_id(int type_id) { type_id_ = type_id; }
    void set_height_offset(int height_offset) { height_offset_ = height_offset; }
    void set_width_offset(int width_offset) { width_offset_ = width_offset; }
    void set_x(int x) { x_ = x; }
    void set_y(int y) { y_ = y; }
    void set_layer(int layer) { layer_ = layer; }

  private:
    int type_id_{-1};
    int height_offset_{-1};
    int width_offset_{-1};
    int x_{-1};
    int y_{-1};
    int layer_{-1};
};

/**
 * @brief Represents a switch with timing and sizing information
 */
class Switch {
  public:
    Switch() = default;
    Switch(SwitchId id, const std::string& name, const std::string& type, const Timing& timing, const Sizing& sizing)
        : id_(id)
        , name_(name)
        , type_(type)
        , timing_(timing)
        , sizing_(sizing) {}

    // Getters
    SwitchId get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    const std::string& get_type() const { return type_; }
    const Timing& get_timing() const { return timing_; }
    const Sizing& get_sizing() const { return sizing_; }

    // Setters
    void set_id(SwitchId id) { id_ = id; }
    void set_name(const std::string& name) { name_ = name; }
    void set_type(const std::string& type) { type_ = type; }
    void set_timing(const Timing& timing) { timing_ = timing; }
    void set_sizing(const Sizing& sizing) { sizing_ = sizing; }

  private:
    SwitchId id_{0};
    std::string name_;
    std::string type_;
    Timing timing_;
    Sizing sizing_;
};

/**
 * @brief Represents a wire segment
 */
class Segment {
  public:
    Segment() = default;
    Segment(SegmentId id, const std::string& name, int length, const std::string& res_type)
        : id_(id)
        , name_(name)
        , length_(length)
        , res_type_(res_type) {}

    // Getters
    SegmentId get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    int get_length() const { return length_; }
    const std::string& get_res_type() const { return res_type_; }
    // Setters
    void set_id(SegmentId id) { id_ = id; }
    void set_name(const std::string& name) { name_ = name; }
    void set_length(int length) { length_ = length; }
    void set_res_type(const std::string& res_type) { res_type_ = res_type; }

  private:
    SegmentId id_{0};
    std::string name_;
    int length_{-1};
    std::string res_type_;
};

/**
 * @brief Main routing resource graph class
 *
 * This class contains all nodes, edges, switches, and segments that make up
 * the FPGA routing resource graph.
 */
class RRGraph {
  public:
    RRGraph() = default;

    // Node management
    void add_node(const RRNode& node);
    void add_node(NodeId id, e_rr_type type, int capacity, const std::string& direction, const Location& location, const NodeTiming& timing, const NodeSegmentId& segment_id);
    const RRNode* get_node(NodeId id) const;
    RRNode* get_node(NodeId id);
    const std::vector<RRNode>& get_nodes() const { return nodes_; }
    std::vector<RRNode>& get_nodes() { return nodes_; }
    size_t get_node_count() const { return nodes_.size(); }

    // Edge management
    void add_edge(const RREdge& edge);
    void add_edge(NodeId src_node, NodeId sink_node, SwitchId switch_id);
    const std::vector<RREdge>& get_edges() const { return edges_; }
    std::vector<RREdge>& get_edges() { return edges_; }
    size_t get_edge_count() const { return edges_.size(); }

    // Channel management
    void add_channel(const Channel& channel);
    void add_channel(int max_width, int x_max, int x_min, int y_max, int y_min);
    const Channel* get_channel(size_t channel_id) const {
        return &channels_[channel_id];
    }
    Channel* get_channel(size_t channel_id) { return &channels_[channel_id]; }
    const std::vector<Channel>& get_channels() const { return channels_; }
    std::vector<Channel>& get_channels() { return channels_; }
    size_t get_channel_count() const { return channels_.size(); }

    // XYList management
    void add_xy_list(const XYList& xy_list);
    void add_xy_list(XYList::Type type, int index, int info);
    const XYList* get_xy_list(size_t xy_list_id) const {
        return &xy_lists_[xy_list_id];
    }
    XYList* get_xy_list(size_t xy_list_id) { return &xy_lists_[xy_list_id]; }
    const std::vector<XYList>& get_xy_lists() const { return xy_lists_; }
    std::vector<XYList>& get_xy_lists() { return xy_lists_; }
    size_t get_xy_list_count() const { return xy_lists_.size(); }

    // Switch management
    void add_switch(const Switch& switch_obj);
    void add_switch(SwitchId id, const std::string& name, const std::string& type, const Timing& timing, const Sizing& sizing);
    const Switch* get_switch(SwitchId id) const;
    Switch* get_switch(SwitchId id);
    const std::vector<Switch>& get_switches() const { return switches_; }
    std::vector<Switch>& get_switches() { return switches_; }
    size_t get_switch_count() const { return switches_.size(); }

    // Segment management
    void add_segment(const Segment& segment);
    void add_segment(SegmentId id, const std::string& name, int length, const std::string& res_type);
    const Segment* get_segment(SegmentId id) const;
    const std::vector<Segment>& get_segments() const { return segments_; }
    std::vector<Segment>& get_segments() { return segments_; }
    size_t get_segment_count() const { return segments_.size(); }

    // Block type management
    void add_block_type(const BlockType& block_type);
    void add_block_type(int id, const std::string& name, int height, int width, const std::vector<PinClass>& pin_classes);
    const BlockType* get_block_type(size_t id) const { return &block_types_[id]; }
    BlockType* get_block_type(size_t id) { return &block_types_[id]; }
    const std::vector<BlockType>& get_block_types() const { return block_types_; }
    std::vector<BlockType>& get_block_types() { return block_types_; }
    size_t get_block_type_count() const { return block_types_.size(); }

    // Grid management
    void add_grid_loc(const GridLoc& grid_loc);
    void add_grid_loc(int type_id, int height_offset, int width_offset, int x, int y, int layer);
    const GridLoc* get_grid_loc(size_t grid_loc_id) const {
        return &grid_locs_[grid_loc_id];
    }
    GridLoc* get_grid_loc(size_t grid_loc_id) { return &grid_locs_[grid_loc_id]; }
    const std::vector<GridLoc>& get_grid_locs() const { return grid_locs_; }
    std::vector<GridLoc>& get_grid_locs() { return grid_locs_; }
    size_t get_grid_loc_count() const { return grid_locs_.size(); }

    // Utility methods
    void clear();
    void reserve_nodes(size_t count) { nodes_.reserve(count); }
    void reserve_edges(size_t count) { edges_.reserve(count); }
    void reserve_switches(size_t count) { switches_.reserve(count); }
    void reserve_segments(size_t count) { segments_.reserve(count); }
    void sort_nodes() { std::sort(nodes_.begin(), nodes_.end()); }
    void sort_edges() { std::sort(edges_.begin(), edges_.end()); }
    void shrink_to_fit() {
        nodes_.shrink_to_fit();
        edges_.shrink_to_fit();
    }

    // Statistics
    void print_statistics() const;

    // Node categorization
    std::unordered_set<NodeId> get_source_nodes() const;
    std::unordered_set<NodeId> get_sink_nodes() const;
    std::unordered_set<NodeId> get_ipin_nodes() const;
    std::unordered_set<NodeId> get_opin_nodes() const;

    // Metadata
    void set_tool_info(const std::string& name, const std::string& version, const std::string& comment);
    const std::string& get_tool_name() const { return tool_name_; }
    const std::string& get_tool_version() const { return tool_version_; }
    const std::string& get_tool_comment() const { return tool_comment_; }

  private:
    std::vector<RRNode> nodes_;
    std::vector<RREdge> edges_;
    std::vector<Channel> channels_;
    std::vector<XYList> xy_lists_;
    std::vector<Switch> switches_;
    std::vector<Segment> segments_;
    std::vector<BlockType> block_types_;
    std::vector<GridLoc> grid_locs_;

    // Lookup maps for efficiency
    std::unordered_map<NodeId, size_t> node_id_to_index_;
    std::unordered_map<SwitchId, size_t> switch_id_to_index_;
    std::unordered_map<SegmentId, size_t> segment_id_to_index_;

    // Metadata
    std::string tool_name_;
    std::string tool_version_;
    std::string tool_comment_;

    // Helper methods
    void update_node_index(NodeId id, size_t index);
    void update_switch_index(SwitchId id, size_t index);
    void update_segment_index(SegmentId id, size_t index);
};

} // namespace crrgenerator
