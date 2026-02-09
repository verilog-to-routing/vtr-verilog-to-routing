#include "crr_connection_builder.h"

#include <charconv>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>

#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_assert.h"
#include "rr_node_types.h"

namespace crrgenerator {

// Indices into the IPIN source node vector returned by get_tile_source_nodes.
//
// When an IPIN appears on a source row in the switch block dataframe, it
// represents a tap-zero connection. The correct IPIN depends on the direction
// of the channel wire (sink) being connected.
//
// Each tile contains a Functional Block (FB) with the Switch Block (SB) at
// its top-right corner. Wires on the right and top sides of each FB belong
// to the same tile.
//
//     +----------+------+   +----------+------+
//     | Top wires| SB   |   | Top wires| SB   |
//     |  (x,y)   |(x,y) |   | (x+1,y) |(x+1,y)|
//     +----------+------+   +----------+------+
//     |          | Right|   |          | Right|
//     | FB(x,y)  | wires|   |FB(x+1,y) | wires|
//     |          | (x,y)|   |          |(x+1,y)|
//     +----------+------+   +----------+------+
//       Tile (x, y)            Tile (x+1, y)
//
// INC CHANX -->  goes right from SB(x,y), uses wires on the right side
//                of FB(x,y) --> tap-0 IPIN at (x, y)
//
// DEC CHANX <--  enters SB(x,y) from the right, originates from the
//                right side of FB(x+1,y) --> tap-0 IPIN at (x+1, y)
//
// INC CHANY ^    goes up from SB(x,y), uses wires on the top side
//                of FB(x,y) --> tap-0 IPIN at (x, y)
//
// DEC CHANY v    enters SB(x,y) from the top, originates from the
//                top side of FB(x,y+1) --> tap-0 IPIN at (x, y+1)
//
static constexpr size_t IPIN_LOCAL = 0;
static constexpr size_t IPIN_X_PLUS_1 = 1;
static constexpr size_t IPIN_Y_PLUS_1 = 2;

static bool is_integer(const std::string& s) {
    int value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    // Uses std::from_chars to parse the entire string as an integer.
    // Returns true only if:
    // 1. The conversion succeeds without errors (ec == std::errc())
    // 2. The entire string was consumed (ptr points to the end)
    // This ensures strings like "123abc" or "12.5" return false, while
    // "123" or "-456" return true.
    return ec == std::errc() && ptr == s.data() + s.size();
}

CRRConnectionBuilder::CRRConnectionBuilder(const RRGraphView& rr_graph,
                                           const NodeLookupManager& node_lookup,
                                           const SwitchBlockManager& sb_manager,
                                           const int verbosity)
    : rr_graph_(rr_graph)
    , node_lookup_(node_lookup)
    , sb_manager_(sb_manager)
    , verbosity_(verbosity) {}

void CRRConnectionBuilder::initialize(int fpga_grid_x,
                                      int fpga_grid_y,
                                      bool preserve_ipin_connections,
                                      bool preserve_opin_connections,
                                      bool is_annotated) {

    fpga_grid_x_ = fpga_grid_x;
    fpga_grid_y_ = fpga_grid_y;
    preserve_ipin_connections_ = preserve_ipin_connections;
    preserve_opin_connections_ = preserve_opin_connections;
    is_annotated_ = is_annotated;
}

std::vector<Connection> CRRConnectionBuilder::build_connections_for_location(size_t x,
                                                                             size_t y) const {
    // Find matching switch block pattern
    std::string pattern = sb_manager_.find_matching_pattern(x, y);
    if (pattern.empty()) {
        // If no pattern is found, it means that no switch block is defined for this location
        // Thus, we return an empty vector of connections.
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No pattern found for switch block at (%zu, %zu)\n", x, y);
        return {};
    }
    std::string sw_block_file_name = sb_manager_.get_pattern_file_name(pattern);
    if (sw_block_file_name.empty()) {
        VTR_LOGV(verbosity_ > 1, "No switch block is associated with the pattern '%s' at (%zu, %zu)\n", pattern.c_str(), x, y);
        return {};
    }

    const DataFrame* df = sb_manager_.get_switch_block_dataframe(pattern);
    if (df == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No dataframe found for pattern '%s' at (%zu, %zu)\n", pattern.c_str(), x, y);
        return {};
    }

    VTR_LOGV(verbosity_ > 1, "Processing switch block with pattern '%s' at (%zu, %zu)\n",
             pattern.c_str(), x, y);

    const auto& col_nodes = node_lookup_.get_column_nodes(x);
    const auto& row_nodes = node_lookup_.get_row_nodes(y);

    // Get vertical and horizontal nodes
    auto source_nodes = get_tile_source_nodes(x, y, *df, col_nodes, row_nodes);
    auto sink_nodes = get_tile_sink_nodes(x, y, *df, col_nodes, row_nodes);

    // Build connections by iterating over the switch block dataframe
    auto tile_connections = build_connections_from_dataframe(*df, source_nodes, sink_nodes, sw_block_file_name);

    // Uniqueify the connections
    vtr::uniquify(tile_connections);
    tile_connections.shrink_to_fit();

    VTR_LOGV(verbosity_ > 1, "Generated %zu connections for location (%zu, %zu)\n",
             tile_connections.size(), x, y);

    return tile_connections;
}

std::vector<Connection> CRRConnectionBuilder::build_connections_from_dataframe(
    const DataFrame& df,
    const std::unordered_map<size_t, std::vector<RRNodeId>>& source_nodes,
    const std::unordered_map<size_t, RRNodeId>& sink_nodes,
    const std::string& sw_block_file_name) const {
    std::vector<Connection> connections;

    for (auto row_iter = df.begin(); row_iter != df.end(); ++row_iter) {
        size_t row_idx = row_iter.get_row_index();
        if (row_idx < NUM_EMPTY_ROWS) {
            continue;
        }

        for (size_t col_idx = NUM_EMPTY_COLS; col_idx < df.cols(); ++col_idx) {
            const Cell& cell = df.at(row_idx, col_idx);

            if (cell.is_empty()) {
                continue;
            }

            auto source_it = source_nodes.find(row_idx);
            auto sink_it = sink_nodes.find(col_idx);

            if (source_it == source_nodes.end() || sink_it == sink_nodes.end()) {
                continue;
            }

            const auto& source_node_vec = source_it->second;
            RRNodeId source_node = source_node_vec[0];
            RRNodeId sink_node = sink_it->second;
            e_rr_type sink_node_type = rr_graph_.node_type(sink_node);

            // IPIN source vectors have 3 slots (local, x+1, y+1) — any of
            // which may be INVALID. We detect an IPIN row by checking size==3
            // and resolve the correct node based on the sink direction.
            bool is_ipin_source = (source_node_vec.size() == 3);

            if (is_ipin_source) {
                // Tap-zero IPIN connections: when an IPIN appears on a source row,
                // the IPIN acts as a sink in the connection (source/sink are swapped).
                // Select the correct IPIN based on the sink channel's direction:
                //   - INC sinks    → IPIN at (x, y)
                //   - DEC CHANX    → IPIN at (x+1, y)
                //   - DEC CHANY    → IPIN at (x, y+1)
                if (rr_graph_.node_direction(sink_node) == Direction::DEC) {
                    if (sink_node_type == e_rr_type::CHANX) {
                        source_node = source_node_vec[IPIN_X_PLUS_1];
                    } else {
                        VTR_ASSERT(sink_node_type == e_rr_type::CHANY);
                        source_node = source_node_vec[IPIN_Y_PLUS_1];
                    }
                }

                // The selected IPIN may not exist (e.g. at grid boundaries)
                if (source_node == RRNodeId::INVALID()) {
                    continue;
                }

                if (preserve_ipin_connections_) {
                    continue;
                }

                std::string sw_template_id = sw_block_file_name + "_" + std::to_string(row_idx) + "_" + std::to_string(col_idx);
                int delay_ps = get_connection_delay_ps(cell.as_string(),
                                                       rr_node_typename[e_rr_type::IPIN],
                                                       sink_node,
                                                       source_node);

                connections.emplace_back(source_node, sink_node, delay_ps, sw_template_id);
            } else {
                if (source_node == RRNodeId::INVALID()) {
                    continue;
                }

                e_rr_type source_node_type = rr_graph_.node_type(source_node);

                // Skip connections involving IPIN/OPIN nodes if preservation is enabled
                if (preserve_ipin_connections_) {
                    if (source_node_type == e_rr_type::IPIN || sink_node_type == e_rr_type::IPIN) {
                        continue;
                    }
                }

                if (preserve_opin_connections_) {
                    if (source_node_type == e_rr_type::OPIN || sink_node_type == e_rr_type::OPIN) {
                        continue;
                    }
                }

                std::string sw_template_id = sw_block_file_name + "_" + std::to_string(row_idx) + "_" + std::to_string(col_idx);
                int segment_length = -1;
                if (sink_node_type == e_rr_type::CHANX || sink_node_type == e_rr_type::CHANY) {
                    segment_length = rr_graph_.node_length(sink_node);
                }
                int delay_ps = get_connection_delay_ps(cell.as_string(),
                                                       rr_node_typename[sink_node_type],
                                                       source_node,
                                                       sink_node,
                                                       segment_length);

                connections.emplace_back(sink_node, source_node, delay_ps, sw_template_id);
            }
        }
    }

    return connections;
}

std::vector<Connection> CRRConnectionBuilder::get_tile_connections(size_t tile_x, size_t tile_y) const {
    std::vector<Connection> tile_connections = build_connections_for_location(tile_x, tile_y);

    return tile_connections;
}

std::unordered_map<size_t, std::vector<RRNodeId>> CRRConnectionBuilder::get_tile_source_nodes(int x,
                                                                                              int y,
                                                                                              const DataFrame& df,
                                                                                              const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& col_nodes,
                                                                                              const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& row_nodes) const {
    std::unordered_map<size_t, std::vector<RRNodeId>> source_nodes;
    std::string prev_seg_type = "";
    int prev_seg_index = -1;
    e_sw_template_dir prev_side = e_sw_template_dir::NUM_SIDES;
    int prev_ptc_number = 0;

    for (size_t row = NUM_EMPTY_ROWS; row < df.rows(); ++row) {
        SegmentInfo info = parse_segment_info(df, row, true);
        if (!info.is_valid()) {
            continue;
        }

        std::vector<RRNodeId> node_ids;
        RRNodeId node_id;
        if (info.side == e_sw_template_dir::IPIN) {
            // For IPIN source rows, look up the IPIN at the local tile and at
            // neighboring tiles. Always push all 3 slots so that IPIN_LOCAL,
            // IPIN_X_PLUS_1, and IPIN_Y_PLUS_1 indices remain valid.
            node_ids.push_back(process_opin_ipin_node(info, x, y, col_nodes, row_nodes));
            node_ids.push_back(process_opin_ipin_node(info, x + 1, y, col_nodes, row_nodes));
            node_ids.push_back(process_opin_ipin_node(info, x, y + 1, col_nodes, row_nodes));
        } else if (info.side == e_sw_template_dir::OPIN) {
            node_id = process_opin_ipin_node(info, x, y, col_nodes, row_nodes);
            if (node_id != RRNodeId::INVALID()) {
                node_ids.push_back(node_id);
            }
        } else if (info.side == e_sw_template_dir::LEFT || info.side == e_sw_template_dir::RIGHT || info.side == e_sw_template_dir::TOP || info.side == e_sw_template_dir::BOTTOM) {
            node_id = process_channel_node(info, x, y, col_nodes, row_nodes, prev_seg_index,
                                           prev_side, prev_seg_type, prev_ptc_number, true);
            if (node_id != RRNodeId::INVALID()) {
                node_ids.push_back(node_id);
            }
        }

        if (!node_ids.empty()) {
            source_nodes[row] = node_ids;
        }

        prev_seg_type = info.seg_type;
        prev_side = info.side;
    }

    return source_nodes;
}

std::unordered_map<size_t, RRNodeId> CRRConnectionBuilder::get_tile_sink_nodes(int x,
                                                                               int y,
                                                                               const DataFrame& df,
                                                                               const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& col_nodes,
                                                                               const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& row_nodes) const {
    std::unordered_map<size_t, RRNodeId> sink_nodes;
    std::string prev_seg_type = "";
    int prev_seg_index = -1;
    e_sw_template_dir prev_side = e_sw_template_dir::NUM_SIDES;
    int prev_ptc_number = 0;

    for (size_t col = NUM_EMPTY_COLS; col < df.cols(); ++col) {
        SegmentInfo info = parse_segment_info(df, col, false);
        if (!info.is_valid()) {
            continue;
        }
        RRNodeId node_id;

        if (info.side == e_sw_template_dir::IPIN) {
            node_id = process_opin_ipin_node(info, x, y, col_nodes, row_nodes);
        } else if (info.side == e_sw_template_dir::LEFT || info.side == e_sw_template_dir::RIGHT || info.side == e_sw_template_dir::TOP || info.side == e_sw_template_dir::BOTTOM) {
            node_id = process_channel_node(info, x, y, col_nodes, row_nodes, prev_seg_index,
                                           prev_side, prev_seg_type, prev_ptc_number,
                                           false);
        }

        if (node_id != RRNodeId::INVALID()) {
            sink_nodes[col] = node_id;
        }

        prev_seg_type = info.seg_type;
        prev_side = info.side;
    }

    return sink_nodes;
}

CRRConnectionBuilder::SegmentInfo CRRConnectionBuilder::parse_segment_info(const DataFrame& df,
                                                                           size_t row_or_col,
                                                                           bool is_vertical) const {
    SegmentInfo info;

    if (is_vertical) {
        // Vertical processing (rows)
        const Cell& side_cell = df.at(row_or_col, 0);
        const Cell& type_cell = df.at(row_or_col, 1);
        const Cell& index_cell = df.at(row_or_col, 2);
        const Cell& tap_cell = df.at(row_or_col, 3);

        if (!side_cell.is_empty()) {
            std::string side_str_cap = side_cell.as_string();
            std::transform(side_str_cap.begin(), side_str_cap.end(), side_str_cap.begin(), ::toupper);
            info.side = get_sw_template_dir(side_str_cap);
        }
        if (!type_cell.is_empty()) {
            info.seg_type = type_cell.as_string();
        }
        if (!index_cell.is_empty()) {
            info.seg_index = static_cast<int>(index_cell.as_int());
        }
        if (!tap_cell.is_empty() && tap_cell.is_number()) {
            info.tap = static_cast<int>(tap_cell.as_int());
        }
    } else {
        // Horizontal processing (columns)
        const Cell& side_cell = df.at(0, row_or_col);
        const Cell& type_cell = df.at(1, row_or_col);
        const Cell& index_cell =
            df.at(3, row_or_col);                    // Note: row 3 for horizontal
        const Cell& tap_cell = df.at(4, row_or_col); // Note: row 4 for horizontal

        if (!side_cell.is_empty()) {
            std::string side_str_cap = side_cell.as_string();
            std::transform(side_str_cap.begin(), side_str_cap.end(), side_str_cap.begin(), ::toupper);
            info.side = get_sw_template_dir(side_str_cap);
        }
        if (!type_cell.is_empty()) {
            info.seg_type = type_cell.as_string();
        }
        if (!index_cell.is_empty()) {
            info.seg_index = static_cast<int>(index_cell.as_int());
        }
        if (!tap_cell.is_empty() && tap_cell.is_number()) {
            info.tap = static_cast<int>(tap_cell.as_int());
        } else {
            info.tap = 1;
        }
    }

    return info;
}

RRNodeId CRRConnectionBuilder::process_opin_ipin_node(const SegmentInfo& info,
                                                      int x,
                                                      int y,
                                                      const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& col_nodes,
                                                      const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& row_nodes) const {
    VTR_ASSERT(info.side == e_sw_template_dir::OPIN || info.side == e_sw_template_dir::IPIN);
    e_rr_type node_type = (info.side == e_sw_template_dir::OPIN) ? e_rr_type::OPIN : e_rr_type::IPIN;
    NodeHash hash = std::make_tuple(node_type,
                                    std::to_string(info.seg_index),
                                    x, x, y, y);

    auto col_it = col_nodes.find(hash);
    if (col_it != col_nodes.end()) {
        return col_it->second;
    }

    auto row_it = row_nodes.find(hash);
    if (row_it != row_nodes.end()) {
        return row_it->second;
    }

    return RRNodeId::INVALID();
}

RRNodeId CRRConnectionBuilder::process_channel_node(const SegmentInfo& info,
                                                    int x,
                                                    int y,
                                                    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& col_nodes,
                                                    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& row_nodes,
                                                    int& prev_seg_index,
                                                    e_sw_template_dir& prev_side,
                                                    std::string& prev_seg_type,
                                                    int& prev_ptc_number,
                                                    bool is_vertical) const {
    int seg_length = std::stoi(
        info.seg_type.substr(1)); // Extract number from "L1", "L4", etc.

    // Update PTC number based on previous segment
    if (prev_seg_type != info.seg_type) {
        prev_ptc_number = prev_seg_index;
    }
    if (prev_side != info.side) {
        prev_ptc_number = 0;
        prev_seg_index = 0;
    }
    std::string seg_type_label = get_segment_type_label(info.side);
    Direction direction = get_direction_for_side(info.side, is_vertical);
    VTR_ASSERT(direction == Direction::INC || direction == Direction::DEC);

    // Calculate segment coordinates
    int x_low, x_high, y_low, y_high;
    int physical_length, truncated;
    calculate_segment_coordinates(info, x, y, x_low, x_high, y_low, y_high,
                                  physical_length, truncated, is_vertical);

    // Calculate starting PTC point
    int seg_index = (info.seg_index - 1) * seg_length * 2;
    seg_index += prev_ptc_number;
    prev_seg_index =
        std::max({prev_seg_index, seg_index + 2 * seg_length});

    seg_index += (direction == Direction::INC) ? 0 : 1;
    seg_index += (direction == Direction::DEC) ? 2 * (seg_length - 1) : 0;

    // Calculate PTC sequence
    std::string seg_sequence = get_ptc_sequence(
        seg_index, seg_length, physical_length, direction, truncated);

    // Create node hash and lookup
    NodeHash hash = std::make_tuple(get_rr_type(seg_type_label),
                                    seg_sequence,
                                    x_low, x_high, y_low, y_high);

    auto col_it = col_nodes.find(hash);
    if (col_it != col_nodes.end()) {
        return col_it->second;
    }

    auto row_it = row_nodes.find(hash);
    if (row_it != row_nodes.end()) {
        return row_it->second;
    }

    VTR_LOGV(verbosity_ > 1, "Node not found: %s [%s] (%d,%d) -> (%d,%d)\n", seg_type_label.c_str(),
             seg_sequence.c_str(), x_low, y_low, x_high, y_high);
    return RRNodeId::INVALID();
}

void CRRConnectionBuilder::calculate_segment_coordinates(const SegmentInfo& info,
                                                         int x,
                                                         int y,
                                                         int& x_low,
                                                         int& x_high,
                                                         int& y_low,
                                                         int& y_high,
                                                         int& physical_length,
                                                         int& truncated,
                                                         bool is_vertical) const {
    int seg_length = std::stoi(info.seg_type.substr(1));
    int tap = info.tap;

    // Calculate initial coordinates based on side
    if (is_vertical) {
        switch (info.side) {
            case e_sw_template_dir::LEFT:
                x_high = x + (seg_length - tap);
                x_low = x - (tap - 1);
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::RIGHT:
                x_high = x + tap;
                x_low = x + tap + 1 - seg_length;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::TOP:
                x_high = x;
                x_low = x;
                y_high = y + tap;
                y_low = y + 1 - seg_length + tap;
                break;
            case e_sw_template_dir::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y + seg_length - tap;
                y_low = y - tap + 1;
                break;
            default:
                x_high = x;
                x_low = x;
                y_high = y;
                y_low = y;
                break;
        }
    } else {
        switch (info.side) {
            case e_sw_template_dir::LEFT:
                x_high = x + tap - 1;
                x_low = x - seg_length + tap;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::RIGHT:
                x_high = x + seg_length;
                x_low = x + 1;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::TOP:
                x_high = x;
                x_low = x;
                y_high = y + seg_length;
                y_low = y + 1;
                break;
            case e_sw_template_dir::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y;
                y_low = y - seg_length + tap;
                break;
            default:
                x_high = x;
                x_low = x;
                y_high = y;
                y_low = y;
                break;
        }
    }

    // VPR don't allow routing tracks on parameter of the device. Depending on the channel type, min_x/y are different.
    int min_x = 0;
    int min_y = 0;
    int max_x = fpga_grid_x_ - 2;
    int max_y = fpga_grid_y_ - 2;
    bool is_chanx = info.side == e_sw_template_dir::LEFT || info.side == e_sw_template_dir::RIGHT;

    if (is_chanx) {
        min_x = 1;
        min_y = 0;
    } else {
        min_x = 0;
        min_y = 1;
    }

    // Calculate truncation
    truncated = (std::max(x_low, min_x) - x_low) - (x_high - std::min(x_high, max_x));
    truncated += (std::max(y_low, min_y) - y_low) - (y_high - std::min(y_high, max_y));

    // Apply grid boundaries
    x_low = std::max(x_low, min_x);
    y_low = std::max(y_low, min_y);
    x_high = std::min(x_high, max_x);
    y_high = std::min(y_high, max_y);

    // Calculate physical length
    physical_length = (x_high - x_low) + (y_high - y_low) + 1;
}

Direction CRRConnectionBuilder::get_direction_for_side(e_sw_template_dir side,
                                                       bool is_vertical) const {
    if (is_vertical) {
        return (side == e_sw_template_dir::RIGHT || side == e_sw_template_dir::TOP) ? Direction::DEC
                                                                                    : Direction::INC;
    } else {
        return (side == e_sw_template_dir::RIGHT || side == e_sw_template_dir::TOP) ? Direction::INC
                                                                                    : Direction::DEC;
    }
}

std::string CRRConnectionBuilder::get_segment_type_label(e_sw_template_dir side) const {
    return (side == e_sw_template_dir::LEFT || side == e_sw_template_dir::RIGHT) ? "CHANX" : "CHANY";
}

std::string CRRConnectionBuilder::get_ptc_sequence(int seg_index,
                                                   int /*seg_length*/,
                                                   int physical_length,
                                                   Direction direction,
                                                   int truncated) const {
    std::vector<std::string> sequence;

    if (direction == Direction::DEC) {
        seg_index -= (2 * truncated > 0) ? 2 * truncated : 0;
        for (int i = 0; i < physical_length; ++i) {
            sequence.push_back(std::to_string(seg_index - (i * 2)));
        }
    } else {
        VTR_ASSERT(direction == Direction::INC);
        seg_index += (2 * truncated > 0) ? 2 * truncated : 0;
        for (int i = 0; i < physical_length; ++i) {
            sequence.push_back(std::to_string(seg_index + (i * 2)));
        }
    }

    std::string result;
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (i > 0) result += ",";
        result += sequence[i];
    }
    return result;
}

int CRRConnectionBuilder::get_connection_delay_ps(const std::string& cell_value,
                                                  const std::string& sink_node_type,
                                                  RRNodeId /*source_node*/,
                                                  RRNodeId /*sink_node*/,
                                                  int /*segment_length*/) const {
    std::string lower_case_sink_node_type = sink_node_type;
    std::transform(lower_case_sink_node_type.begin(),
                   lower_case_sink_node_type.end(),
                   lower_case_sink_node_type.begin(), ::tolower);

    if (is_integer(cell_value) && is_annotated_) {
        // TODO: This is a temporary solution. We need to have an API call to get
        // the switch id from delay.
        if (cell_value == "0") {
            return 0;
        }
        int switch_delay_ps = std::stoi(cell_value);
        return switch_delay_ps;
    } else {
        return -1;
    }
}

} // namespace crrgenerator
