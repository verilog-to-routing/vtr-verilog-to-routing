#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <cstdint>

#include "vtr_range.h"
#include "vtr_array.h"
#include "vtr_ndmatrix.h"
#include "rr_graph_fwd.h"

/**
 * @brief Type of a routing resource node.
 *
 * x-directed channel segment, y-directed channel segment,
 * input pin to a clb to pad, output from a clb or pad
 * (i.e. output pin of a net) and:
 * - SOURCE
 * - SINK
 */
enum class e_rr_type : unsigned char {
    SOURCE = 0, ///<A dummy node that is a logical output within a block -- i.e., the gate that generates a signal.
    SINK,       ///<A dummy node that is a logical input within a block -- i.e. the gate that needs a signal.
    IPIN,       ///<Input pin to a block
    OPIN,       ///<Output pin of a block
    CHANX,      ///<x-directed routing wire, or an x-directed segment of a channel for global routing
    CHANY,      ///<y-directed routing wire, or a y-directed segment of a channel for global routing
    CHANZ,      ///<z-directed routing wire used to connect two different layers.
                ///< For CHANZ nodes, xlow == xhigh and yhigh == ylow
    MUX,        ///<a routing multiplexer that does not traverse a significant distance before feeding
                /// other rr-nodes. E.g. the first node in a 2-stage mux in a switch block.
    NUM_RR_TYPES
};

constexpr bool is_pin(e_rr_type type) { return (type == e_rr_type::IPIN || type == e_rr_type::OPIN); }
constexpr bool is_chanxy(e_rr_type type) { return (type == e_rr_type::CHANX || type == e_rr_type::CHANY); }
constexpr bool is_chanz(e_rr_type type) { return (type == e_rr_type::CHANZ); }
constexpr bool is_src_sink(e_rr_type type) { return (type == e_rr_type::SOURCE || type == e_rr_type::SINK); }

/// Used to iterate for different e_rr_type values in range-based for loops.
constexpr std::array<e_rr_type, (size_t)e_rr_type::NUM_RR_TYPES> RR_TYPES = {{e_rr_type::SOURCE, e_rr_type::SINK,
                                                                              e_rr_type::IPIN, e_rr_type::OPIN,
                                                                              e_rr_type::CHANX, e_rr_type::CHANY, e_rr_type::CHANZ,
                                                                              e_rr_type::MUX}};

/**
 * @brief Lookup for the string representation of the given node type. This is useful
 *        for logging the type of an RR node.
 */
constexpr vtr::array<e_rr_type, const char*, (size_t)e_rr_type::NUM_RR_TYPES> rr_node_typename {"SOURCE", "SINK",
                                                                                               "IPIN", "OPIN",
                                                                                               "CHANX", "CHANY", "CHANZ",
                                                                                               "MUX"};

/**
 * @enum Direction
 * @brief Represents the wire direction for a routing resource node.
 */
enum class Direction : unsigned char {
    INC = 0,     ///< wire driver is positioned at the low-coordinate end of the wire.
    DEC = 1,     ///< wire_driver is positioned at the high-coordinate end of the wire.
    BIDIR = 2,   ///< wire has multiple drivers, so signals can travel either way along the wire
    NONE = 3,    ///< node does not have a direction, such as IPIN/OPIN
    NUM_DIRECTIONS
};


constexpr std::array<const char*, static_cast<int>(Direction::NUM_DIRECTIONS)> DIRECTION_STRING = {{"INC_DIRECTION", "DEC_DIRECTION", "BI_DIRECTION", "NONE"}};

//this array is used in rr_graph_storage.cpp so that node_direction_string() can return a const std::string&
const std::array<std::string, static_cast<int>(Direction::NUM_DIRECTIONS)> CONST_DIRECTION_STRING = {{"INC_DIR", "DEC_DIR", "BI_DIR", "NONE"}};

// Node reordering algorithms for rr_nodes
enum e_rr_node_reorder_algorithm {
    DONT_REORDER,
    DEGREE_BFS,
    RANDOM_SHUFFLE,
};

///@brief Type used to express rr_node edge index.
typedef uint16_t t_edge_size;

/**
 * @brief An iterator that dereferences to an edge index
 *
 * Used in conjunction with vtr::Range to return ranges of edge indices
 */
class edge_idx_iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = t_edge_size;
    using pointer = t_edge_size*;
    using reference = t_edge_size&;

    edge_idx_iterator(value_type init)
        : value_(init) {}
    edge_idx_iterator& operator++() {
        value_ += 1;
        return *this;
    }
    edge_idx_iterator& operator--() {
        value_ -= 1;
        return *this;
    }
    reference operator*() { return value_; }
    pointer operator->() { return &value_; }

    friend bool operator==(const edge_idx_iterator& lhs, const edge_idx_iterator& rhs) { return lhs.value_ == rhs.value_; }
    friend bool operator!=(const edge_idx_iterator& lhs, const edge_idx_iterator& rhs) { return !(lhs == rhs); }

  private:
    value_type value_;
};

typedef vtr::Range<edge_idx_iterator> edge_idx_range;

typedef std::vector<std::map<int, RRSwitchId>> t_arch_switch_fanin;

/**
 * @brief Resistance/Capacitance data for an RR Node.
 *
 * In practice many RR nodes have the same values, so they are fly-weighted
 * to keep t_rr_node small. Each RR node holds an rc_index which allows
 * retrieval of it's RC data.
 *
 * R:  Resistance to go through an RR node.  This is only metal
 *     resistance (end to end, so conservative) -- it doesn't include the
 *     switch that leads to another rr_node.
 * C:  Total capacitance of an RR node.  Includes metal capacitance, the
 *     input capacitance of all switches hanging off the node, the
 *     output capacitance of all switches to the node, and the connection
 *     box buffer capacitances hanging off it.
 */
struct t_rr_rc_data {
    t_rr_rc_data(float Rval, float Cval) noexcept;

    float R;    ///< Resistance to go through an RR node
    float C;    ///<  Total capacitance of an RR node.
};

// This is the data type of fast lookups of an rr-node given an (rr_type, layer, x, y, and the side)
//[0..num_rr_types-1][0..num_layer-1][0..grid_width-1][0..grid_height-1][0..NUM_2D_SIDES-1][0..max_ptc-1]
typedef vtr::array<e_rr_type, vtr::NdMatrix<std::vector<RRNodeId>, 4>, (size_t)e_rr_type::NUM_RR_TYPES> t_rr_node_indices;
