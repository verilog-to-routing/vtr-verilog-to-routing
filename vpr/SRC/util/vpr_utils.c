#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "cluster_placement.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */

/******************** Subroutine definitions ********************************/

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
						L_grid[i][j].blocks[k] = OPEN;
					}
				}
			}
		}
	}

	/* Go through each block */
	for (i = 0; i < L_num_blocks; ++i) {
		/* Check range of block coords */
		if (block[i].x < 0 || block[i].x > (L_nx + 1) || block[i].y < 0
				|| (block[i].y + block[i].type->height - 1) > (L_ny + 1)
				|| block[i].z < 0 || block[i].z > (block[i].type->capacity)) {
			printf(ERRTAG
			"Block %d is at invalid location (%d, %d, %d)\n", i, block[i].x,
					block[i].y, block[i].z);
			exit(1);
		}

		/* Check types match */
		if (block[i].type != L_grid[block[i].x][block[i].y].type) {
			printf(ERRTAG "A block is in a grid location "
			"(%d x %d) with a conflicting type.\n", block[i].x, block[i].y);
			exit(1);
		}

		/* Check already in use */
		if (OPEN != L_grid[block[i].x][block[i].y].blocks[block[i].z]) {
			printf(ERRTAG
			"Location (%d, %d, %d) is used more than once\n", block[i].x,
					block[i].y, block[i].z);
			exit(1);
		}

		if (L_grid[block[i].x][block[i].y].offset != 0) {
			printf(ERRTAG
			"Large block not aligned in placment for block %d at (%d, %d, %d)",
					i, block[i].x, block[i].y, block[i].z);
			exit(1);
		}

		/* Set the block */
		for (j = 0; j < block[i].type->height; j++) {
			L_grid[block[i].x][block[i].y + j].blocks[block[i].z] = i;
			L_grid[block[i].x][block[i].y + j].usage++;
			assert(L_grid[block[i].x][block[i].y + j].offset == j);
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

void get_class_range_for_block(INP int iblk, OUTP int *class_low,
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
						if (logical_block[iblk].input_nets[port->index][j]
								!= OPEN) {
							return FALSE;
						}
					} else if (port->dir == OUT_PORT) {
						if (logical_block[iblk].output_nets[port->index][j]
								!= OPEN) {
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

	pb_type = pb->pb_graph_node->pb_type;

	total_nodes = pb->pb_graph_node->total_pb_pins + pb_type->num_input_pins
			+ pb_type->num_output_pins + pb_type->num_clock_pins;

	for (i = 0; i < total_nodes; i++) {
		if (pb->rr_graph[i].edges != NULL) {
			free(pb->rr_graph[i].edges);
		}
		if (pb->rr_graph[i].switches != NULL) {
			free(pb->rr_graph[i].switches);
		}
	}
	free(pb->rr_graph);
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
		for (i = 0;
				i < pb_type->modes[mode].num_pb_type_children
						&& pb->child_pbs != NULL; i++) {
			for (j = 0;
					j < pb_type->modes[mode].pb_type_children[i].num_pb
							&& pb->child_pbs[i] != NULL; j++) {
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

		if(pb->local_nets != NULL) {
			for(i = 0; i < pb->num_local_nets; i++) {
				free(pb->local_nets[i].node_block);
				free(pb->local_nets[i].node_block_port);
				free(pb->local_nets[i].node_block_pin);
				if(pb->local_nets[i].name != NULL) {
					free(pb->local_nets[i].name);
				}
			}
			free(pb->local_nets);
			pb->local_nets = NULL;
		}

		if(pb->rr_node_to_pb_mapping != NULL) {
			free(pb->rr_node_to_pb_mapping);
			pb->rr_node_to_pb_mapping = NULL;
		}
		
		if(pb->name)
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
		if (pb->logical_block != OPEN && logical_block != NULL) {
			logical_block[pb->logical_block].clb_index = NO_CLUSTER;
			logical_block[pb->logical_block].pb = NULL;
			/* If any molecules were marked invalid because of this logic block getting packed, mark them valid */
			revalid_molecule = logical_block[pb->logical_block].packed_molecules;
			while(revalid_molecule != NULL) {
				cur_molecule = (t_pack_molecule*)revalid_molecule->data_vptr;
				if(cur_molecule->valid == FALSE) {
					for (i = 0; i < get_array_size_of_molecule(cur_molecule); i++) {
						if (cur_molecule->logical_block_ptrs[i] != NULL) {
							if(cur_molecule->logical_block_ptrs[i]->clb_index != OPEN) {
								break;
							}
						}
					}
					/* All logical blocks are open for this molecule, place back in queue */
					if(i == get_array_size_of_molecule(cur_molecule)) {
						cur_molecule->valid = TRUE;	
					}
				}
				revalid_molecule = revalid_molecule->next;
			}
		}
		pb->logical_block = OPEN;
	}
	free_pb_stats(pb);
	pb->pb_stats.gain = NULL;
}

void free_pb_stats(t_pb *pb) {
	int i;
	t_pb_graph_node *pb_graph_node = pb->pb_graph_node;

	if (pb->pb_stats.gain != NULL) {
		free(pb->pb_stats.gain);
		free(pb->pb_stats.lengthgain);
		free(pb->pb_stats.sharinggain);
		free(pb->pb_stats.hillgain);
		free(pb->pb_stats.connectiongain);
		for (i = 0; i < pb_graph_node->num_input_pin_class; i++) {
			free(pb->pb_stats.input_pins_used[i]);
			free(pb->pb_stats.lookahead_input_pins_used[i]);
		}
		free(pb->pb_stats.input_pins_used);
		free(pb->pb_stats.lookahead_input_pins_used);
		for (i = 0; i < pb_graph_node->num_output_pin_class; i++) {
			free(pb->pb_stats.output_pins_used[i]);
			free(pb->pb_stats.lookahead_output_pins_used[i]);
		}
		free(pb->pb_stats.output_pins_used);
		free(pb->pb_stats.lookahead_output_pins_used);
		free(pb->pb_stats.feasible_blocks);
		free(pb->pb_stats.num_pins_of_net_in_pb);
		free(pb->pb_stats.marked_nets);
		free(pb->pb_stats.marked_blocks);
		pb->pb_stats.gain = NULL;
	}
}



