#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "physical_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "place_macro.h"
#include "string.h"
#include "pack_types.h"


/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */

/* This defines the maximum string length that could be parsed by functions  *
 * in vpr_utils.                                                             */
#define MAX_STRING_LEN 128

/******************** File-scope variables delcarations **********************/

/* These three mappings are needed since there are two different netlist *
 * conventions - in the cluster level, ports and port pins are used      *
 * while in the post-pack level, block pins are used. The reason block   *
 * type is used instead of blocks is to save memories.                   */

 /* f_port_from_blk_pin array allow us to quickly find what port a block *
 * pin corresponds to.                                                   *
 * [0...num_types-1][0...blk_pin_count-1]                                *
 *                                                                       */
static int ** f_port_from_blk_pin = NULL;

/* f_port_pin_from_blk_pin array allow us to quickly find what port pin a*
 * block pin corresponds to.                                             *
 * [0...num_types-1][0...blk_pin_count-1]                                */
static int ** f_port_pin_from_blk_pin = NULL;

/* f_port_pin_to_block_pin array allows us to quickly find what block    *
 * pin a port pin corresponds to.                                        *
 * [0...num_types-1][0...num_ports-1][0...num_port_pins-1]               */
static int *** f_blk_pin_from_port_pin = NULL;


/******************** Subroutine declarations ********************************/

/* Allocates and loads f_port_from_blk_pin and f_port_pin_from_blk_pin   *
 * arrays.                                                               *
 * The arrays are freed in free_placement_structs()                      */
static void alloc_and_load_port_pin_from_blk_pin(void);

/* Allocates and loads blk_pin_from_port_pin array.                      *
 * The arrays are freed in free_placement_structs()                      */
static void alloc_and_load_blk_pin_from_port_pin(void);

/* Go through all the ports in all the blocks to find the port that has the same   *
 * name as port_name and belongs to the block type that has the name pb_type_name. *
 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
 * Otherwise, mark down all the pins in that port.                                 */
static void mark_direct_of_ports (int idirect, int direct_type, char * pb_type_name, 
		char * port_name, int end_pin_index, int start_pin_index, char * src_string, 
		int line, int ** idirect_from_blk_pin, int ** direct_type_from_blk_pin);

/* Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in    *
 * direct_type_from_blk_pin with direct_type from start_pin_index to               *
 * end_pin_index.                                                                  */
static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype, 
		int iport, int ** idirect_from_blk_pin, int idirect, 
		int ** direct_type_from_blk_pin, int direct_type, int line, char * src_string);

static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin ** pb_graph_pin_lookup_from_index, t_pb_graph_node *pb_graph_node);

/******************** Subroutine definitions *********************************/

/**
 * print tabs given number of tabs to file
 */
void print_tabs(FILE * fpout, int num_tab) {

	int i;
	for (i = 0; i < num_tab; i++) {
		fprintf(fpout, "\t");
	}
}

/* Points the grid structure back to the blocks list */
void sync_grid_to_blocks(INP int L_num_blocks,
		INP const struct s_block block_list[], INP int L_nx, INP int L_ny,
		INOUTP struct s_grid_tile **L_grid) {

	int i, j, k;

	/* Reset usage and allocate blocks list if needed */
	for (j = 0; j <= (L_ny + 1); ++j) {
		for (i = 0; i <= (L_nx + 1); ++i) {
			L_grid[i][j].usage = 0;
			if (L_grid[i][j].type) {
				/* If already allocated, leave it since size doesn't change */
				if (NULL == L_grid[i][j].blocks) {
					L_grid[i][j].blocks = (int *) my_malloc(
							sizeof(int) * L_grid[i][j].type->capacity);

					/* Set them as unconnected */
					for (k = 0; k < L_grid[i][j].type->capacity; ++k) {
						L_grid[i][j].blocks[k] = EMPTY;
					}
				}
			}
		}
	}

	/* Go through each block */
	for (i = 0; i < L_num_blocks; ++i) {
		/* Check range of block coords */
		if (block[i].x < 0 || block[i].y < 0
				|| (block[i].x + block[i].type->width - 1) > (L_nx + 1)
				|| (block[i].y + block[i].type->height - 1) > (L_ny + 1)
				|| block[i].z < 0 || block[i].z > (block[i].type->capacity)) {
			vpr_printf_error(__FILE__, __LINE__,
					"Block %d is at invalid location (%d, %d, %d).\n", 
					i, block[i].x, block[i].y, block[i].z);
			exit(1);
		}

		/* Check types match */
		if (block[i].type != L_grid[block[i].x][block[i].y].type) {
			vpr_printf_error(__FILE__, __LINE__,
					"A block is in a grid location (%d x %d) with a conflicting type.\n", 
					block[i].x, block[i].y);
			exit(1);
		}

		/* Check already in use */
		if ((EMPTY != L_grid[block[i].x][block[i].y].blocks[block[i].z])
				&& (INVALID != L_grid[block[i].x][block[i].y].blocks[block[i].z])) {
			vpr_printf_error(__FILE__, __LINE__,
					"Location (%d, %d, %d) is used more than once.\n", 
					block[i].x, block[i].y, block[i].z);
			exit(1);
		}

		if (L_grid[block[i].x][block[i].y].width_offset != 0 || L_grid[block[i].x][block[i].y].height_offset != 0) {
			vpr_printf_error(__FILE__, __LINE__,
					"Large block not aligned in placment for block %d at (%d, %d, %d).",
					i, block[i].x, block[i].y, block[i].z);
			exit(1);
		}

		/* Set the block */
		for (int width = 0; width < block[i].type->width; ++width) {
			for (int height = 0; height < block[i].type->height; ++height) {
				L_grid[block[i].x + width][block[i].y + height].blocks[block[i].z] = i;
				L_grid[block[i].x + width][block[i].y + height].usage++;
				assert(L_grid[block[i].x + width][block[i].y + height].width_offset == width);
				assert(L_grid[block[i].x + width][block[i].y + height].height_offset == height);
			}
		}
	}
}

boolean is_opin(int ipin, t_type_ptr type) {

	/* Returns TRUE if this clb pin is an output, FALSE otherwise. */

	int iclass;

	iclass = type->pin_class[ipin];

	if (type->class_inf[iclass].type == DRIVER)
		return (TRUE);
	else
		return (FALSE);
}


/* Each node in the pb_graph for a top-level pb_type can be uniquely identified 
 * by its pins. Since the pins in a cluster of a certain type are densely indexed,
 * this function will find the pin index (int pin_count_in_cluster) of the first 
 * pin for a given pb_graph_node, and use this index value as unique identifier 
 * for the node.
 */
int get_unique_pb_graph_node_id(t_pb_graph_node *pb_graph_node) {
	t_pb_graph_pin first_input_pin;
	t_pb_graph_pin first_output_pin;
	int node_id;
	
	if (pb_graph_node->num_input_pins != 0) {
		/* If input port exists on this node, return the index of the first
		 * input pin as node_id.
		 */
		first_input_pin = pb_graph_node->input_pins[0][0];
		node_id = first_input_pin.pin_count_in_cluster;
		return node_id;
	}
	else {
		/* If no input port exists on node, then return the index of the first
		 * output pin. Every pb_node is guaranteed to have at least an input or
		 * output pin.
		 */
		first_output_pin = pb_graph_node->output_pins[0][0];
		node_id = first_output_pin.pin_count_in_cluster;
		return node_id;
	}
}


void get_class_range_for_block(INP int iblk, 
		OUTP int *class_low,
		OUTP int *class_high) {

	/* Assumes that the placement has been done so each block has a set of pins allocated to it */
	t_type_ptr type;

	type = block[iblk].type;
	assert(type->num_class % type->capacity == 0);
	*class_low = block[iblk].z * (type->num_class / type->capacity);
	*class_high = (block[iblk].z + 1) * (type->num_class / type->capacity) - 1;
}

int get_max_primitives_in_pb_type(t_pb_type *pb_type) {

	int i, j;
	int max_size, temp_size;
	if (pb_type->modes == 0) {
		max_size = 1;
	} else {
		max_size = 0;
		temp_size = 0;
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				temp_size += pb_type->modes[i].pb_type_children[j].num_pb
						* get_max_primitives_in_pb_type(
								&pb_type->modes[i].pb_type_children[j]);
			}
			if (temp_size > max_size) {
				max_size = temp_size;
			}
		}
	}
	return max_size;
}

/* finds maximum number of nets that can be contained in pb_type, this is bounded by the number of driving pins */
int get_max_nets_in_pb_type(const t_pb_type *pb_type) {

	int i, j;
	int max_nets, temp_nets;
	if (pb_type->modes == 0) {
		max_nets = pb_type->num_output_pins;
	} else {
		max_nets = 0;
		for (i = 0; i < pb_type->num_modes; i++) {
			temp_nets = 0;
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				temp_nets += pb_type->modes[i].pb_type_children[j].num_pb
						* get_max_nets_in_pb_type(
								&pb_type->modes[i].pb_type_children[j]);
			}
			if (temp_nets > max_nets) {
				max_nets = temp_nets;
			}
		}
	}
	if (pb_type->parent_mode == NULL) {
		max_nets += pb_type->num_input_pins + pb_type->num_output_pins
				+ pb_type->num_clock_pins;
	}
	return max_nets;
}

int get_max_depth_of_pb_type(t_pb_type *pb_type) {

	int i, j;
	int max_depth, temp_depth;
	max_depth = pb_type->depth;
	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			temp_depth = get_max_depth_of_pb_type(
					&pb_type->modes[i].pb_type_children[j]);
			if (temp_depth > max_depth) {
				max_depth = temp_depth;
			}
		}
	}
	return max_depth;
}

/**
 * given a primitive type and a logical block, is the mapping legal
 */
boolean primitive_type_feasible(int iblk, const t_pb_type *cur_pb_type) {

	t_model_ports *port;
	int i, j;
	boolean second_pass;

	if (cur_pb_type == NULL) {
		return FALSE;
	}

	/* check if ports are big enough */
	port = logical_block[iblk].model->inputs;
	second_pass = FALSE;
	while (port || !second_pass) {
		/* TODO: This is slow if the number of ports are large, fix if becomes a problem */
		if (!port) {
			second_pass = TRUE;
			port = logical_block[iblk].model->outputs;
		}
		for (i = 0; i < cur_pb_type->num_ports; i++) {
			if (cur_pb_type->ports[i].model_port == port) {
				for (j = cur_pb_type->ports[i].num_pins; j < port->size; j++) {
					if (port->dir == IN_PORT && !port->is_clock) {
						if (logical_block[iblk].input_nets[port->index][j] != OPEN) {
							return FALSE;
						}
					} else if (port->dir == OUT_PORT) {
						if (logical_block[iblk].output_nets[port->index][j] != OPEN) {
							return FALSE;
						}
					} else {
						assert(port->dir == IN_PORT && port->is_clock);
						assert(j == 0);
						if (logical_block[iblk].clock_net != OPEN) {
							return FALSE;
						}
					}
				}
				break;
			}
		}
		if (i == cur_pb_type->num_ports) {
			if ((logical_block[iblk].model->inputs != NULL && !second_pass)
					|| (logical_block[iblk].model->outputs != NULL
							&& second_pass)) {
				/* physical port not found */
				return FALSE;
			}
		}
		if (port) {
			port = port->next;
		}
	}
	return TRUE;
}


/**
 * Return pb_graph_node pin from model port and pin
 *  NULL if not found
 */
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(t_model_ports *model_port, int model_pin, t_pb_graph_node *pb_graph_node) {
	int i;

	if(model_port->dir == IN_PORT) {
		if(model_port->is_clock == FALSE) {
			for (i = 0; i < pb_graph_node->num_input_ports; i++) {
				if (pb_graph_node->input_pins[i][0].port->model_port == model_port) {
					if(pb_graph_node->num_input_pins[i] > model_pin) {
						return &pb_graph_node->input_pins[i][model_pin];
					} else {
						return NULL;
					}
				}
			}
		} else {
			for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
				if (pb_graph_node->clock_pins[i][0].port->model_port == model_port) {
					if(pb_graph_node->num_clock_pins[i] > model_pin) {
						return &pb_graph_node->clock_pins[i][model_pin];
					} else {
						return NULL;
					}
				}
			}
		}
	} else {
		assert(model_port->dir == OUT_PORT);
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			if (pb_graph_node->output_pins[i][0].port->model_port == model_port) {
				if(pb_graph_node->num_output_pins[i] > model_pin) {
					return &pb_graph_node->output_pins[i][model_pin];
				} else {
					return NULL;
				}
			}
		}
	}
	return NULL;
}

t_pb_graph_pin* get_pb_graph_node_pin_from_g_atoms_nlist_net(int inet, int ipin) {

	int ilogical_block;
	t_model_ports *port;

	ilogical_block = g_atoms_nlist.net[inet].pins[ipin].block;

	assert(ilogical_block != OPEN);
	if(logical_block[ilogical_block].pb == NULL) {
		/* This net has not been packed yet thus pb_graph_pin does not exist */
		return NULL;
	}

	if(ipin > 0) {
		port = logical_block[ilogical_block].model->inputs;
		if(g_atoms_nlist.net[inet].is_global) {
			while(port != NULL) {
				if(port->is_clock) {
					if(port->index == g_atoms_nlist.net[inet].pins[ipin].block_port) {
						break;
					}
				}
				port = port->next;
			}
		} else {
			while(port != NULL) {
				if(!port->is_clock) {
					if(port->index == g_atoms_nlist.net[inet].pins[ipin].block_port) {
						break;
					}
				}
				port = port->next;
			}
		}
	} else {
		/* This is an output pin */
		port = logical_block[ilogical_block].model->outputs;
		while(port != NULL) {
			if(port->index == g_atoms_nlist.net[inet].pins[ipin].block_pin) {
				break;
			}
			port = port->next;
		}
	}

	assert(port != NULL);
	return get_pb_graph_node_pin_from_model_port_pin(port, g_atoms_nlist.net[inet].pins[ipin].block_pin, logical_block[ilogical_block].pb->pb_graph_node);
}

t_pb_graph_pin* get_pb_graph_node_pin_from_g_clbs_nlist_net(int inet, int ipin) {

	int iblock, target_pin;

	iblock = g_clbs_nlist.net[inet].pins[ipin].block;
	target_pin =  g_clbs_nlist.net[inet].pins[ipin].block_pin;
	
	return get_pb_graph_node_pin_from_block_pin(iblock, target_pin);
}

t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(int iblock, int ipin) {

	int i, count;
	const t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	
	pb_graph_node = block[iblock].pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* If this is post-placed, then the ipin may have been shuffled up by the z * num_pins, 
	bring it back down to 0..num_pins-1 range for easier analysis */
	ipin %= (pb_type->num_input_pins + pb_type->num_output_pins + pb_type->num_clock_pins);
		
	if(ipin < pb_type->num_input_pins) {
		count = ipin;
		for(i = 0; i < pb_graph_node->num_input_ports; i++) {
			if(count - pb_graph_node->num_input_pins[i] >= 0) {
				count -= pb_graph_node->num_input_pins[i];
			} else {
				return &pb_graph_node->input_pins[i][count];
			}
		}
	} else if (ipin < pb_type->num_input_pins + pb_type->num_output_pins) {
		count = ipin - pb_type->num_input_pins;
		for(i = 0; i < pb_graph_node->num_output_ports; i++) {
			if(count - pb_graph_node->num_output_pins[i] >= 0) {
				count -= pb_graph_node->num_output_pins[i];
			} else {
				return &pb_graph_node->output_pins[i][count];
			}
		}
	} else {
		count = ipin - pb_type->num_input_pins - pb_type->num_output_pins;
		for(i = 0; i < pb_graph_node->num_clock_ports; i++) {
			if(count - pb_graph_node->num_clock_pins[i] >= 0) {
				count -= pb_graph_node->num_clock_pins[i];
			} else {
				return &pb_graph_node->clock_pins[i][count];
			}
		}
	}
	assert(0);
	return NULL;
}

/* Recusively visit through all pb_graph_nodes to populate pb_graph_pin_lookup_from_index */
static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin ** pb_graph_pin_lookup_from_index, t_pb_graph_node *pb_graph_node) {
	for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->input_pins[iport][ipin];
			assert(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == NULL);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->output_pins[iport][ipin];
			assert(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == NULL);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->clock_pins[iport][ipin];
			assert(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == NULL);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}

	for(int imode = 0; imode < pb_graph_node->pb_type->num_modes; imode++) {
		for(int ipb_type = 0; ipb_type < pb_graph_node->pb_type->modes[imode].num_pb_type_children; ipb_type++) {
			for(int ipb = 0; ipb < pb_graph_node->pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
				load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_index, &pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb]);
			}
		}
	}
}

/* Create a lookup that returns a pb_graph_pin pointer given the pb_graph_pin index */
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_type_ptr type) {
	t_pb_graph_pin** pb_graph_pin_lookup_from_type;

	t_pb_graph_node *pb_graph_head = type->pb_graph_head;
	if(pb_graph_head == NULL) {
		return NULL;
	}
	int num_pins = pb_graph_head->total_pb_pins;

	pb_graph_pin_lookup_from_type = new t_pb_graph_pin* [num_pins];
	for(int id = 0; id < num_pins; id++) {
		pb_graph_pin_lookup_from_type[id] = NULL;
	}

	load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_type, pb_graph_head);

	for(int id = 0; id < num_pins; id++) {
		assert(pb_graph_pin_lookup_from_type[id] != NULL);
	}

	return pb_graph_pin_lookup_from_type;
}

/* Free pb_graph_pin lookup array */
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type) {
	if(pb_graph_pin_lookup_from_type == NULL) {
		return;
	}
	delete [] pb_graph_pin_lookup_from_type;
}

/**
 * Determine cost for using primitive within a complex block, should use primitives of low cost before selecting primitives of high cost
 For now, assume primitives that have a lot of pins are scarcer than those without so use primitives with less pins before those with more
 */
float compute_primitive_base_cost(INP t_pb_graph_node *primitive) {

	return (primitive->pb_type->num_input_pins
			+ primitive->pb_type->num_output_pins
			+ primitive->pb_type->num_clock_pins);
}

int num_ext_inputs_logical_block(int iblk) {

	/* Returns the number of input pins on this logical_block that must be hooked *
	 * up through external interconnect.  That is, the number of input    *
	 * pins used - the number which connect (internally) to the outputs.   */

	int ext_inps, output_net, ipin, opin;

	t_model_ports *port, *out_port;

	/* TODO: process to get ext_inps is slow, should cache in lookup table */
	ext_inps = 0;
	port = logical_block[iblk].model->inputs;
	while (port) {
		if (port->is_clock == FALSE) {
			for (ipin = 0; ipin < port->size; ipin++) {
				if (logical_block[iblk].input_nets[port->index][ipin] != OPEN) {
					ext_inps++;
				}
				out_port = logical_block[iblk].model->outputs;
				while (out_port) {
					for (opin = 0; opin < out_port->size; opin++) {
						output_net =
								logical_block[iblk].output_nets[out_port->index][opin];
						if (output_net == OPEN)
							continue;
						/* TODO: I could speed things up a bit by computing the number of inputs *
						 * and number of external inputs for each logic logical_block at the start of   *
						 * clustering and storing them in arrays.  Look into if speed is a      *
						 * problem.                                                             */

						if (logical_block[iblk].input_nets[port->index][ipin]
								== output_net) {
							ext_inps--;
							break;
						}
					}
					out_port = out_port->next;
				}
			}
		}
		port = port->next;
	}

	assert(ext_inps >= 0);

	return (ext_inps);
}


void free_cb(t_pb *pb) {

	const t_pb_type * pb_type;
	int i, total_nodes;

	if (pb == NULL) {
		return;
	}

	pb_type = pb->pb_graph_node->pb_type;

	total_nodes = pb->pb_graph_node->total_pb_pins + pb_type->num_input_pins
			+ pb_type->num_output_pins + pb_type->num_clock_pins;

	if (pb->rr_graph != NULL) {
		for (i = 0; i < total_nodes; i++) {
			if (pb->rr_graph[i].edges != NULL) {
				free(pb->rr_graph[i].edges);
			}
			if (pb->rr_graph[i].switches != NULL) {
				free(pb->rr_graph[i].switches);
			}
		}
		free(pb->rr_graph);
	}	
	free_pb(pb);
}

void free_pb(t_pb *pb) {

	const t_pb_type * pb_type;
	int i, j, mode;
	struct s_linked_vptr *revalid_molecule;
	t_pack_molecule *cur_molecule;

	pb_type = pb->pb_graph_node->pb_type;

	if (pb_type->blif_model == NULL) {
		mode = pb->mode;
		for (i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != NULL; i++) {
			for (j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb	&& pb->child_pbs[i] != NULL; j++) {
				if (pb->child_pbs[i][j].name != NULL || pb->child_pbs[i][j].child_pbs != NULL) {
					free_pb(&pb->child_pbs[i][j]);
				}
			}
			if (pb->child_pbs[i])
				free(pb->child_pbs[i]);
		}
		if (pb->child_pbs)
			free(pb->child_pbs);
		pb->child_pbs = NULL;

		if (pb->local_nets != NULL) {
			for (i = 0; i < pb->num_local_nets; i++) {
				free(pb->local_nets[i].node_block);
				free(pb->local_nets[i].node_block_port);
				free(pb->local_nets[i].node_block_pin);
				if (pb->local_nets[i].name != NULL) {
					free(pb->local_nets[i].name);
				}
			}
			free(pb->local_nets);
			pb->local_nets = NULL;
		}

		if (pb->rr_node_to_pb_mapping != NULL) {
			free(pb->rr_node_to_pb_mapping);
			pb->rr_node_to_pb_mapping = NULL;
		}
		
		if (pb->name)
			free(pb->name);
		pb->name = NULL;
	} else {
		/* Primitive */
		if (pb->name)
			free(pb->name);
		pb->name = NULL;
		if (pb->lut_pin_remap) {
			free(pb->lut_pin_remap);
		}
		pb->lut_pin_remap = NULL;
		if (pb->logical_block != EMPTY && pb->logical_block != INVALID && logical_block != NULL) {
			logical_block[pb->logical_block].clb_index = NO_CLUSTER;
			logical_block[pb->logical_block].pb = NULL;
			/* If any molecules were marked invalid because of this logic block getting packed, mark them valid */
			revalid_molecule = logical_block[pb->logical_block].packed_molecules;
			while (revalid_molecule != NULL) {
				cur_molecule = (t_pack_molecule*)revalid_molecule->data_vptr;
				if (cur_molecule->valid == FALSE) {
					for (i = 0; i < get_array_size_of_molecule(cur_molecule); i++) {
						if (cur_molecule->logical_block_ptrs[i] != NULL) {
							if (cur_molecule->logical_block_ptrs[i]->clb_index != OPEN) {
								break;
							}
						}
					}
					/* All logical blocks are open for this molecule, place back in queue */
					if (i == get_array_size_of_molecule(cur_molecule)) {
						cur_molecule->valid = TRUE;	
					}
				}
				revalid_molecule = revalid_molecule->next;
			}
		}
		pb->logical_block = OPEN;
	}
	free_pb_stats(pb);
}

void free_pb_stats(t_pb *pb) {

	int i;
	t_pb_graph_node *pb_graph_node = pb->pb_graph_node;

	if(pb->pb_stats == NULL) {
		return;
	}

	pb->pb_stats->gain.clear();
	pb->pb_stats->timinggain.clear();
	pb->pb_stats->sharinggain.clear();
	pb->pb_stats->hillgain.clear();
	pb->pb_stats->connectiongain.clear();
	pb->pb_stats->num_pins_of_net_in_pb.clear();
	
	if(pb->pb_stats->marked_blocks != NULL) {
		for (i = 0; i < pb_graph_node->num_input_pin_class; i++) {
			free(pb->pb_stats->input_pins_used[i]);
		}
		free(pb->pb_stats->input_pins_used);
		delete [] pb->pb_stats->lookahead_input_pins_used;
		for (i = 0; i < pb_graph_node->num_output_pin_class; i++) {
			free(pb->pb_stats->output_pins_used[i]);
		}
		free(pb->pb_stats->output_pins_used);
		delete [] pb->pb_stats->lookahead_output_pins_used;
		free(pb->pb_stats->feasible_blocks);
		free(pb->pb_stats->marked_nets);
		free(pb->pb_stats->marked_blocks);
	}
	pb->pb_stats->marked_blocks = NULL;
	if(pb->pb_stats->transitive_fanout_candidates != NULL) {
		delete pb->pb_stats->transitive_fanout_candidates;
	};
	delete pb->pb_stats;
	pb->pb_stats = NULL;
}

int ** alloc_and_load_net_pin_index() {

	/* Allocates and loads net_pin_index array, this array allows us to quickly   *
	 * find what pin on the net a block pin corresponds to. Returns the pointer   *
	 * to the 2D net_pin_index array.                                             */

	unsigned int netpin, inet;
	int blk, iblk, ipin, itype, **temp_net_pin_index, max_pins_per_clb = 0;
	t_type_ptr type;

	/* Compute required size. */
	for (itype = 0; itype < num_types; itype++)
		max_pins_per_clb = max(max_pins_per_clb, type_descriptors[itype].num_pins);
	
	/* Allocate for maximum size. */
	temp_net_pin_index = (int **) alloc_matrix(0, num_blocks - 1, 0,
				max_pins_per_clb - 1, sizeof(int));

	/* Initialize values to OPEN */
	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		for (ipin = 0; ipin < type->num_pins; ipin++) {
			temp_net_pin_index[iblk][ipin] = OPEN;
		}
	}

	/* Load the values */
	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global)
			continue;
		for (netpin = 0; netpin < g_clbs_nlist.net[inet].pins.size(); netpin++) {
			blk =g_clbs_nlist.net[inet].pins[netpin].block;
			temp_net_pin_index[blk][g_clbs_nlist.net[inet].pins[netpin].block_pin] = netpin;
		}
	}

	/* Returns the pointers to the 2D array. */
	return temp_net_pin_index;
}

/***************************************************************************************
  Y.G.THIEN
  29 AUG 2012

 * The following functions maps the block pins indices for all block types to the      *
 * corresponding port indices and port_pin indices. This is necessary since there are  *
 * different netlist conventions - in the cluster level, ports and port pins are used  *
 * while in the post-pack level, block pins are used.                                  *
 *                                                                                     *
 ***************************************************************************************/

void get_port_pin_from_blk_pin(int blk_type_index, int blk_pin, int * port,
		int * port_pin) {

	/* These two mappings are needed since there are two different netlist   *
	 * conventions - in the cluster level, ports and port pins are used      *
	 * while in the post-pack level, block pins are used. The reason block   *
	 * type is used instead of blocks is that the mapping is the same for    *
	 * blocks belonging to the same block type.                              *
	 *                                                                       *
	 * f_port_from_blk_pin array allow us to quickly find what port a        *
	 * block pin corresponds to.                                             *
	 * [0...num_types-1][0...blk_pin_count-1]                                *
	 *                                                                       *
	 * f_port_pin_from_blk_pin array allow us to quickly find what port      *
	 * pin a block pin corresponds to.                                       *
	 * [0...num_types-1][0...blk_pin_count-1]                                */

	/* If either one of the arrays is not allocated and loaded, it is        *
	 * corrupted, so free both of them.                                      */ 
	if ((f_port_from_blk_pin == NULL && f_port_pin_from_blk_pin != NULL)
		|| (f_port_from_blk_pin != NULL && f_port_pin_from_blk_pin == NULL)){
		free_port_pin_from_blk_pin();
	}
	
	/* If the arrays are not allocated and loaded, allocate it.              */ 
	if (f_port_from_blk_pin == NULL && f_port_pin_from_blk_pin == NULL) {
		alloc_and_load_port_pin_from_blk_pin();
	}
	
	/* Return the port and port_pin for the pin.                             */
	*port = f_port_from_blk_pin[blk_type_index][blk_pin];
	*port_pin = f_port_pin_from_blk_pin[blk_type_index][blk_pin];

}

void free_port_pin_from_blk_pin(void) {

	/* Frees the f_port_from_blk_pin and f_port_pin_from_blk_pin arrays.     *
	 *                                                                       *
	 * This function is called when the file-scope arrays are corrupted.     *
	 * Otherwise, the arrays are freed in free_placement_structs()           */

	int itype;
	
	if (f_port_from_blk_pin != NULL) {
		for (itype = 1; itype < num_types; itype++) {
			free(f_port_from_blk_pin[itype]);
		}
		free(f_port_from_blk_pin);
		
		f_port_from_blk_pin = NULL;
	}

	if (f_port_pin_from_blk_pin != NULL) {
		for (itype = 1; itype < num_types; itype++) {
			free(f_port_pin_from_blk_pin[itype]);
		}
		free(f_port_pin_from_blk_pin);
		
		f_port_pin_from_blk_pin = NULL;
	}

}

static void alloc_and_load_port_pin_from_blk_pin(void) {
	
	/* Allocates and loads f_port_from_blk_pin and f_port_pin_from_blk_pin   *
	 * arrays.                                                               *
	 *                                                                       *
	 * The arrays are freed in free_placement_structs()                      */

	int ** temp_port_from_blk_pin = NULL;
	int ** temp_port_pin_from_blk_pin = NULL;
	int itype, iblk_pin, iport, iport_pin;
	int blk_pin_count, num_port_pins, num_ports;

	/* Allocate and initialize the values to OPEN (-1). */
	temp_port_from_blk_pin = (int **) my_malloc(num_types* sizeof(int*));
	temp_port_pin_from_blk_pin = (int **) my_malloc(num_types* sizeof(int*));
	for (itype = 1; itype < num_types; itype++) {
		
		blk_pin_count = type_descriptors[itype].num_pins;

		temp_port_from_blk_pin[itype] = (int *) my_malloc(blk_pin_count* sizeof(int));
		temp_port_pin_from_blk_pin[itype] = (int *) my_malloc(blk_pin_count* sizeof(int));

		for (iblk_pin = 0; iblk_pin < blk_pin_count; iblk_pin++) {
			temp_port_from_blk_pin[itype][iblk_pin] = OPEN;
			temp_port_pin_from_blk_pin[itype][iblk_pin] = OPEN;
		}
	}

	/* Load the values */
	/* itype starts from 1 since type_descriptors[0] is the EMPTY_TYPE. */
	for (itype = 1; itype < num_types; itype++) {

		blk_pin_count = 0;
		num_ports = type_descriptors[itype].pb_type->num_ports;

		for (iport = 0; iport < num_ports; iport++) {

			num_port_pins = type_descriptors[itype].pb_type->ports[iport].num_pins;

			for (iport_pin = 0; iport_pin < num_port_pins; iport_pin++) {

				temp_port_from_blk_pin[itype][blk_pin_count] = iport;
				temp_port_pin_from_blk_pin[itype][blk_pin_count] = iport_pin;
				blk_pin_count++;
			}
		}
	}

	/* Sets the file_scope variables to point at the arrays. */
	f_port_from_blk_pin = temp_port_from_blk_pin;
	f_port_pin_from_blk_pin = temp_port_pin_from_blk_pin;
}

void get_blk_pin_from_port_pin(int blk_type_index, int port,int port_pin, 
		int * blk_pin) {

	/* This mapping is needed since there are two different netlist          *
	 * conventions - in the cluster level, ports and port pins are used      *
	 * while in the post-pack level, block pins are used. The reason block   *
	 * type is used instead of blocks is to save memories.                   *
	 *                                                                       *
	 * f_port_pin_to_block_pin array allows us to quickly find what block    *
	 * pin a port pin corresponds to.                                        *
	 * [0...num_types-1][0...num_ports-1][0...num_port_pins-1]               */

	/* If the array is not allocated and loaded, allocate it.                */ 
	if (f_blk_pin_from_port_pin == NULL) {
		alloc_and_load_blk_pin_from_port_pin();
	}

	/* Return the port and port_pin for the pin.                             */
	*blk_pin = f_blk_pin_from_port_pin[blk_type_index][port][port_pin];

}

void free_blk_pin_from_port_pin(void) {

	/* Frees the f_blk_pin_from_port_pin array.               *
	 *                                                        *
	 * This function is called when the arrays are freed in   *
	 * free_placement_structs()                               */

	int itype, iport, num_ports;
	
	if (f_blk_pin_from_port_pin != NULL) {
		
		for (itype = 1; itype < num_types; itype++) {
			num_ports = type_descriptors[itype].pb_type->num_ports;
			for (iport = 0; iport < num_ports; iport++) {
				free(f_blk_pin_from_port_pin[itype][iport]);
			}
			free(f_blk_pin_from_port_pin[itype]);
		}
		free(f_blk_pin_from_port_pin);
		
		f_blk_pin_from_port_pin = NULL;
	}

}

static void alloc_and_load_blk_pin_from_port_pin(void) {

	/* Allocates and loads blk_pin_from_port_pin array.                      *
	 *                                                                       *
	 * The arrays are freed in free_placement_structs()                      */

	int *** temp_blk_pin_from_port_pin = NULL;
	int itype, iport, iport_pin;
	int blk_pin_count, num_port_pins, num_ports;

	/* Allocate and initialize the values to OPEN (-1). */
	temp_blk_pin_from_port_pin = (int ***) my_malloc(num_types * sizeof(int**));
	for (itype = 1; itype < num_types; itype++) {
		num_ports = type_descriptors[itype].pb_type->num_ports;
		temp_blk_pin_from_port_pin[itype] = (int **) my_malloc(num_ports * sizeof(int*));
		for (iport = 0; iport < num_ports; iport++) {
			num_port_pins = type_descriptors[itype].pb_type->ports[iport].num_pins;
			temp_blk_pin_from_port_pin[itype][iport] = (int *) my_malloc(num_port_pins * sizeof(int));
			
			for(iport_pin = 0; iport_pin < num_port_pins; iport_pin ++) {
				temp_blk_pin_from_port_pin[itype][iport][iport_pin] = OPEN;
			}
		}
	}
	
	/* Load the values */
	/* itype starts from 1 since type_descriptors[0] is the EMPTY_TYPE. */
	for (itype = 1; itype < num_types; itype++) {
		blk_pin_count = 0;
		num_ports = type_descriptors[itype].pb_type->num_ports;
		for (iport = 0; iport < num_ports; iport++) {
			num_port_pins = type_descriptors[itype].pb_type->ports[iport].num_pins;
			for (iport_pin = 0; iport_pin < num_port_pins; iport_pin++) {
				temp_blk_pin_from_port_pin[itype][iport][iport_pin] = blk_pin_count;
				blk_pin_count++;
			}
		}
	}
	
	/* Sets the file_scope variables to point at the arrays. */
	f_blk_pin_from_port_pin = temp_blk_pin_from_port_pin;
}


/***************************************************************************************
  Y.G.THIEN
  30 AUG 2012

 * The following functions parses the direct connections' information obtained from    *
 * the arch file. Then, the functions map the block pins indices for all block types   *
 * to the corresponding idirect (the index of the direct connection as specified in    *
 * the arch file) and direct type (whether this pin is a SOURCE or a SINK for the      *
 * direct connection). If a pin is not part of any direct connections, the value       *
 * OPEN (-1) is stored in both entries.                                                *
 *                                                                                     *
 * The mapping arrays are freed by the caller. Currently, this mapping is only used to *
 * load placement macros in place_macro.c                                              *
 *                                                                                     *
 ***************************************************************************************/

void parse_direct_pin_name(char * src_string, int line, int * start_pin_index, 
		int * end_pin_index, char * pb_type_name, char * port_name){

	/* Parses out the pb_type_name and port_name from the direct passed in.   *
	 * If the start_pin_index and end_pin_index is specified, parse them too. *
	 * Return the values parsed by reference.                                 */

	char source_string[MAX_STRING_LEN+1];
	char * find_format = NULL;
	int ichar, match_count;

	// parse out the pb_type and port name, possibly pin_indices
	find_format = strstr(src_string,"[");
	if (find_format == NULL) {
		/* Format "pb_type_name.port_name" */
		*start_pin_index = *end_pin_index = -1;
			
		strcpy (source_string, src_string);
		for (ichar = 0; ichar < (int)(strlen(source_string)); ichar++) {
			if (source_string[ichar] == '.')
				source_string[ichar] = ' ';
		}

		match_count = sscanf(source_string, "%s %s", pb_type_name, port_name);
		if (match_count != 2){
			vpr_printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, name should be in the format "
					"\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
					"The end_pin_index and start_pin_index can be the same.\n", 
					line, src_string);
			exit(1);
		}
	} else {
		/* Format "pb_type_name.port_name[end_pin_index:start_pin_index]" */
		strcpy (source_string, src_string);
		for (ichar = 0; ichar < (int)(strlen(source_string)); ichar++) {
            //Need white space between the components when using %s with
            //sscanf
			if (source_string[ichar] == '.')
				source_string[ichar] = ' ';
			if (source_string[ichar] == '[') 
				source_string[ichar] = ' ';
		}

		match_count = sscanf(source_string, "%s %s %d:%d]", 
								pb_type_name, port_name, 
								end_pin_index, start_pin_index);
		if (match_count != 4){
			vpr_printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, name should be in the format "
					"\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
					"The end_pin_index and start_pin_index can be the same.\n", 
					line, src_string);
			exit(1);
		}
		if (*end_pin_index < 0 || *start_pin_index < 0) {
			vpr_printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, the pin_index in "
					"[end_pin_index:start_pin_index] should not be a negative value.\n", 
					line, src_string);
			exit(1);
		}
		if ( *end_pin_index < *start_pin_index) {
			vpr_printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid from_pin - %s, the end_pin_index in "
					"[end_pin_index:start_pin_index] should not be less than start_pin_index.\n", 
					line, src_string);
			exit(1);
		}
	}
}

static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype, 
		int iport, int ** idirect_from_blk_pin, int idirect, 
		int ** direct_type_from_blk_pin, int direct_type, int line, char * src_string) {

	/* Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in    *
	 * direct_type_from_blk_pin with direct_type from start_pin_index to               *
	 * end_pin_index.                                                                  */

	int iport_pin, iblk_pin;

	// Mark pins with indices from start_pin_index to end_pin_index, inclusive
	for (iport_pin = start_pin_index; iport_pin <= end_pin_index; iport_pin++) {
		get_blk_pin_from_port_pin(itype, iport, iport_pin, &iblk_pin);
								
		// Check the fc for the pin, direct chain link only if fc == 0
		if (type_descriptors[itype].Fc[iblk_pin] == 0) {
			idirect_from_blk_pin[itype][iblk_pin] = idirect;
							
			// Check whether the pins are marked, errors out if so
			if (direct_type_from_blk_pin[itype][iblk_pin] != OPEN) {
				vpr_printf_error(__FILE__, __LINE__,
						"[LINE %d] Invalid pin - %s, this pin is in more than one direct connection.\n", 
						line, src_string);
				exit(1);
			} else {
				direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
			}
		}
	} // Finish marking all the pins

}

static void mark_direct_of_ports (int idirect, int direct_type, char * pb_type_name, 
		char * port_name, int end_pin_index, int start_pin_index, char * src_string, 
		int line, int ** idirect_from_blk_pin, int ** direct_type_from_blk_pin) {

	/* Go through all the ports in all the blocks to find the port that has the same   *
	 * name as port_name and belongs to the block type that has the name pb_type_name. *
	 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
	 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
	 * Otherwise, mark down all the pins in that port.                                 */

	int num_ports, num_port_pins;
	int itype, iport;

	// Go through all the block types
	for (itype = 1; itype < num_types; itype++) {

		// Find blocks with the same pb_type_name
		if (strcmp(type_descriptors[itype].pb_type->name, pb_type_name) == 0) {
			num_ports = type_descriptors[itype].pb_type->num_ports;
			for (iport = 0; iport < num_ports; iport++) {
				// Find ports with the same port_name
				if (strcmp(type_descriptors[itype].pb_type->ports[iport].name, port_name) == 0) {
					num_port_pins = type_descriptors[itype].pb_type->ports[iport].num_pins;

					// Check whether the end_pin_index is valid
					if (end_pin_index > num_port_pins) {
						vpr_printf_error(__FILE__, __LINE__,
								"[LINE %d] Invalid pin - %s, the end_pin_index in "
								"[end_pin_index:start_pin_index] should "
								"be less than the num_port_pins %d.\n", 
								line, src_string, num_port_pins);
						exit(1);
					}

					// Check whether the pin indices are specified
					if (start_pin_index >= 0 || end_pin_index >= 0) {
						mark_direct_of_pins(start_pin_index, end_pin_index, itype, 
								iport, idirect_from_blk_pin, idirect, 
								direct_type_from_blk_pin, direct_type, line, src_string);
					} else {
						mark_direct_of_pins(0, num_port_pins-1, itype, 
								iport, idirect_from_blk_pin, idirect, 
								direct_type_from_blk_pin, direct_type, line, src_string);
					}
				} // Do nothing if port_name does not match
			} // Finish going through all the ports
		} // Do nothing if pb_type_name does not match
	} // Finish going through all the blocks

}

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs, 
		int *** idirect_from_blk_pin, int *** direct_type_from_blk_pin) {

	/* Allocates and loads idirect_from_blk_pin and direct_type_from_blk_pin arrays.    *
	 *                                                                                  *
	 * For a bus (multiple bits) direct connection, all the pins in the bus are marked. *
	 *                                                                                  *
	 * idirect_from_blk_pin array allow us to quickly find pins that could be in a      *
	 * direct connection. Values stored is the index of the possible direct connection  *
	 * as specified in the arch file, OPEN (-1) is stored for pins that could not be    *
	 * part of a direct chain conneciton.                                               *
	 *                                                                                  *
	 * direct_type_from_blk_pin array stores the value SOURCE if the pin is the         *
	 * from_pin, SINK if the pin is the to_pin in the direct connection as specified in *
	 * the arch file, OPEN (-1) is stored for pins that could not be part of a direct   *
	 * chain conneciton.                                                                *
	 *                                                                                  *
	 * Stores the pointers to the two 2D arrays in the addresses passed in.             *
	 *                                                                                  *
	 * The two arrays are freed by the caller(s).                                       */

	int itype, iblk_pin, idirect, num_type_pins;
	int ** temp_idirect_from_blk_pin, ** temp_direct_type_from_blk_pin;

	char to_pb_type_name[MAX_STRING_LEN+1], to_port_name[MAX_STRING_LEN+1], 
			from_pb_type_name[MAX_STRING_LEN+1], from_port_name[MAX_STRING_LEN+1];
	int to_start_pin_index = -1, to_end_pin_index = -1;
	int from_start_pin_index = -1, from_end_pin_index = -1;
		
	/* Allocate and initialize the values to OPEN (-1). */
	temp_idirect_from_blk_pin = (int **) my_malloc(num_types * sizeof(int *));
	temp_direct_type_from_blk_pin = (int **) my_malloc(num_types * sizeof(int *));
	for (itype = 1; itype < num_types; itype++) {
		
		num_type_pins = type_descriptors[itype].num_pins;

		temp_idirect_from_blk_pin[itype] = (int *) my_malloc(num_type_pins * sizeof(int));
		temp_direct_type_from_blk_pin[itype] = (int *) my_malloc(num_type_pins * sizeof(int));
	
		/* Initialize values to OPEN */
		for (iblk_pin = 0; iblk_pin < num_type_pins; iblk_pin++) {
			temp_idirect_from_blk_pin[itype][iblk_pin] = OPEN;
			temp_direct_type_from_blk_pin[itype][iblk_pin] = OPEN;
		}
	}

	/* Load the values */
	// Go through directs and find pins with possible direct connections
	for (idirect = 0; idirect < num_directs; idirect++) {
		
		// Parse out the pb_type and port name, possibly pin_indices from from_pin
		parse_direct_pin_name(directs[idirect].from_pin, directs[idirect].line, 
				&from_end_pin_index, &from_start_pin_index, from_pb_type_name, from_port_name);
			
		// Parse out the pb_type and port name, possibly pin_indices from to_pin
		parse_direct_pin_name(directs[idirect].to_pin, directs[idirect].line,
				&to_end_pin_index, &to_start_pin_index, to_pb_type_name, to_port_name);
		
		
		/* Now I have all the data that I need, I could go through all the block pins   *
		 * in all the blocks to find all the pins that could have possible direct       *
		 * connections. Mark all down all those pins with the idirect the pins belong   *
		 * to and whether it is a source or a sink of the direct connection.            */
		
		// Find blocks with the same name as from_pb_type_name and from_port_name
		mark_direct_of_ports (idirect, SOURCE, from_pb_type_name, from_port_name, 
				from_end_pin_index, from_start_pin_index, directs[idirect].from_pin, 
				directs[idirect].line,
				temp_idirect_from_blk_pin, temp_direct_type_from_blk_pin);

		// Then, find blocks with the same name as to_pb_type_name and from_port_name
		mark_direct_of_ports (idirect, SINK, to_pb_type_name, to_port_name, 
				to_end_pin_index, to_start_pin_index, directs[idirect].to_pin, 
				directs[idirect].line,
				temp_idirect_from_blk_pin, temp_direct_type_from_blk_pin);

	} // Finish going through all the directs

	/* Returns the pointer to the 2D arrays by reference. */
	*idirect_from_blk_pin = temp_idirect_from_blk_pin;
	*direct_type_from_blk_pin = temp_direct_type_from_blk_pin;
}
