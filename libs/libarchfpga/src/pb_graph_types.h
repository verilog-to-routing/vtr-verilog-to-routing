#pragma once

/**
 * @file pb_graph_types.h
 * @brief The PB graph represents the flattened and elaborated connectivity within a t_pb_type
 *        (i.e. the routing resource graph), derived from the t_pb_type hierarchy.
 *
 *        The PB graph is built of t_pb_graph_node and t_pb_graph_pin objects.
 *
 *        There is a single PB graph associated with each root t_pb_type, and it is referenced
 *        in other objects (e.g. t_pb) as a flyweight.
 */

#include <limits>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vtr_assert.h"

#include "arch_types.h"
#include "physical_types.h"

/**
 * @brief Describes the internal connectivity corresponding to a t_pb_type and t_mode of a cluster.
 *
 *        There is a t_pb_graph_node for each instance of the pb_type (i.e. t_pb_type may
 *        describe num_pb instances of the type, with each instance represented as a
 *        t_pb_graph_node). The distinction between the pb_type and the pb_graph_node is
 *        necessary since the 'position' of a particular instance in the cluster is important
 *        when routing the cluster (since the routing accessible from each position may be
 *        different).
 */
class t_pb_graph_node {
  public:
    t_pb_graph_node() = default;
    /// Pins, edges and child nodes point at a node,
    /// and its port views point into its own pins. It is never copied.
    t_pb_graph_node(const t_pb_graph_node&) = delete;
    t_pb_graph_node& operator=(const t_pb_graph_node&) = delete;

    /// Pointer to the type of pb graph node this belongs to
    t_pb_type* pb_type;

    /// There are a certain number of pbs available, this gives the index of the node
    int placement_index;

    /// There is a root-level pb_graph_node assigned to each logical type. Each logical type
    /// can contain multiple primitives. If this pb_graph_node is associated with a primitive,
    /// a unique number is assigned to it within the logical block level.
    int primitive_num = ARCH_FPGA_UNDEFINED_VAL;

    /** Contains a collection of mode indices that cannot be used as they produce conflicts during VPR packing stage
     *
     * Illegal modes do arise when children of a graph_node do have inconsistent `edge_modes` with respect to
     * the parent_pb.
     * Example: Edges that connect LUTs A, B and C to the parent pb_graph_node refer to the correct parent's mode which is set to "LUTs",
     *          but edges of LUT D have the mode of edge corresponding to a wrong parent's pb_graph_node mode, namely "LUTRAM".
     *          This situation is unfeasible as the edge modes are inconsistent between siblings of the same parent pb_graph_node.
     *          In this case, the "LUTs" mode of the parent pb_graph_node cannot be used as the LUT D is not able to have a feasible
     *          edge mode that does relate with the other sibling's edge modes.
     *
     *          The "LUTs" index mode is added to the illegal_modes vector. The conflicting mode marked as illegal is the most restrictive one.
     *          This means that LUT D is unable to be routed if using the parent's "LUTs" mode (otherwise "LUTs" mode would be selected for LUT D
     *          as well), but LUTs A, B and C could still be routed using the parent pb_graph_node's mode "LUTRAM".
     *          Therefore, "LUTs" is marked as illegal and all the LUTs (A, B, C and D) will have a consistent parent pb_graph_node mode, namely "LUTRAM".
     *
     * Usage: cluster_router uses this information to exclude the expansion of a node which has a not consistent mode.
     *        Every time the mode consistency check fails, the index of the mode that causes the conflict is added to this vector.
     * */
    std::vector<int> illegal_modes;

    t_pin_range pin_num_range;

    /// Pins of each input port, as views into pins().
    /// [0..num_input_ports-1][0..num_port_pins-1]
    std::vector<std::span<t_pb_graph_pin>> pins_by_input_port;
    /// Pins of each output port, as views into pins().
    /// [0..num_output_ports-1][0..num_port_pins-1]
    std::vector<std::span<t_pb_graph_pin>> pins_by_output_port;
    /// Pins of each clock port, as views into pins().
    /// [0..num_clock_ports-1][0..num_port_pins-1]
    std::vector<std::span<t_pb_graph_pin>> pins_by_clock_port;

    int num_input_ports;
    int num_output_ports;
    int num_clock_ports;

    /// Number of pins of each input port.
    /// [0..num_input_ports-1]
    std::vector<int> num_input_pins;
    /// Number of pins of each output port.
    /// [0..num_output_ports-1]
    std::vector<int> num_output_pins;
    /// Number of pins of each clock port.
    /// [0..num_clock_ports-1]
    std::vector<int> num_clock_pins;

    /// Child nodes of each mode, by child pb_type and then by instance.
    /// [0..num_modes-1][0..num_pb_type_in_mode-1][0..num_pb-1]
    std::vector<std::vector<std::vector<t_pb_graph_node>>> child_pb_graph_nodes;
    /// parent pb graph node
    t_pb_graph_node* parent_pb_graph_node;

    int total_pb_pins; /* only valid for top-level */

    /// Stores the number of pins in each input pin class indexed by input pin class number
    std::vector<size_t> input_pin_class_sizes;
    /// Stores the number of pins in each output pin class indexed by output pin class number
    std::vector<size_t> output_pin_class_sizes;

    /// Total number of this primitive type in the cluster. If there are 10 ALMs per cluster
    /// and 2 FFs per ALM (given the mode of the parent of this primitive) then the total is 20.
    /// This member is only used by nodes corresponding to primitive sites.
    int total_primitive_count;

    /* Interconnect instances for this pb
     * Only used for power
     */
    t_pb_graph_node_power* pb_node_power;
    t_interconnect_pins** interconnect_pins; /* [0..num_modes-1][0..num_interconnect_in_mode] */

    /// Returns true if this pb_graph_node represents a primitive type (primitives have 0 modes)
    bool is_primitive() const { return this->pb_type->is_primitive(); }

    /// Returns true if this pb_graph_node represents a root graph node (ex. clb)
    bool is_root() const { return this->parent_pb_graph_node == nullptr; }

    /// Returns the number of pins on this graph node
    /// Note this is the total for all ports on this node excluding any children (i.e. sum of all num_input_pins, num_output_pins, num_clock_pins)
    int num_pins() const;

    /// All pins of this node in pin_count_in_cluster order, which follows
    /// the port order of pb_type.
    std::span<const t_pb_graph_pin> pins() const;
    std::span<t_pb_graph_pin> mutable_pins();

    /// Creates the pins of this node. Called once by the builder,
    /// since the port views and the edges point into the storage.
    void alloc_pins(int num_pins);

    /// Returns a string containing the hierarchical type name of the pb_graph_node
    /// Ex: clb[0][default]/lab[0][default]/fle[3][n1_lut6]/ble6[0][default]/lut6[0]
    std::string hierarchical_type_name() const;

  private:
    /// Storage of all pins of this node.
    std::vector<t_pb_graph_pin> pins_;
};

/// @brief Identify pb pin type for timing purposes
enum class e_pb_graph_pin_type {
    NORMAL = 0,
    SEQUENTIAL,
    INPAD,
    OUTPAD,
    TERMINAL,
    CLOCK
};

/** Describes a pb graph pin
 *
 *  Data Members:
 *      port: pointer to the port that this pin is associated with
 *      pin_number: pin number of the port that this pin is associated with
 *      input edges: [0..num_input_edges - 1]edges incoming
 *      num_input_edges: number edges incoming
 *      output edges: [0..num_output_edges - 1]edges out_going
 *      num_output_edges: number edges out_going
 *      parent_node: parent pb_graph_node
 *      pin_count_in_cluster: Unique number for pin inside cluster
 */
class t_pb_graph_pin {
  public:
    t_pb_graph_pin() = default;
    /// Edges and lookup tables point at a pin, so it is never copied.
    t_pb_graph_pin(const t_pb_graph_pin&) = delete;
    t_pb_graph_pin& operator=(const t_pb_graph_pin&) = delete;

    t_port* port = nullptr;
    int pin_number = 0;
    std::vector<t_pb_graph_edge*> input_edges; /* [0..num_input_edges] */
    int num_input_edges = 0;
    /// This map is initialized only if flat_routing is enabled
    std::unordered_map<const t_pb_graph_pin*, int> sink_pin_edge_idx_map; /* [t_pb_graph_pin*] -> edge_idx - This is the index of the corresponding edge stored in output_edges vector */

    std::vector<t_pb_graph_edge*> output_edges; /* [0..num_output_edges] */
    int num_output_edges = 0;

    t_pb_graph_node* parent_node = nullptr;
    int pin_count_in_cluster = 0;

    /// The type of this pin (sequential, i/o etc.)
    e_pb_graph_pin_type type = e_pb_graph_pin_type::NORMAL;

    /* sequential timing information */
    float tsu = std::numeric_limits<float>::quiet_NaN();     /* For sequential logic elements the setup time */
    float thld = std::numeric_limits<float>::quiet_NaN();    /* For sequential logic elements the hold time */
    float tco_min = std::numeric_limits<float>::quiet_NaN(); /* For sequential logic elements the minimum clock to output time */
    float tco_max = std::numeric_limits<float>::quiet_NaN(); /* For sequential logic elements the maximum clock to output time */
    t_pb_graph_pin* associated_clock_pin = nullptr;          /* For sequential elements, the associated clock */

    /* This member is used when flat-routing and router_opt_choke_points are enabled.
     * It is used to identify choke points.
     * This is only valid for IPINs, and it only contains the pins that are reachable to the pin by a forwarding path.
     * It doesn't take into account feed-back connection.
     * */
    std::unordered_set<int> connected_sinks_ptc; /* ptc numbers of sinks which are directly or indirectly connected to this pin */

    /* combinational timing information */
    int num_pin_timing = 0;                   /* Number of ipin to opin timing edges*/
    std::vector<t_pb_graph_pin*> pin_timing;  /* timing edge sink pins  [0..num_pin_timing-1]*/
    std::vector<float> pin_timing_del_max;    /* primitive ipin to opin max-delay [0..num_pin_timing-1]*/
    std::vector<float> pin_timing_del_min;    /* primitive ipin to opin min-delay [0..num_pin_timing-1]*/
    int num_pin_timing_del_max_annotated = 0; ///< The list of valid pin_timing_del_max entries runs from [0..num_pin_timing_del_max_annotated-1]
    int num_pin_timing_del_min_annotated = 0; ///< The list of valid pin_timing_del_max entries runs from [0..num_pin_timing_del_min_annotated-1]

    /* Applies to clusters only */
    int pin_class = 0;

    /// The grouping of pins that this particular pin belongs to.
    /// @note Applies to pins of primitive only.
    /// [0..depth-1]
    std::vector<int> parent_pin_class;
    /// What input pins this output can connect to without exiting cluster at given depth.
    /// @note Applies to output pins of primitives only.
    /// [0..depth-1][0..num_connectable_input_pins-1]
    std::vector<std::vector<t_pb_graph_pin*>> list_of_connectable_input_pin_ptrs;

    bool is_forced_connection = false; /* This output pin connects to one and only one input pin */

    t_pb_graph_pin_power* pin_power = nullptr;

    // class member functions
  public:
    /// Returns true if this pin belongs to a primitive block like
    /// a LUT or FF, instead of a cluster-level block like a CLB.
    bool is_primitive_pin() const {
        return this->parent_node->is_primitive();
    }
    /// Returns true if this pin is an output bit of a bus-based mux, i.e. one of
    /// its incoming edges implements a bit of such a mux.
    bool is_bus_mux_output() const;
    /// Returns true if this pin belongs to a root pb_block which is a pb_block
    /// that has no parent block. For example, pins of a CLB, IO, DSP, etc.
    bool is_root_block_pin() const {
        return this->parent_node->is_root();
    }
    /// @brief Returns the edge from this pin to sink_pin. Only valid when flat routing is enabled.
    const t_pb_graph_edge* edge_to(const t_pb_graph_pin* sink_pin) const;
    /// This function returns a string that contains the name of the pin
    /// and the entire sequence of pb_types in the hierarchy from the block
    /// of this pin back to the cluster-level (top-level) pb_type in the
    /// following format: clb[0]/lab[0]/fle[3]/ble6[0]/lut6[0].in[0]
    /// if full_description is set to false it will only return lut6[0].in[0]
    std::string to_string(const bool full_description = true) const;
};

// These members touch the pin storage, so they are defined
// once t_pb_graph_pin is complete.
inline int t_pb_graph_node::num_pins() const { return (int)pins_.size(); }

inline std::span<const t_pb_graph_pin> t_pb_graph_node::pins() const { return pins_; }

inline std::span<t_pb_graph_pin> t_pb_graph_node::mutable_pins() { return pins_; }

inline void t_pb_graph_node::alloc_pins(int num_pins) {
    VTR_ASSERT(pins_.empty());
    pins_ = std::vector<t_pb_graph_pin>(num_pins);
}

/** Describes a pb graph edge
 *
 *  An edge connects one input pin to one output pin.
 *
 *  Data members:
 *      num_pack_patterns: number of pack patterns this edge belongs to
 *      pack_pattern_names: [0..num_pack_patterns-1] name of each pack pattern
 *      pack_pattern_indices: [0..num_pack_patterns-1] id of each pack pattern
 *      infer_pattern: if true, pattern of this edge could be inferred by checking
 *                     input/output edges. This is true when the edge is a single
 *                     fanout edge and is driven or driving another edge which is
 *                     annotated with a pack pattern.
 */
class t_pb_graph_edge {
  public:
    /* timing information */
    float delay_max;
    float delay_min;

    /* who drives this edge */
    t_interconnect* interconnect;
    int driver_set;

    /* pack pattern info */
    int num_pack_patterns;
    std::vector<const char*> pack_pattern_names;
    std::vector<int> pack_pattern_indices;
    /* [0..num_pack_patterns-1] parallel to pack_pattern_names: true if the
     * pattern annotation on this edge is marked allow_multi_fanout */
    std::vector<bool> pack_pattern_allow_multi_fanout;
    bool infer_pattern;

    int switch_type_idx = ARCH_FPGA_UNDEFINED_VAL; /* architecture switch id of the edge - used when flat_routing is enabled */

    // class member functions
  public:
    /// @brief Returns the pin that drives this edge.
    t_pb_graph_pin* input_pin() const { return input_pin_; }

    /// @brief Returns the pin that this edge drives.
    t_pb_graph_pin* output_pin() const { return output_pin_; }

    /// @brief Connects this edge from input_pin to output_pin.
    void set_pins(t_pb_graph_pin* input_pin, t_pb_graph_pin* output_pin) {
        input_pin_ = input_pin;
        output_pin_ = output_pin;
    }

    /// Returns true if this edge implements one bit of a bus-based mux
    /// (<mux bus="true">), whose single select drives every bit of its output bus.
    bool is_bus_mux() const;

    /// Returns the pb_graph_node instance whose mode contains this edge's
    /// bus-based mux, i.e. the node that owns the one shared select.
    /// Only valid when is_bus_mux() is true.
    const t_pb_graph_node* bus_mux_owner() const;

    /// Returns true is this edge is annotated with the given pattern_index
    ///  pattern_index : index of the packing pattern
    bool annotated_with_pattern(int pattern_index) const;

    /// Returns true is this edge is annotated with pattern_index or its pattern
    /// is inferred and a connected output edge is annotated with pattern_index
    ///   pattern_index : index of the packing pattern
    bool belongs_to_pattern(int pattern_index) const;

  private:
    /// The pin that drives this edge
    t_pb_graph_pin* input_pin_ = nullptr;
    /// The pin that this edge drives
    t_pb_graph_pin* output_pin_ = nullptr;
};

struct t_pb_graph_node_power {
    float transistor_cnt_pb_children; /* Total transistor size of this pb */
    float transistor_cnt_interc;      /* Total transistor size of the interconnect in this pb */
    float transistor_cnt_buffers;
};

struct t_pb_graph_pin_power {
    /* Transistor-level Power Properties */
    float C_wire;
    float buffer_size;

    /* Pin-Toggle Power Properties */
    t_pb_graph_pin* scaled_by_pin;
};
