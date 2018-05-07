/* 
 Data types used to give architectural hints for the CAD algorithm 
 */
#ifndef CAD_TYPES_H
#define CAD_TYPES_H

#include "logic_types.h"
#include "physical_types.h"

//An edge in a pack pattern graph
struct t_pack_pattern_edge {
    int from_node_id = OPEN; //From node's id in t_pack_pattern::nodes
    int to_node_id = OPEN; //To node's id in t_pack_pattern::nodes
    t_pb_graph_pin* from_pin = nullptr; //Driving PB graph pin
    t_pb_graph_pin* to_pin = nullptr; //Sink PB graph pin
};

//A node in a pack pattern graph
struct t_pack_pattern_node {
    t_pack_pattern_node(t_pb_graph_node* node)
        : pb_graph_node(node) {}

    t_pb_graph_node* pb_graph_node = nullptr; //Associated PB graph node

    std::vector<int> in_edge_ids; //Incoming edge ids in t_pack_pattern::edges
    std::vector<int> out_edge_ids; //Outgoing edge ids in t_pack_pattern::edges
};

//A pack pattern represented as a graph
//
//This is usually a strict tree, unless is_chain is true
struct t_pack_pattern {
    //The name of the pack pattern
    std::string name;

    //The root node id of the pattern
    int root_node_id = OPEN;

    //If true represents a chain-like structure which 
    //stretches accross multiple logic blocks
    //
    //Note that if this is a chain, then the graph is not
    //strictly a tree, but the root (top-level) will have both
    //in-coming and out-going edges forming a loop.
    bool is_chain = false; 

    //The set of edges in the pack pattern graph
    std::vector<t_pack_pattern_node> nodes;

    //The set of nodes in the pack pattern graph
    std::vector<t_pack_pattern_edge> edges;
};


//OLD
struct t_pack_pattern_connections;
struct t_pack_pattern_block {
	int pattern_index; /* index of pattern that this block is a part of */
	const t_pb_type *pb_type; /* pb_type that this block is an instance of */
	t_pack_pattern_connections *connections; /* linked list of connections involving this block (as either a from or to block) */
	int block_id;
};

/* Describes connections of t_pack_pattern_block 
 * Note that each t_pack_pattern_block has a unique copy of the connections it is involved with 
 */
struct t_pack_pattern_connections {
	t_pack_pattern_block *from_block;
	t_pb_graph_pin *from_pin;

	t_pack_pattern_block *to_block;
	t_pb_graph_pin *to_pin;

	t_pack_pattern_connections *next; //The next connection in the linked list
};

struct t_pack_patterns {
	char *name; /* name of this logic model pattern */
	int index; /* array index  for pattern*/
	t_pack_pattern_block *root_block; /* root block used by this pattern */
	float base_cost; /* base cost of pattern eg. If a group of logical blocks match a pattern of smaller primitives, that is better than the same group using bigger primitives */
	int num_blocks; /* number of blocks in pattern */
	bool *is_block_optional; /* [0..num_blocks-1] is the block_id in this pattern mandatory or optional to form a molecule */

	bool is_chain; /* Does this pattern chain across logic blocks */
	t_pb_graph_pin *chain_root_pin; /* pointer to logic block input pin that drives this chain from the preceding logic block */	
};

struct t_model_chain_pattern {
	char *name; /* name of this chain of logic */
	t_model *model; /* block associated with chain */
	t_model_ports *input_link_port; /* pointer to port of chain input */
	int inport_link_pin; /* applicable pin of chain input port */
	t_model_ports *output_link_port; /* pointer to port of chain output */
	int outport_link_pin; /* applicable pin of chain output port */
	t_model_chain_pattern *next; /* next chain (linked list) */
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
