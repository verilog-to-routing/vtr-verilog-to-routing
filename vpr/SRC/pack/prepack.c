/* 
Prepacking: Group together technology-mapped netlist blocks before packing.  This gives hints to the packer on what groups of blocks to keep together during packing.
Primary purpose 1) "Forced" packs (eg LUT+FF pair)
                2) Carry-chains


Duties: Find pack patterns in architecture, find pack patterns in netlist.

Author: Jason Luu
March 12, 2012
*/
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "prepack.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "read_xml_arch_file.h"


static int add_pattern_name_to_hash(INOUTP struct s_hash **nhash, INP char *pattern_name, INOUTP int *ncount);
static void discover_pattern_names_in_pb_graph_node(INP t_pb_graph_node *pb_graph_node, INOUTP struct s_hash **nhash, INOUTP int *ncount);

/* Find all packing patterns in architecture */
void alloc_and_load_pack_patterns() {
	int i, ncount;
	struct s_hash **nhash;

	nhash = alloc_hash_table();
	ncount = 0;
	for(i = 0; i < num_types; i++) {
		discover_pattern_names_in_pb_graph_node(type_descriptors[i].pb_graph_head, nhash, &ncount);
	}
	printf("\n\n\n\n\njedit ncount %d\n\n\n\n", ncount);
}



/**
 * Adds pack pattern name to hashtable of pack pattern names.
 */
static int add_pattern_name_to_hash(INOUTP struct s_hash **nhash, INP char *pattern_name, INOUTP int *ncount) {
	struct s_hash *hash_value;

	hash_value = insert_in_hash_table(nhash, pattern_name, *ncount);
	if(hash_value->count == 1) {
		assert(*ncount == hash_value->index);
		(*ncount)++;
	}
	return hash_value->index;
}

/* Locate all pattern names */
static void discover_pattern_names_in_pb_graph_node(INP t_pb_graph_node *pb_graph_node, INOUTP struct s_hash **nhash, INOUTP int *ncount) {
	int i, j, k, m;
	if(pb_graph_node == NULL) {
		return;
	}

	for(i = 0; i < pb_graph_node->num_input_ports; i++) {
		for(j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					add_pattern_name_to_hash(nhash, pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for(j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for(k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				discover_pattern_names_in_pb_graph_node(&pb_graph_node->child_pb_graph_nodes[i][j][k], nhash, ncount);
			}
		}
	}
}










