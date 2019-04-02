/*
 Data types used to give architectural hints for the CAD algorithm
 */
#ifndef CAD_TYPES_H
#define CAD_TYPES_H

#include "logic_types.h"
#include "physical_types.h"

struct t_pack_pattern_connections;
struct t_pack_pattern_block {
	int pattern_index; /* index of pattern that this block is a part of */
	const t_pb_type *pb_type; /* pb_type that this block is an instance of */
	t_pack_pattern_connections *connections; /* linked list of connections of logic blocks in pattern */
	int block_id;
};

/* Describes connections of s_pack_pattern_block */
struct t_pack_pattern_connections {
	t_pack_pattern_block *from_block;
	t_pb_graph_pin *from_pin;

	t_pack_pattern_block *to_block;
	t_pb_graph_pin *to_pin;

	t_pack_pattern_connections *next;
};

struct t_pack_patterns {
	char *name; /* name of this logic model pattern */
	int index; /* array index  for pattern*/
	t_pack_pattern_block *root_block; /* root block used by this pattern */
	float base_cost; /* base cost of pattern eg. If a group of logical blocks match a pattern of smaller primitives, that is better than the same group using bigger primitives */
	int num_blocks; /* number of blocks in pattern */
	bool *is_block_optional; /* [0..num_blocks-1] is the block_id in this pattern mandatory or optional to form a molecule */

	bool is_chain; /* Does this pattern chain across logic blocks */
    std::vector<t_pb_graph_pin *> chain_root_pins; /* Array of pointers to logic block input pins that can drive this chain from the preceding logic block */
};

/**
 * Keeps track of locations that a primitive can go to during packing
 * Linked list for easy insertion/deletion
 */
struct t_cluster_placement_primitive {
	t_pb_graph_node *pb_graph_node;
	t_cluster_placement_primitive *next_primitive;
	bool valid;
	float base_cost; /* cost independant of current status of packing */
	float incremental_cost; /* cost dependant on current status of packing */
};

#endif
