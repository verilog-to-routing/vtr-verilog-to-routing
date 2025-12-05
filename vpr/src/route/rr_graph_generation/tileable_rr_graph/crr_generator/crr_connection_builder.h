#pragma once

#include "rr_graph_view.h"
#include "physical_types.h"

#include "crr_common.h"
#include "data_frame_processor.h"
#include "node_lookup_manager.h"
#include "crr_switch_block_manager.h"

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
    CRRConnectionBuilder(const RRGraphView& rr_graph,
                         const NodeLookupManager& node_lookup,
                         const SwitchBlockManager& sb_manager);

    /**
     * @brief Initialize the connection builder
     * @param node_lookup Node lookup manager
     * @param sb_manager Switch block manager
     * @param original_switches Original switches from the input graph
     */
    void initialize(int fpga_grid_x,
                    int fpga_grid_y,
                    bool is_annotated_excel);

    /**
     * @brief Get connections for a tile
     * @param tile_x Tile x coordinate
     * @param tile_y Tile y coordinate
     * @return Vector of connections
     */
    std::vector<Connection> get_tile_connections(size_t tile_x, size_t tile_y) const;

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
     * @brief Clear all connections
     */
    void clear() { all_connections_.clear(); }
    void remove_tile_connections(int x, int y) {
        all_connections_[static_cast<size_t>(x)][static_cast<size_t>(y)].clear();
        all_connections_[static_cast<size_t>(x)][static_cast<size_t>(y)].shrink_to_fit();
    }

  private:
    // Info from config
    int fpga_grid_x_;
    int fpga_grid_y_;
    bool is_annotated_excel_;

    // Dependencies
    const RRGraphView& rr_graph_;
    const NodeLookupManager& node_lookup_;
    const SwitchBlockManager& sb_manager_;

    // Generated connections
    std::vector<std::vector<std::vector<Connection>>> all_connections_;

    // Processing state
    std::atomic<size_t> processed_locations_{0};
    size_t total_locations_{0};

    // Connection building methods
    void build_connections_for_location(size_t x,
                                        size_t y,
                                        std::vector<Connection>& tile_connections) const;

    // Node processing methods
    std::map<size_t, RRNodeId> get_vertical_nodes(int x,
                                                  int y,
                                                  const DataFrame& df,
                                                  const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const;

    std::map<size_t, RRNodeId> get_horizontal_nodes(int x,
                                                    int y,
                                                    const DataFrame& df,
                                                    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const;

    // PTC sequence calculation
    std::string get_ptc_sequence(int seg_index,
                                 int seg_length,
                                 int physical_length,
                                 Direction direction,
                                 int truncated) const;

    // Segment processing helpers
    struct SegmentInfo {
        e_sw_template_side side;
        std::string seg_type;
        int seg_index;
        int tap;

        SegmentInfo()
            : side(e_sw_template_side::NUM_SIDES)
            , seg_index(-1)
            , tap(-1) {}
        SegmentInfo(e_sw_template_side s, const std::string& type, int index, int t = 1)
            : side(s)
            , seg_type(type)
            , seg_index(index)
            , tap(t) {}
    };

    SegmentInfo parse_segment_info(const DataFrame& df, size_t row_or_col, bool is_vertical) const;

    RRNodeId process_opin_ipin_node(const SegmentInfo& info,
                                    int x,
                                    int y,
                                    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const;

    RRNodeId process_channel_node(const SegmentInfo& info,
                                  int x,
                                  int y,
                                  const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup,
                                  int& prev_seg_index,
                                  e_sw_template_side& prev_side,
                                  std::string& prev_seg_type,
                                  int& prev_ptc_number,
                                  bool is_vertical) const;

    // Coordinate and direction calculations
    void calculate_segment_coordinates(const SegmentInfo& info,
                                       int x,
                                       int y,
                                       int& x_low,
                                       int& x_high,
                                       int& y_low,
                                       int& y_high,
                                       int& physical_length,
                                       int& truncated,
                                       bool is_vertical) const;

    Direction get_direction_for_side(e_sw_template_side side, bool is_vertical) const;
    std::string get_segment_type_label(e_sw_template_side side) const;

    // Return the switch id of an edge between two nodes
    int get_connection_delay_ps(const std::string& cell_value,
                                const std::string& sink_node_type,
                                RRNodeId source_node,
                                RRNodeId sink_node,
                                int segment_length = -1) const;

    // Validation and bounds checking
    bool is_valid_grid_location(int x, int y) const;

    // Progress tracking
    void update_progress();
};

} // namespace crrgenerator
