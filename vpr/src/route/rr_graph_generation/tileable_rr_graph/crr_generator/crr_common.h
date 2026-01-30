#pragma once

/**
 * @file 
 * @brief Common data structures used by custom RR graph generator.
 */

#include <unordered_map>
#include <vector>
#include <array>
#include <string>

#include "rr_graph_fwd.h"
#include "rr_node_types.h"
namespace crrgenerator {
// Number of empty rows and columns in the switch template file
constexpr int NUM_EMPTY_ROWS = 5;
constexpr int NUM_EMPTY_COLS = 4;

// e_sw_template_dir types
enum class e_sw_template_dir { LEFT = 0,
                               RIGHT,
                               TOP,
                               BOTTOM,
                               IPIN,
                               OPIN,
                               NUM_SIDES };

inline e_sw_template_dir get_sw_template_dir(const std::string& name) {
    if (name == "LEFT") {
        return e_sw_template_dir::LEFT;
    } else if (name == "RIGHT") {
        return e_sw_template_dir::RIGHT;
    } else if (name == "TOP") {
        return e_sw_template_dir::TOP;
    } else if (name == "BOTTOM") {
        return e_sw_template_dir::BOTTOM;
    } else if (name == "IPIN") {
        return e_sw_template_dir::IPIN;
    } else if (name == "OPIN") {
        return e_sw_template_dir::OPIN;
    } else {
        std::string error_message = "Invalid switch template direction name: " + name;
        VTR_ASSERT_MSG(false, error_message.c_str());
        return e_sw_template_dir::NUM_SIDES;
    }
}
constexpr vtr::array<e_sw_template_dir, const char*, (size_t)e_sw_template_dir::NUM_SIDES> template_side_name = {"LEFT", "RIGHT",
                                                                                                                 "TOP", "BOTTOM",
                                                                                                                 "IPIN", "OPIN"};

/**
 * @brief Connection class
 * 
 * This class represents a connection between two nodes in the RR graph.
 * It is used to store the connection information and to compare connections.
 */
class Connection {
  public:
    Connection(RRNodeId sink_node, RRNodeId src_node, int delay_ps, std::string sw_template_id) noexcept
        : sink_node_(sink_node)
        , src_node_(src_node)
        , delay_ps_(delay_ps)
        , sw_template_id_(std::move(sw_template_id)) {}

    RRNodeId sink_node() const { return sink_node_; }
    RRNodeId src_node() const { return src_node_; }
    int delay_ps() const { return delay_ps_; }
    std::string sw_template_id() const { return sw_template_id_; }

    bool operator<(const Connection& other) const {
        return std::tie(sink_node_, src_node_, delay_ps_) < std::tie(other.sink_node_, other.src_node_, other.delay_ps_);
    }

    bool operator==(const Connection& other) const {
        return sink_node_ == other.sink_node_ && src_node_ == other.src_node_ && delay_ps_ == other.delay_ps_;
    }

  private:
    RRNodeId sink_node_;
    RRNodeId src_node_;
    int delay_ps_;
    std::string sw_template_id_;
};

// Node hash type for lookups
// The first element is the node type
// The second element is the PTC sequence
// The third element is the x-low coordinate of the node
// The fourth element is the x-high coordinate of the node
// The fifth element is the y-low coordinate of the node
// The sixth element is the y-high coordinate of the node
using NodeHash = std::tuple<e_rr_type, std::string, short, short, short, short>;

// Hash function for NodeHash
struct NodeHasher {
    std::size_t operator()(const NodeHash& hash) const noexcept {
        auto h1 = std::hash<int>{}(static_cast<int>(std::get<0>(hash)));
        auto h2 = std::hash<std::string>{}(std::get<1>(hash));
        auto h3 = std::hash<short>{}(std::get<2>(hash));
        auto h4 = std::hash<short>{}(std::get<3>(hash));
        auto h5 = std::hash<short>{}(std::get<4>(hash));
        auto h6 = std::hash<short>{}(std::get<5>(hash));

        // Combine hashes using a better mixing function
        std::size_t result = h1;
        result ^= h2 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h3 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h4 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h5 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= h6 + 0x9e3779b9 + (result << 6) + (result >> 2);

        return result;
    }
};

} // namespace crrgenerator
