#pragma once

#include <vector>

#include "rr_graph_fwd.h"
#include "rr_node_types.h"
namespace crrgenerator {

// Forward declarations
class SwitchBlockManager;
class NodeLookupManager;
class ConnectionBuilder;
class DataFrameProcessor;

// Type aliases for convenience
using SwitchId = int32_t;
using SegmentId = int32_t;
using Coordinate = int32_t;
using DelayValue = float;

// Constants
// Number of empty rows and columns in the Excel file
constexpr int NUM_EMPTY_ROWS = 5;
constexpr int NUM_EMPTY_COLS = 4;

constexpr SwitchId DELAY_0_ID = 1;
constexpr int DEFAULT_SWITCH_DELAY_MIN = 10000;

// Direction types
enum class Direction { INC_DIR,
                       DEC_DIR };

// Side types
enum class Side { LEFT,
                  RIGHT,
                  TOP,
                  BOTTOM,
                  IPIN,
                  OPIN,
                  INVALID };

// Location structure
struct Location {
    Coordinate x_low{-1};
    Coordinate x_high{-1};
    Coordinate y_low{-1};
    Coordinate y_high{-1};
    Coordinate layer{-1};
    std::string ptc;
    std::string side;

    Location() = default;
    Location(Coordinate xl, Coordinate xh, Coordinate yl, Coordinate yh, Coordinate l, const std::string& p, const std::string& s)
        : x_low(xl)
        , x_high(xh)
        , y_low(yl)
        , y_high(yh)
        , layer(l)
        , ptc(p)
        , side(s) {}

    bool operator==(const Location& other) const {
        return x_low == other.x_low && x_high == other.x_high && y_low == other.y_low && y_high == other.y_high && layer == other.layer && ptc == other.ptc && side == other.side;
    }
};

// Node timing information
struct NodeTiming {
    DelayValue r{0.0};
    DelayValue c{0.0};

    NodeTiming() = default;
    NodeTiming(DelayValue r_val, DelayValue c_val)
        : r(r_val)
        , c(c_val) {}
};

// Node segment information
struct NodeSegmentId {
    int segment_id{-1};

    NodeSegmentId() = default;
    NodeSegmentId(int segment_id_val)
        : segment_id(segment_id_val) {}

    bool empty() const { return segment_id == -1; }
};

// Timing information
struct Timing {
    DelayValue Cin{0.0};
    DelayValue Tdel{0.0};

    Timing() = default;
    Timing(DelayValue cin_val, DelayValue tdel_val)
        : Cin(cin_val)
        , Tdel(tdel_val) {}
};

// Sizing information
struct Sizing {
    DelayValue mux_trans_size{0.0};
    DelayValue buf_size{0.0};

    Sizing() = default;
    Sizing(DelayValue mux_size, DelayValue buffer_size)
        : mux_trans_size(mux_size)
        , buf_size(buffer_size) {}
};

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
