#include "crr_connection_builder.h"

#include <charconv>
#include <cmath>

#include "vtr_log.h"
#include "vtr_assert.h"
// #include <nlohmann/json.hpp>

// using json = nlohmann::json;
// using ordered_json = nlohmann::ordered_json;

namespace crrgenerator {

static bool is_integer(const std::string& s) {
    int value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    return ec == std::errc() && ptr == s.data() + s.size();
}

// static void write_node_storage_to_json(
//     const std::vector<std::string>& storage_sw_names,
//     const std::vector<std::map<size_t, NodeId>>& storage_source_nodes,
//     const std::vector<std::map<size_t, NodeId>>& storage_sink_nodes,
//     const std::string& filename) {
//   assert(storage_sw_names.size() == storage_source_nodes.size());
//   assert(storage_sw_names.size() == storage_sink_nodes.size());

//   ordered_json j;

//   for (size_t i = 0; i < storage_sw_names.size(); ++i) {
//     ordered_json source_nodes_json;
//     for (const auto& [idx, node] : storage_source_nodes[i]) {
//       source_nodes_json[std::to_string(idx)] = node;
//     }

//     ordered_json sink_nodes_json;
//     for (const auto& [idx, node] : storage_sink_nodes[i]) {
//       sink_nodes_json[std::to_string(idx)] = node;
//     }

//     j[storage_sw_names[i]] = {{"source_nodes", source_nodes_json},
//                               {"sink_nodes", sink_nodes_json}};
//   }

//   std::ofstream file(filename);
//   file << j.dump(4);  // Pretty print with indent of 4
// }

CRRConnectionBuilder::CRRConnectionBuilder(const RRGraph& rr_graph,
                                           const NodeLookupManager& node_lookup,
                                           const SwitchBlockManager& sb_manager)
    : rr_graph_(rr_graph)
    , node_lookup_(node_lookup)
    , sb_manager_(sb_manager) {}

void CRRConnectionBuilder::initialize(
    Coordinate fpga_grid_x,
    Coordinate fpga_grid_y,
    bool is_annotated_excel) {

    fpga_grid_x_ = fpga_grid_x;
    fpga_grid_y_ = fpga_grid_y;
    is_annotated_excel_ = is_annotated_excel;

    for (const auto& original_switch : rr_graph_.get_switches()) {
        std::string switch_name = original_switch.get_name();
        std::transform(switch_name.begin(), switch_name.end(), switch_name.begin(),
                       ::tolower);

        if (switch_name.find("delayless") != std::string::npos) {
            VTR_LOG("Adding delayless switch: %s\n", switch_name.c_str());
            default_switch_id_["delayless"] = original_switch.get_id();
        } else if (switch_name.find("ipin") != std::string::npos) {
            VTR_LOG("Adding ipin switch: %s\n", switch_name.c_str());
            default_switch_id_["ipin"] = original_switch.get_id();
        } else if (std::regex_match(switch_name, std::regex(R"(l1(_.*)?)"))) {
            VTR_LOG("Adding l1 switch: %s\n", switch_name.c_str());
            default_switch_id_["l1"] = original_switch.get_id();
        } else if (std::regex_match(switch_name, std::regex(R"(l2(_.*)?)"))) {
            VTR_LOG("Adding l2 switch: %s\n", switch_name.c_str());
            default_switch_id_["l2"] = original_switch.get_id();
        } else if (std::regex_match(switch_name, std::regex(R"(l4(_.*)?)"))) {
            VTR_LOG("Adding l4 switch: %s\n", switch_name.c_str());
            default_switch_id_["l4"] = original_switch.get_id();
        } else if (std::regex_match(switch_name, std::regex(R"(l8(_.*)?)"))) {
            VTR_LOG("Adding l8 switch: %s\n", switch_name.c_str());
            default_switch_id_["l8"] = original_switch.get_id();
        } else if (std::regex_match(switch_name, std::regex(R"(l12(_.*)?)"))) {
            VTR_LOG("Adding l12 switch: %s\n", switch_name.c_str());
            default_switch_id_["l12"] = original_switch.get_id();
        } else {
            VTR_LOG_ERROR("Unknown switch type: %s\n", switch_name.c_str());
        }
    }

    assert(default_switch_id_.size() == rr_graph_.get_switches().size());
    sw_zero_id_ = static_cast<SwitchId>(rr_graph_.get_switches().size());

    // Total locations is the number of locations on the FPGA grid minus the 4
    // corner locations.
    total_locations_ = static_cast<size_t>(fpga_grid_x_ * fpga_grid_y_) - 4;
    processed_locations_ = 0;

    all_connections_.resize(static_cast<size_t>(fpga_grid_x_ + 1),
                            std::vector<std::vector<Connection>>(static_cast<size_t>(fpga_grid_y_ + 1)));

    VTR_LOG("CRRConnectionBuilder initialized for %d x %d grid (%zu locations)\n",
            fpga_grid_x_, fpga_grid_y_, total_locations_);
}

void CRRConnectionBuilder::build_connections_for_location(Coordinate x,
                                                          Coordinate y,
                                                          std::vector<Connection>& tile_connections) const {

    // Find matching switch block pattern
    std::string sw_name = "SB_" + std::to_string(x) + "__" + std::to_string(y) + "_";
    std::string pattern = sb_manager_.find_matching_pattern(sw_name);
    tile_connections.clear();

    if (pattern.empty()) {
        VTR_LOG_DEBUG("No pattern found for switch block at (%d, %d)\n", x, y);
        return;
    }

    const DataFrame* df = sb_manager_.get_switch_block_dataframe(pattern);
    if (df == nullptr) {
        VTR_LOG_WARN("No dataframe found for pattern '%s' at (%d, %d)\n", pattern.c_str(), x, y);
        return;
    }

    VTR_LOG("Processing switch block '%s' with pattern '%s' at (%d, %d)\n",
            sw_name.c_str(), pattern.c_str(), x, y);

    // Get combined nodes for this location
    auto combined_nodes = node_lookup_.get_combined_nodes(x, y);

    // Get vertical and horizontal nodes
    auto source_nodes = get_vertical_nodes(x, y, *df, combined_nodes);
    auto sink_nodes = get_horizontal_nodes(x, y, *df, combined_nodes);

    // Build connections based on dataframe
    for (auto row_iter = df->begin(); row_iter != df->end(); ++row_iter) {
        size_t row_idx = row_iter.get_row_index();
        if (row_idx < NUM_EMPTY_ROWS) {
            continue;
        }

        for (size_t col_idx = NUM_EMPTY_COLS; col_idx < df->cols(); ++col_idx) {
            const Cell& cell = df->at(row_idx, col_idx);

            if (!cell.is_empty()) {
                auto source_it = source_nodes.find(row_idx);
                auto sink_it = sink_nodes.find(col_idx);

                if (source_it != source_nodes.end() && sink_it != sink_nodes.end()) {
                    NodeId source_node = source_it->second;
                    NodeType source_node_type = rr_graph_.get_node(source_node)->get_type();
                    NodeId sink_node = sink_it->second;
                    NodeType sink_node_type = rr_graph_.get_node(sink_node)->get_type();
                    // If the source node is an IPIN, then it should be considered as
                    // a sink of the connection.
                    if (source_node_type == NodeType::IPIN) {
                        SwitchId switch_id =
                            get_edge_switch_id(cell.as_string(),
                                               to_string(source_node_type),
                                               sink_node,
                                               source_node);

                        tile_connections.emplace_back(source_node, sink_node, switch_id);
                    } else {
                        int segment_length = -1;
                        if (sink_node_type == NodeType::CHANX || sink_node_type == NodeType::CHANY) {
                            segment_length = rr_graph_.get_segment(rr_graph_.get_node(sink_node)->get_segment().segment_id)->get_length();
                        }
                        SwitchId switch_id =
                            get_edge_switch_id(cell.as_string(),
                                               to_string(sink_node_type),
                                               source_node,
                                               sink_node,
                                               segment_length);

                        tile_connections.emplace_back(sink_node, source_node, switch_id);
                    }
                }
            }
        }
    }

    std::sort(tile_connections.begin(), tile_connections.end());
    tile_connections.erase(std::unique(tile_connections.begin(),
                                       tile_connections.end()),
                           tile_connections.end());
    tile_connections.shrink_to_fit();

    VTR_LOG_DEBUG("Generated %zu connections for location (%d, %d)\n",
                  tile_connections.size(), x, y);
}

std::vector<Connection> CRRConnectionBuilder::get_tile_connections(Coordinate tile_x, Coordinate tile_y) const {
    std::vector<Connection> tile_connections;
    build_connections_for_location(tile_x, tile_y, tile_connections);

    return tile_connections;
}

std::map<size_t, NodeId> CRRConnectionBuilder::get_vertical_nodes(Coordinate x, Coordinate y, const DataFrame& df,
                                                                  const std::unordered_map<NodeHash, const RRNode*, NodeHasher>& node_lookup) const {
    std::map<size_t, NodeId> source_nodes;
    std::string prev_seg_type = "";
    int prev_seg_index = -1;
    Side prev_side = Side::INVALID;
    int prev_ptc_number = 0;

    for (size_t row = NUM_EMPTY_ROWS; row < df.rows(); ++row) {
        SegmentInfo info = parse_segment_info(df, row, true);
        NodeId node_id = 0;

        if (info.side == Side::IPIN || info.side == Side::OPIN) {
            node_id = process_opin_ipin_node(info, x, y, node_lookup);
        } else if (info.side == Side::LEFT || info.side == Side::RIGHT || info.side == Side::TOP || info.side == Side::BOTTOM) {
            node_id =
                process_channel_node(info, x, y, node_lookup, prev_seg_index,
                                     prev_side, prev_seg_type, prev_ptc_number, true);
        }

        if (node_id > 0) {
            source_nodes[row] = node_id;
        }

        prev_seg_type = info.seg_type;
        prev_side = info.side;
    }

    return source_nodes;
}

std::map<size_t, NodeId> CRRConnectionBuilder::get_horizontal_nodes(Coordinate x, Coordinate y, const DataFrame& df,
                                                                    const std::unordered_map<NodeHash, const RRNode*, NodeHasher>& node_lookup) const {
    std::map<size_t, NodeId> sink_nodes;
    std::string prev_seg_type = "";
    int prev_seg_index = -1;
    Side prev_side = Side::INVALID;
    int prev_ptc_number = 0;

    for (size_t col = NUM_EMPTY_COLS; col < df.cols(); ++col) {
        SegmentInfo info = parse_segment_info(df, col, false);
        if (info.side == Side::INVALID) {
            continue;
        }
        NodeId node_id = 0;

        if (info.side == Side::IPIN) {
            node_id = process_opin_ipin_node(info, x, y, node_lookup);
        } else if (info.side == Side::LEFT || info.side == Side::RIGHT || info.side == Side::TOP || info.side == Side::BOTTOM) {
            node_id = process_channel_node(info, x, y, node_lookup, prev_seg_index,
                                           prev_side, prev_seg_type, prev_ptc_number,
                                           false);
        }

        if (node_id > 0) {
            sink_nodes[col] = node_id;
        }

        prev_seg_type = info.seg_type;
        prev_side = info.side;
    }

    return sink_nodes;
}

CRRConnectionBuilder::SegmentInfo CRRConnectionBuilder::parse_segment_info(const DataFrame& df, size_t row_or_col,
                                                                           bool is_vertical) const {
    SegmentInfo info;

    if (is_vertical) {
        // Vertical processing (rows)
        const Cell& side_cell = df.at(row_or_col, 0);
        const Cell& type_cell = df.at(row_or_col, 1);
        const Cell& index_cell = df.at(row_or_col, 2);
        const Cell& tap_cell = df.at(row_or_col, 3);

        if (!side_cell.is_empty()) {
            info.side = string_to_side(side_cell.as_string());
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
            info.side = string_to_side(side_cell.as_string());
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

NodeId CRRConnectionBuilder::process_opin_ipin_node(const SegmentInfo& info, Coordinate x, Coordinate y,
                                                    const std::unordered_map<NodeHash, const RRNode*, NodeHasher>& node_lookup) const {
    assert(info.side == Side::OPIN || info.side == Side::IPIN);
    NodeType node_type =
        (info.side == Side::OPIN) ? NodeType::OPIN : NodeType::IPIN;
    NodeHash hash =
        std::make_tuple(node_type, std::to_string(info.seg_index), x, x, y, y);

    auto it = node_lookup.find(hash);
    if (it != node_lookup.end()) {
        return it->second->get_id();
    }

    return 0;
}

NodeId CRRConnectionBuilder::process_channel_node(const SegmentInfo& info, Coordinate x, Coordinate y,
                                                  const std::unordered_map<NodeHash, const RRNode*, NodeHasher>& node_lookup,
                                                  int& prev_seg_index, Side& prev_side, std::string& prev_seg_type, int& prev_ptc_number,
                                                  bool is_vertical) const {
    // Check grid boundaries
    if ((info.side == Side::RIGHT && x == fpga_grid_x_) || (info.side == Side::TOP && y == fpga_grid_y_)) {
        return 0;
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

    // Calculate segment coordinates
    Coordinate x_low, x_high, y_low, y_high;
    int physical_length, truncated;
    calculate_segment_coordinates(info, x, y, x_low, x_high, y_low, y_high,
                                  physical_length, truncated, is_vertical);

    // Calculate starting PTC point
    int seg_index = (info.seg_index - 1) * seg_length * 2;
    seg_index += prev_ptc_number;
    prev_seg_index =
        std::max({prev_seg_index, seg_index + 2 * seg_length});

    seg_index += (direction == Direction::INC_DIR) ? 0 : 1;
    seg_index += (direction == Direction::DEC_DIR) ? 2 * (seg_length - 1) : 0;

    // Calculate PTC sequence
    std::string seg_sequence = get_ptc_sequence(
        seg_index, seg_length, physical_length, direction, truncated);

    // Create node hash and lookup
    NodeHash hash = std::make_tuple(string_to_node_type(seg_type_label),
                                    seg_sequence, x_low, x_high, y_low, y_high);
    auto it = node_lookup.find(hash);

    if (it != node_lookup.end()) {
        return it->second->get_id();
    } else {
        VTR_LOG_DEBUG("Node not found: %s [%s] (%d,%d) -> (%d,%d)\n", seg_type_label.c_str(),
                      seg_sequence.c_str(), x_low, y_low, x_high, y_high);
        return 0;
    }
}

void CRRConnectionBuilder::calculate_segment_coordinates(const SegmentInfo& info,
                                                         Coordinate x, Coordinate y,
                                                         Coordinate& x_low, Coordinate& x_high,
                                                         Coordinate& y_low, Coordinate& y_high,
                                                         int& physical_length, int& truncated,
                                                         bool is_vertical) const {
    int seg_length = std::stoi(info.seg_type.substr(1));
    int tap = info.tap;

    // Calculate initial coordinates based on side
    if (is_vertical) {
        switch (info.side) {
            case Side::LEFT:
                x_high = x + (seg_length - tap);
                x_low = x - (tap - 1);
                y_high = y;
                y_low = y;
                break;
            case Side::RIGHT:
                x_high = x + tap;
                x_low = x + tap + 1 - seg_length;
                y_high = y;
                y_low = y;
                break;
            case Side::TOP:
                x_high = x;
                x_low = x;
                y_high = y + tap;
                y_low = y + 1 - seg_length + tap;
                break;
            case Side::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y + seg_length - tap;
                y_low = y - tap + 1;
                break;
            default:
                x_high = x_low = x;
                y_high = y_low = y;
                break;
        }
    } else {
        switch (info.side) {
            case Side::LEFT:
                x_high = x + tap - 1;
                x_low = x - seg_length + tap;
                y_high = y;
                y_low = y;
                break;
            case Side::RIGHT:
                x_high = x + seg_length;
                x_low = x + 1;
                y_high = y;
                y_low = y;
                break;
            case Side::TOP:
                x_high = x;
                x_low = x;
                y_high = y + seg_length;
                y_low = y + 1;
                break;
            case Side::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y;
                y_low = y - seg_length + tap;
                break;
            default:
                x_high = x_low = x;
                y_high = y_low = y;
                break;
        }
    }

    // Calculate truncation
    truncated = (std::max(x_low, static_cast<Coordinate>(1)) - x_low) - (x_high - std::min(x_high, fpga_grid_x_));
    truncated += (std::max(y_low, static_cast<Coordinate>(1)) - y_low) - (y_high - std::min(y_high, fpga_grid_y_));

    // Apply grid boundaries
    x_low = std::max(x_low, static_cast<Coordinate>(1));
    y_low = std::max(y_low, static_cast<Coordinate>(1));
    x_high = std::min(x_high, fpga_grid_x_);
    y_high = std::min(y_high, fpga_grid_y_);

    // Calculate physical length
    physical_length = (x_high - x_low) + (y_high - y_low) + 1;
}

Direction CRRConnectionBuilder::get_direction_for_side(Side side,
                                                       bool is_vertical) const {
    if (is_vertical) {
        return (side == Side::RIGHT || side == Side::TOP) ? Direction::DEC_DIR
                                                          : Direction::INC_DIR;
    } else {
        return (side == Side::RIGHT || side == Side::TOP) ? Direction::INC_DIR
                                                          : Direction::DEC_DIR;
    }
}

std::string CRRConnectionBuilder::get_segment_type_label(Side side) const {
    return (side == Side::LEFT || side == Side::RIGHT) ? "CHANX" : "CHANY";
}

std::string CRRConnectionBuilder::get_ptc_sequence(int seg_index,
                                                   int /*seg_length*/,
                                                   int physical_length,
                                                   Direction direction,
                                                   int truncated) const {
    std::vector<std::string> sequence;

    if (direction == Direction::DEC_DIR) {
        seg_index -= (2 * truncated > 0) ? 2 * truncated : 0;
        for (int i = 0; i < physical_length; ++i) {
            sequence.push_back(std::to_string(seg_index - (i * 2)));
        }
    } else {
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

SwitchId CRRConnectionBuilder::get_edge_switch_id(const std::string& cell_value, const std::string& sink_node_type,
                                                  NodeId /*source_node*/, NodeId /*sink_node*/,
                                                  int segment_length) const {
    std::string lower_case_sink_node_type = sink_node_type;
    std::transform(lower_case_sink_node_type.begin(),
                   lower_case_sink_node_type.end(),
                   lower_case_sink_node_type.begin(), ::tolower);

    if (is_integer(cell_value) && is_annotated_excel_) {
        // TODO: This is a temporary solution. We need to have an API call to get
        // the switch id from delay.
        if (cell_value == "0") {
            return sw_zero_id_;
        }
        int switch_delay_ps = std::stoi(cell_value);
        int switch_id = switch_delay_ps;
        return static_cast<SwitchId>(switch_id);
    } else {
        std::string switch_id_key = "";
        if (segment_length > 0) {
            switch_id_key = "l" + std::to_string(segment_length);
        } else {
            switch_id_key = lower_case_sink_node_type;
        }

        std::string capitalized_switch_id_key = switch_id_key;
        std::transform(capitalized_switch_id_key.begin(),
                       capitalized_switch_id_key.end(),
                       capitalized_switch_id_key.begin(), ::toupper);

        if (default_switch_id_.find(switch_id_key) != default_switch_id_.end()) {
            return default_switch_id_.at(switch_id_key);
        } else if (default_switch_id_.find(capitalized_switch_id_key) != default_switch_id_.end()) {
            return default_switch_id_.at(capitalized_switch_id_key);
        } else {
            throw std::runtime_error("Default switch id not found for Node Type: " + lower_case_sink_node_type + " and Switch ID Key: " + capitalized_switch_id_key);
        }
    }
}

bool CRRConnectionBuilder::is_valid_grid_location(Coordinate x,
                                                  Coordinate y) const {
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
