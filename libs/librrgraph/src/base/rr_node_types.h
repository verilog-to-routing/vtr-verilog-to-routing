#ifndef RR_NODE_TYPES_H
#define RR_NODE_TYPES_H

#include <string>
#include <vector>
#include <array>
#include <map>
#include "vtr_range.h"
#include "vtr_ndmatrix.h"

/**
 * @brief Type of a routing resource node.
 *
 * x-directed channel segment, y-directed channel segment,
 * input pin to a clb to pad, output from a clb or pad
 * (i.e. output pin of a net) and:
 * - SOURCE
 * - SINK
 */
typedef enum e_rr_type : unsigned char {
    SOURCE = 0, ///<A dummy node that is a logical output within a block -- i.e., the gate that generates a signal.
    SINK,       ///<A dummy node that is a logical input within a block -- i.e. the gate that needs a signal.
    IPIN,       ///Input pin to a block
    OPIN,       ///Output pin of a block
    CHANX,      ///x-directed routing wire, or an x-directed segment of a channel for global routing
    CHANY,      ///y-directed routing wire, or a y-directed segment of a channel for global routing
    NUM_RR_TYPES
} t_rr_type;

constexpr std::array<t_rr_type, NUM_RR_TYPES> RR_TYPES = {{SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}};
constexpr std::array<const char*, NUM_RR_TYPES> rr_node_typename{{"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY"}};

/*
 * Direction::INC: wire driver is positioned at the low-coordinate end of the wire.
 * Direction::DEC: wire_driver is positioned at the high-coordinate end of the wire.
 * Direction::BIDIR: wire has multiple drivers, so signals can travel either way along the wire
 * Direction::NONE: node does not have a direction, such as IPIN/OPIN
 */
enum class Direction : unsigned char {
    INC = 0,
    DEC = 1,
    BIDIR = 2,
    NONE = 3,
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
 * Used inconjunction with vtr::Range to return ranges of edge indices
 */
class edge_idx_iterator : public std::iterator<std::bidirectional_iterator_tag, t_edge_size> {
  public:
    edge_idx_iterator(value_type init)
        : value_(init) {}
    iterator operator++() {
        value_ += 1;
        return *this;
    }
    iterator operator--() {
        value_ -= 1;
        return *this;
    }
    reference operator*() { return value_; }
    pointer operator->() { return &value_; }

    friend bool operator==(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return lhs.value_ == rhs.value_; }
    friend bool operator!=(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return !(lhs == rhs); }

  private:
    value_type value_;
};

typedef vtr::Range<edge_idx_iterator> edge_idx_range;

typedef std::vector<std::map<int, int>> t_arch_switch_fanin;

/*
 * Reistance/Capacitance data for an RR Nodes
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

    float R;
    float C;
};

// This is the data type of fast lookups of an rr-node given an (rr_type, x, y, and the side)
//[0..num_rr_types-1][0..grid_width-1][0..grid_height-1][0..NUM_SIDES-1][0..max_ptc-1]
typedef std::array<vtr::NdMatrix<std::vector<int>, 3>, NUM_RR_TYPES> t_rr_node_indices;

#endif
