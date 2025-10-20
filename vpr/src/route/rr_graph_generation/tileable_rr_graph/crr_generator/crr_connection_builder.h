#pragma once

#include "crr_common.h"
#include "data_frame_processor.h"
#include "node_lookup_manager.h"
#include "crr_switch_block_manager.h"
#include "crr_thread_pool.h"

namespace crrgenerator {

/**
 * @brief Builds connections between routing nodes based on switch block
 * configurations
 *
 * This class processes switch block configurations and generates routing
 * connections between nodes, supporting both parallel and sequential
 * processing.
 */
class CRRConnectionBuilder {
 public:
  CRRConnectionBuilder(const RRGraph& rr_graph,
                    const NodeLookupManager& node_lookup,
                    const SwitchBlockManager& sb_manager);

  /**
   * @brief Initialize the connection builder
   * @param node_lookup Node lookup manager
   * @param sb_manager Switch block manager
   * @param original_switches Original switches from the input graph
   */
  void initialize(Coordinate fpga_grid_x,
                  Coordinate fpga_grid_y,
                  bool is_annotated_excel);

  /**
   * @brief Get connections for a tile
   * @param tile_x Tile x coordinate
   * @param tile_y Tile y coordinate
   * @return Vector of connections
   */
  std::vector<Connection> get_tile_connections(Coordinate tile_x, Coordinate tile_y);

  /**
   * @brief Get all generated connections
   * @return Vector of all connections
   */
  const std::vector<std::vector<std::vector<Connection>>>& get_all_connections() const {
    return all_connections_;
  }

  /**
   * @brief Get connection count
   * @return Number of connections generated
   */
  size_t get_connection_count() const {
    size_t count = 0;
    for (const auto& x : all_connections_) {
      for (const auto& y : x) {
        count += y.size();
      }
    }
    return count;
  }

  /**
   * @brief Get the default switch id map
   * @return Map of default switch id
   */
  std::map<std::string, SwitchId> get_default_swithes_map() const {
    return default_switch_id_;
  }

  /**
   * @brief Clear all connections
   */
  void clear() { all_connections_.clear(); }
  void remove_tile_connections(Coordinate x, Coordinate y) {
    all_connections_[static_cast<size_t>(x)][static_cast<size_t>(y)].clear();
    all_connections_[static_cast<size_t>(x)][static_cast<size_t>(y)].shrink_to_fit();
  }

 private:

  // Info from config
  Coordinate fpga_grid_x_;
  Coordinate fpga_grid_y_;
  bool is_annotated_excel_;

  // Dependencies
  const RRGraph& rr_graph_;
  const NodeLookupManager& node_lookup_;
  const SwitchBlockManager& sb_manager_;
  SwitchId sw_zero_id_;

  std::vector<std::string> storage_sw_names_;
  std::vector<std::map<size_t, NodeId>> storage_source_nodes_;
  std::vector<std::map<size_t, NodeId>> storage_sink_nodes_;

  // Generated connections
  std::vector<std::vector<std::vector<Connection>>> all_connections_;
  std::mutex connections_mutex_;

  // Processing state
  std::atomic<size_t> processed_locations_{0};
  size_t total_locations_{0};

  // Default switch id based on the node type
  std::map<std::string, SwitchId> default_switch_id_;

  // Connection building methods
  void build_connections_for_location(Coordinate x,
                                      Coordinate y,
                                      std::vector<Connection>& tile_connections);

  // Node processing methods
  std::map<size_t, NodeId> get_vertical_nodes(
      Coordinate x, Coordinate y, const DataFrame& df,
      const std::unordered_map<NodeHash, const RRNode*, NodeHasher>&
          node_lookup);

  std::map<size_t, NodeId> get_horizontal_nodes(
      Coordinate x, Coordinate y, const DataFrame& df,
      const std::unordered_map<NodeHash, const RRNode*, NodeHasher>&
          node_lookup);

  // PTC sequence calculation
  std::string get_ptc_sequence(int seg_index, int seg_length,
                               int physical_length, Direction direction,
                               int truncated);

  // Segment processing helpers
  struct SegmentInfo {
    Side side;
    std::string seg_type;
    int seg_index;
    int tap;

    SegmentInfo() : side(Side::INVALID), seg_index(-1), tap(-1) {}
    SegmentInfo(Side s, const std::string& type, int index, int t = 1)
        : side(s), seg_type(type), seg_index(index), tap(t) {}
  };

  SegmentInfo parse_segment_info(const DataFrame& df, size_t row_or_col,
                                 bool is_vertical);
  NodeId process_opin_ipin_node(
      const SegmentInfo& info, Coordinate x, Coordinate y,
      const std::unordered_map<NodeHash, const RRNode*, NodeHasher>&
          node_lookup);
  NodeId process_channel_node(const SegmentInfo& info, Coordinate x,
                              Coordinate y,
                              const std::unordered_map<NodeHash, const RRNode*,
                                                       NodeHasher>& node_lookup,
                              int& prev_seg_index, Side& prev_side,
                              std::string& prev_seg_type, int& prev_ptc_number,
                              bool is_vertical);

  // Coordinate and direction calculations
  void calculate_segment_coordinates(const SegmentInfo& info, Coordinate x,
                                     Coordinate y, Coordinate& x_low,
                                     Coordinate& x_high, Coordinate& y_low,
                                     Coordinate& y_high, int& physical_length,
                                     int& truncated, bool is_vertical);

  Direction get_direction_for_side(Side side, bool is_vertical);
  std::string get_segment_type_label(Side side);

  // Return the switch id of an edge between two nodes
  SwitchId get_edge_switch_id(const std::string& cell_value,
                              const std::string& sink_node_type,
                              NodeId source_node, NodeId sink_node,
                              int segment_length=-1);

  // Validation and bounds checking
  bool is_valid_grid_location(Coordinate x, Coordinate y) const;

  // Progress tracking
  void update_progress();
};

}  // namespace crrgenerator
