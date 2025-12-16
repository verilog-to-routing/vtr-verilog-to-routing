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
                                           const SwitchBlockManager& sb_manager)
    : rr_graph_(rr_graph)
    , node_lookup_(node_lookup)
    , sb_manager_(sb_manager) {}

void CRRConnectionBuilder::initialize(int fpga_grid_x,
                                      int fpga_grid_y,
                                      bool is_annotated) {

    fpga_grid_x_ = fpga_grid_x;
    fpga_grid_y_ = fpga_grid_y;
    is_annotated_ = is_annotated;

    // Total locations is the number of locations on the FPGA grid minus the 4
    // corner locations.
    int number_of_tiles = fpga_grid_x_ * fpga_grid_y_;
    VTR_ASSERT(number_of_tiles > 4);
    total_locations_ = static_cast<size_t>(number_of_tiles) - 4;
    processed_locations_ = 0;
}

void CRRConnectionBuilder::build_connections_for_location(size_t x,
                                                          size_t y,
                                                          std::vector<Connection>& tile_connections) const {

    // Find matching switch block pattern
    std::string pattern = sb_manager_.find_matching_pattern(x, y);
    std::string sw_block_file_name = sb_manager_.get_pattern_file_name(pattern);
    tile_connections.clear();

    if (pattern.empty()) {
        // If no pattern is found, it means that no switch block is defined for this location
        // Thus, we return an empty vector of connections.
        VTR_LOG_DEBUG("No pattern found for switch block at (%zu, %zu)\n", x, y);
        return;
    }

    const DataFrame* df = sb_manager_.get_switch_block_dataframe(pattern);
    if (df == nullptr) {
        VTR_LOG_WARN("No dataframe found for pattern '%s' at (%zu, %zu)\n", pattern.c_str(), x, y);
        return;
    }

    VTR_LOG("Processing switch block with pattern '%s' at (%zu, %zu)\n",
            pattern.c_str(), x, y);

    // Get combined nodes for this location
    auto combined_nodes = node_lookup_.get_combined_nodes(x, y);

    // Get vertical and horizontal nodes
    auto source_nodes = get_tile_source_nodes(x, y, *df, combined_nodes);
    auto sink_nodes = get_tile_sink_nodes(x, y, *df, combined_nodes);

    // Build connections based on dataframe
    for (auto row_iter = df->begin(); row_iter != df->end(); ++row_iter) {
        size_t row_idx = row_iter.get_row_index();
        if (row_idx < NUM_EMPTY_ROWS) {
            continue;
        }

        for (size_t col_idx = NUM_EMPTY_COLS; col_idx < df->cols(); ++col_idx) {
            const Cell& cell = df->at(row_idx, col_idx);

            if (cell.is_empty()) {
                continue;
            }

            auto source_it = source_nodes.find(row_idx);
            auto sink_it = sink_nodes.find(col_idx);

            if (source_it == source_nodes.end() || sink_it == sink_nodes.end()) {
                continue;
            }

            RRNodeId source_node = source_it->second;
            e_rr_type source_node_type = rr_graph_.node_type(source_node);
            RRNodeId sink_node = sink_it->second;
            e_rr_type sink_node_type = rr_graph_.node_type(sink_node);
            std::string sw_template_id = sw_block_file_name + "_" + std::to_string(row_idx) + "_" + std::to_string(col_idx);
            // If the source node is an IPIN, then it should be considered as
            // a sink of the connection.
            if (source_node_type == e_rr_type::IPIN) {
                int delay_ps = get_connection_delay_ps(cell.as_string(),
                                                       rr_node_typename[source_node_type],
                                                       sink_node,
                                                       source_node);

                tile_connections.emplace_back(source_node, sink_node, delay_ps, sw_template_id);
            } else {
                int segment_length = -1;
                if (sink_node_type == e_rr_type::CHANX || sink_node_type == e_rr_type::CHANY) {
                    segment_length = rr_graph_.node_length(sink_node);
                }
                int delay_ps = get_connection_delay_ps(cell.as_string(),
                                                       rr_node_typename[sink_node_type],
                                                       source_node,
                                                       sink_node,
                                                       segment_length);

                tile_connections.emplace_back(sink_node, source_node, delay_ps, sw_template_id);
            }
        }
    }

    // Uniqueify the connections
    vtr::uniquify(tile_connections);
    tile_connections.shrink_to_fit();

    VTR_LOG_DEBUG("Generated %zu connections for location (%zu, %zu)\n",
                  tile_connections.size(), x, y);
}

std::vector<Connection> CRRConnectionBuilder::get_tile_connections(size_t tile_x, size_t tile_y) const {
    std::vector<Connection> tile_connections;
    build_connections_for_location(tile_x, tile_y, tile_connections);

    return tile_connections;
}

std::map<size_t, RRNodeId> CRRConnectionBuilder::get_tile_source_nodes(int x,
                                                                       int y,
                                                                       const DataFrame& df,
                                                                       const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const {
    std::map<size_t, RRNodeId> source_nodes;
    std::string prev_seg_type = "";
    int prev_seg_index = -1;
    e_sw_template_dir prev_side = e_sw_template_dir::NUM_SIDES;
    int prev_ptc_number = 0;

    for (size_t row = NUM_EMPTY_ROWS; row < df.rows(); ++row) {
        SegmentInfo info = parse_segment_info(df, row, true);
        if (!info.is_valid()) {
            continue;
        }

        RRNodeId node_id;
        if (info.side == e_sw_template_dir::IPIN || info.side == e_sw_template_dir::OPIN) {
            node_id = process_opin_ipin_node(info, x, y, node_lookup);
        } else if (info.side == e_sw_template_dir::LEFT || info.side == e_sw_template_dir::RIGHT || info.side == e_sw_template_dir::TOP || info.side == e_sw_template_dir::BOTTOM) {
            node_id = process_channel_node(info, x, y, node_lookup, prev_seg_index,
                                           prev_side, prev_seg_type, prev_ptc_number, true);
        }

        if (node_id != RRNodeId::INVALID()) {
            source_nodes[row] = node_id;
        }

        prev_seg_type = info.seg_type;
        prev_side = info.side;
    }

    return source_nodes;
}

std::map<size_t, RRNodeId> CRRConnectionBuilder::get_tile_sink_nodes(int x,
                                                                     int y,
                                                                     const DataFrame& df,
                                                                     const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const {
    std::map<size_t, RRNodeId> sink_nodes;
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
            node_id = process_opin_ipin_node(info, x, y, node_lookup);
        } else if (info.side == e_sw_template_dir::LEFT || info.side == e_sw_template_dir::RIGHT || info.side == e_sw_template_dir::TOP || info.side == e_sw_template_dir::BOTTOM) {
            node_id = process_channel_node(info, x, y, node_lookup, prev_seg_index,
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
                                                      const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup) const {
    VTR_ASSERT(info.side == e_sw_template_dir::OPIN || info.side == e_sw_template_dir::IPIN);
    e_rr_type node_type = (info.side == e_sw_template_dir::OPIN) ? e_rr_type::OPIN : e_rr_type::IPIN;
    NodeHash hash = std::make_tuple(node_type,
                                    std::to_string(info.seg_index),
                                    x, x, y, y);

    auto it = node_lookup.find(hash);
    if (it != node_lookup.end()) {
        return it->second;
    }

    return RRNodeId::INVALID();
}

RRNodeId CRRConnectionBuilder::process_channel_node(const SegmentInfo& info,
                                                    int x,
                                                    int y,
                                                    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& node_lookup,
                                                    int& prev_seg_index,
                                                    e_sw_template_dir& prev_side,
                                                    std::string& prev_seg_type,
                                                    int& prev_ptc_number,
                                                    bool is_vertical) const {
    // Check grid boundaries
    if ((info.side == e_sw_template_dir::RIGHT && x == fpga_grid_x_) || (info.side == e_sw_template_dir::TOP && y == fpga_grid_y_)) {
        return RRNodeId::INVALID();
    }

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
    auto it = node_lookup.find(hash);

    if (it != node_lookup.end()) {
        return it->second;
    } else {
        VTR_LOG_DEBUG("Node not found: %s [%s] (%d,%d) -> (%d,%d)\n", seg_type_label.c_str(),
                      seg_sequence.c_str(), x_low, y_low, x_high, y_high);
        return RRNodeId::INVALID();
    }
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

    // Calculate truncation
    truncated = (std::max(x_low, 1) - x_low) - (x_high - std::min(x_high, fpga_grid_x_));
    truncated += (std::max(y_low, 1) - y_low) - (y_high - std::min(y_high, fpga_grid_y_));

    // Apply grid boundaries
    x_low = std::max(x_low, 1);
    y_low = std::max(y_low, 1);
    x_high = std::min(x_high, fpga_grid_x_);
    y_high = std::min(y_high, fpga_grid_y_);

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

bool CRRConnectionBuilder::is_valid_grid_location(int x,
                                                  int y) const {
    return x >= 1 && x <= fpga_grid_x_ && y >= 1 && y <= fpga_grid_y_;
}

void CRRConnectionBuilder::update_progress() {
    size_t current = processed_locations_.fetch_add(1) + 1;
    if (current % std::max(size_t(1), total_locations_ / 20) == 0 || current == total_locations_) {
        double percentage =
            (static_cast<double>(current) / total_locations_) * 100.0;
        VTR_LOG("Connection building progress: %zu/%zu (%.1f%%)\n", current,
                total_locations_, percentage);
    }
}

} // namespace crrgenerator
