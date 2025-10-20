#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <sys/resource.h> // For getrusage on Unix/Linux

namespace crrgenerator {

// Forward declarations
class RRGraph;
class RRNode;
class RREdge;
class Switch;
class Segment;
class ConfigManager;
class XMLHandler;
class SwitchBlockManager;
class NodeLookupManager;
class ConnectionBuilder;
class SwitchManager;
class DataFrameProcessor;
class ThreadPool;
class RRGraphGenerator;

// Type aliases for convenience
using NodeId = int32_t;
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

// Node types
enum class NodeType { SOURCE, SINK, IPIN, OPIN, CHANX, CHANY, INVALID };

// Direction types
enum class Direction { INC_DIR, DEC_DIR };

// Side types
enum class Side { LEFT, RIGHT, TOP, BOTTOM, IPIN, OPIN, INVALID };

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
  Location(Coordinate xl, Coordinate xh, Coordinate yl, Coordinate yh,
           Coordinate l, const std::string& p, const std::string& s)
      : x_low(xl),
        x_high(xh),
        y_low(yl),
        y_high(yh),
        layer(l),
        ptc(p),
        side(s) {}

  bool operator==(const Location& other) const {
    return x_low == other.x_low && x_high == other.x_high &&
           y_low == other.y_low && y_high == other.y_high &&
           layer == other.layer && ptc == other.ptc && side == other.side;
  }
};

// Node timing information
struct NodeTiming {
  DelayValue r{0.0};
  DelayValue c{0.0};

  NodeTiming() = default;
  NodeTiming(DelayValue r_val, DelayValue c_val) : r(r_val), c(c_val) {}
};

// Node segment information
struct NodeSegmentId {
  int segment_id{-1};

  NodeSegmentId() = default;
  NodeSegmentId(int segment_id_val) : segment_id(segment_id_val) {}

  bool empty() const { return segment_id == -1; }
};

// Timing information
struct Timing {
  DelayValue Cin{0.0};
  DelayValue Tdel{0.0};

  Timing() = default;
  Timing(DelayValue cin_val, DelayValue tdel_val)
      : Cin(cin_val), Tdel(tdel_val) {}
};

// Sizing information
struct Sizing {
  DelayValue mux_trans_size{0.0};
  DelayValue buf_size{0.0};

  Sizing() = default;
  Sizing(DelayValue mux_size, DelayValue buffer_size)
      : mux_trans_size(mux_size), buf_size(buffer_size) {}
};

/**
 * @brief Connection class
 * 
 * This class represents a connection between two nodes in the RR graph.
 * It is used to store the connection information and to compare connections.
 */
class Connection {
public:
  Connection(NodeId sink_node, NodeId src_node, SwitchId switch_id)
      : sink_node_(sink_node), src_node_(src_node), switch_id_(switch_id) {}
  NodeId sink_node() const { return sink_node_; }
  NodeId src_node() const { return src_node_; }
  SwitchId switch_id() const { return switch_id_; }

  bool operator<(const Connection& other) const {
    return std::tie(sink_node_, src_node_, switch_id_) <
           std::tie(other.sink_node_, other.src_node_, other.switch_id_);
  }

  bool operator==(const Connection& other) const {
    return sink_node_ == other.sink_node_ && src_node_ == other.src_node_ &&
           switch_id_ == other.switch_id_;
  }

private:
  NodeId sink_node_;
  NodeId src_node_;
  SwitchId switch_id_;
};

// Hash function for Location
struct LocationHash {
  std::size_t operator()(const Location& loc) const {
    auto h1 = std::hash<Coordinate>{}(loc.x_low);
    auto h2 = std::hash<Coordinate>{}(loc.x_high);
    auto h3 = std::hash<Coordinate>{}(loc.y_low);
    auto h4 = std::hash<Coordinate>{}(loc.y_high);
    auto h5 = std::hash<std::string>{}(loc.ptc);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
  }
};

// Node hash type for lookups
using NodeHash = std::tuple<NodeType, std::string, Coordinate, Coordinate,
                            Coordinate, Coordinate>;

// Hash function for NodeHash
struct NodeHasher {
  std::size_t operator()(const NodeHash& hash) const {
    auto h1 = std::hash<int>{}(static_cast<int>(std::get<0>(hash)));
    auto h2 = std::hash<std::string>{}(std::get<1>(hash));
    auto h3 = std::hash<Coordinate>{}(std::get<2>(hash));
    auto h4 = std::hash<Coordinate>{}(std::get<3>(hash));
    auto h5 = std::hash<Coordinate>{}(std::get<4>(hash));
    auto h6 = std::hash<Coordinate>{}(std::get<5>(hash));

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

// Edge hash type for lookups
using EdgeKey = std::tuple<NodeId, NodeId, SwitchId>;

// Utility functions
inline std::string to_string(NodeType type) {
  switch (type) {
    case NodeType::SOURCE:
      return "SOURCE";
    case NodeType::SINK:
      return "SINK";
    case NodeType::IPIN:
      return "IPIN";
    case NodeType::OPIN:
      return "OPIN";
    case NodeType::CHANX:
      return "CHANX";
    case NodeType::CHANY:
      return "CHANY";
    default:
      return "UNKNOWN";
  }
}

inline NodeType string_to_node_type(const std::string& str) {
  if (str == "SOURCE" || str == "source") return NodeType::SOURCE;
  if (str == "SINK" || str == "sink") return NodeType::SINK;
  if (str == "IPIN" || str == "ipin") return NodeType::IPIN;
  if (str == "OPIN" || str == "opin") return NodeType::OPIN;
  if (str == "CHANX" || str == "chanx") return NodeType::CHANX;
  if (str == "CHANY" || str == "chany") return NodeType::CHANY;
  throw std::invalid_argument("Unknown node type: " + str);
}

inline std::string to_string(Direction dir) {
  return (dir == Direction::INC_DIR) ? "INC_DIR" : "DEC_DIR";
}

inline std::string to_string(Side side) {
  switch (side) {
    case Side::LEFT:
      return "Left";
    case Side::RIGHT:
      return "Right";
    case Side::TOP:
      return "Top";
    case Side::BOTTOM:
      return "Bottom";
    case Side::IPIN:
      return "IPIN";
    case Side::OPIN:
      return "OPIN";
    default:
      return "UNKNOWN";
  }
}

inline Side string_to_side(const std::string& str) {
  if (str == "Left") return Side::LEFT;
  if (str == "Right") return Side::RIGHT;
  if (str == "Top") return Side::TOP;
  if (str == "Bottom") return Side::BOTTOM;
  if (str == "IPIN") return Side::IPIN;
  if (str == "OPIN") return Side::OPIN;
  throw std::invalid_argument("Unknown side: " + str);
}

/**
 * @brief Get the memory usage
 * @return Memory usage in KB
 */
inline size_t get_memory_usage() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      return pmc.WorkingSetSize / 1024; // Convert bytes to KB
  }
  return 0;
#else
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return static_cast<size_t>(usage.ru_maxrss); // Already in KB on Linux, bytes on macOS
#ifdef __APPLE__
    return static_cast<size_t>(usage.ru_maxrss / 1024); // Convert bytes to KB on macOS
#endif
#endif
}

/**
 * @brief Get the peak memory usage
 * @return Peak memory usage in KB
 */
inline size_t get_peak_memory_usage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize / 1024; // Convert bytes to KB
    }
    return 0;
#else
    // On Unix/Linux, we need to read /proc/self/status for peak memory
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmHWM:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoul(value); // Already in KB
        }
    }
    
    // Fallback to getrusage if /proc/self/status is not available
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    return static_cast<size_t>(usage.ru_maxrss / 1024); // Convert bytes to KB on macOS
#else
    return static_cast<size_t>(usage.ru_maxrss); // Already in KB on Linux
#endif
#endif
}

// Exception classes
class RRGeneratorException : public std::runtime_error {
 public:
  explicit RRGeneratorException(const std::string& message)
      : std::runtime_error(message) {}
};

class ConfigException : public RRGeneratorException {
 public:
  explicit ConfigException(const std::string& message)
      : RRGeneratorException("Configuration error: " + message) {}
};

class FileException : public RRGeneratorException {
 public:
  explicit FileException(const std::string& message)
      : RRGeneratorException("File error: " + message) {}
};

class ParseException : public RRGeneratorException {
 public:
  explicit ParseException(const std::string& message)
      : RRGeneratorException("Parse error: " + message) {}
};

class SwitchException : public RRGeneratorException {
 public:
  explicit SwitchException(const std::string& message)
      : RRGeneratorException("Switch error: " + message) {}
};

}  // namespace crrgenerator