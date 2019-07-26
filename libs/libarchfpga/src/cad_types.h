/*
 * Data types used to give architectural hints for the CAD algorithm
 */
#ifndef CAD_TYPES_H
#define CAD_TYPES_H

#include "logic_types.h"
#include "physical_types.h"

struct t_pack_pattern_connections;

/**
 * Data structure used to define the structure of a pack pattern that is defined in the architecture file
 *
 * For example: for a pack pattern of a 6-LUT and a FF, each of those primitives will be defined by
 * a t_pack_pattern_block and each of them will have one t_pack_pattern_connections.
 *
 * Data members:
 *      pattern_index : the id of the pattern this block is part of (matches "index" in t_pack_patterns)
 *      pb_type       : the pb_type (primitive) that this block represents (Ex. LUT, Adder, FF, etc.)
 *      connections   : linked list of connections between this t_pack_pattern_block and other
 *                      t_pack_pattern_blocks in this pack pattern as defined in the architecture
 *      block_id      : the id of this t_pack_pattern_block within its pack pattern, used to access
 *                      is_block_optional array in t_pack_patterns and also to access the atom_block_ids
 *                      vector in the t_pack_molecule data structure.
 */
struct t_pack_pattern_block {
    int pattern_index;
    const t_pb_type* pb_type;
    t_pack_pattern_connections* connections;
    int block_id;
};

/**
 * Describes a linked list of connections of a t_pack_pattern_block
 *
 * Data members:
 *      from_block : block driving this connection
 *      from_pin   : specific pin in the from_block driving the connection
 *      to_block   : block driven by this connection
 *      to_pin     : specific pin in the to_block driven by this connection
 *      next       : next connection in the linked list
 */
struct t_pack_pattern_connections {
    t_pack_pattern_block* from_block;
    t_pb_graph_pin* from_pin;

    t_pack_pattern_block* to_block;
    t_pb_graph_pin* to_pin;

    t_pack_pattern_connections* next;
};

/**
 * Describes a pack pattern defined in the architecture. A pack pattern is an
 * architectural concept that defines a pattern of highly constrained and/or desirable
 * arrangement of primitives that exists within one logic cluster (Ex. CLB).
 *
 * For example: A pack pattern could be a 6-LUT and a FF. Where the architecture
 * file gives a hint for the packer to pack a 6-LUT that is followed by a FF in the
 * netlist together in one logic element. This helps the packer to achieve high
 * packing density. Another example, is a carry chain where the adders in the netlist
 * should be packed together to be able to route Cout to Cin connections using the
 * dedicated wiring in the architecture.
 *
 * Data members:
 *      name              : name given to the pack pattern in the architecture file
 *      index             : id of the pack pattern in the list_of_pack_patterns array defined in the packer code
 *      root_block        : the block defining the starting point of this pattern. For example: for
 *                          a carry chain pattern, it is the primitive driven by a cluster input pin.
 *      base_cost         : the sum of the primitive base costs of all the primitives in this pack pattern.
 *                          The primitive base cost is defined by compute_primitive_base_cost in vpr_utils.cpp 
 *      num_blocks        : total number of primitives in this pack pattern
 *      is_block_optional : [0..num_blocks-1] is true if the t_pack_pattern_block defined by block_id
 *                          is not mandatory for this pack pattern to be formed. For example, in a carry
 *                          chain pack pattern, the first adder primitive (root_block) is mandatory to
 *                          form the pattern, but every adder primitive after that is optional as the case
 *                          when forming a short adder chain.
 *      is_chain          : does this pack pattern go across clusters. For example, carry chains can normally cross
 *                          between logic blocks.
 *      chain_root_pins   : this is only non-empty for pack_patterns with is_chain set. It points to a specific
 *                          pin of the root_block primitive (Ex. cin of an adder primitive) that is directly
 *                          connected to a cluster-level block pin that can be drive from the preceding cluster.
 *                          The first dimension size is greater than one if the cluster has more than one chain
 *                          of this type. For example, an architecture with two independent adder carry chains
 *                          with different cluster level Cin and Cout pins. The second dimension size is greater
 *                          than one if cin of the cluster can reach more than one adder. This means that there is
 *                          a mux in front of the cin pin of one or more adders in the middle of this chain that
 *                          chooses between the cout of the preceding adder and the cin pin of the cluster. Which will
 *                          give more freedom to the packer when placing small adders that are driven by a constant
 *                          net (gnd/vdd)  [0...num_of_chains][0...num_of_tie_offs]
 */
struct t_pack_patterns {
    char* name;
    int index;
    float base_cost;

    t_pack_pattern_block* root_block;

    int num_blocks;
    bool* is_block_optional;

    bool is_chain;
    std::vector<std::vector<t_pb_graph_pin*>> chain_root_pins;

    // default constructor initializing to an invalid pack pattern
    t_pack_patterns() {
        name = nullptr;
        index = -1;
        root_block = nullptr;
        base_cost = 0;
        num_blocks = 0;
        is_block_optional = nullptr;
        is_chain = false;
    }
};

/**
 * Keeps track of locations that a primitive can go to during packing
 * Linked list for easy insertion/deletion
 */
struct t_cluster_placement_primitive {
    t_pb_graph_node* pb_graph_node;
    t_cluster_placement_primitive* next_primitive;
    bool valid;
    float base_cost;        /* cost independent of current status of packing */
    float incremental_cost; /* cost dependant on current status of packing */
};

#endif
