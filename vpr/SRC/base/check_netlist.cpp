/**
* Simple checks to make sure netlist data structures are consistent.  These include checking for duplicated names, dangling links, etc.
*/

#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "check_netlist.h"
#include "read_xml_arch_file.h"

#define ERROR_THRESHOLD 100

/**************** Subroutines local to this module **************************/

static int check_connections_to_global_clb_pins(unsigned int inet);

static int check_for_duplicated_names(void);

static int check_clb_conn(int iblk, int num_conn);

static int check_clb_internal_nets(unsigned int iblk);

static int get_num_conn(int bnum);

/*********************** Subroutine definitions *****************************/

void check_netlist() {
	unsigned int i;
	int error, num_conn;
	t_hash **net_hash_table, *h_net_ptr;

	
	/* This routine checks that the netlist makes sense         */

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

	net_hash_table = alloc_hash_table();

	error = 0;

	/* Check that nets fanout and have a driver. */
	for (i = 0; i < cluster_ctx.clbs_nlist.net.size(); i++) {
		h_net_ptr = insert_in_hash_table(net_hash_table, cluster_ctx.clbs_nlist.net[i].name, i);
		if (h_net_ptr->count != 1) {
			vtr::printf_error(__FILE__, __LINE__, 
					"Net %s has multiple drivers.\n", cluster_ctx.clbs_nlist.net[i].name);
			error++;
		}
		error += check_connections_to_global_clb_pins(i);
		if (error >= ERROR_THRESHOLD) {
			vtr::printf_error(__FILE__, __LINE__, 
					"Too many errors in netlist, exiting.\n");
		}
	}
	free_hash_table(net_hash_table);

	/* Check that each block makes sense. */
	for (i = 0; i < (unsigned int)cluster_ctx.num_blocks; i++) {
		num_conn = get_num_conn(i);
		error += check_clb_conn(i, num_conn);
		error += check_clb_internal_nets(i);
		if (error >= ERROR_THRESHOLD) {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
					"Too many errors in netlist, exiting.\n");
		}
	}

	error += check_for_duplicated_names();

	if (error != 0) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"Found %d fatal Errors in the input netlist.\n", error);
	}

	/* 
     * Enhanced HACK: July 2017 
     *     Do not route constant nets (e.g. gnd/vcc). Identifying these nets as constants 
     *     is more robust than the previous approach (exact name match to gnd/vcc).
     *     Note that by not routing constant nets we are implicitly assuming that all pins 
     *     in the FPGA can be tied to gnd/vcc, and hence we do not need to route them.
	 * 
     * TODO: We should ultimately make this architecture driven (e.g. specify which
     *       pins which can be tied to gnd/vcc), and then route from those pins to
     *       deliver any constants to those primitive input pins which can not be directly 
     *       tied directly to gnd/vcc.
	 */
    auto& atom_ctx = g_vpr_ctx.atom();
	for (i = 0; i < cluster_ctx.clbs_nlist.net.size(); i++) {
        AtomNetId atom_net = atom_ctx.lookup.atom_net(i);
        VTR_ASSERT(atom_net);

        if (atom_ctx.nlist.net_is_constant(atom_net)) {
            //Mark net as global, so that it is not routed
            vtr::printf_warning(__FILE__, __LINE__, "Treating constant net '%s' as global, so it will not be routed\n", atom_ctx.nlist.net_name(atom_net).c_str());
            cluster_ctx.clbs_nlist.net[i].is_global = true;
        }
	}
}

static int check_connections_to_global_clb_pins(unsigned int inet) {

	/* Checks that a global net (inet) connects only to global CLB input pins  *
	 * and that non-global nets never connects to a global CLB pin.  Either    *
	 * global or non-global nets are allowed to connect to pads.               */

	unsigned int ipin, num_pins, iblk, node_block_pin, error;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	num_pins = (cluster_ctx.clbs_nlist.net[inet].pins.size());
	error = 0;

	/* For now global signals can be driven by an I/O pad or any CLB output       *
	 * although a CLB output generates a warning.  I could make a global CLB      *
	 * output pin type to allow people to make architectures that didn't have     *
	 * this warning.                                                              */

	for (ipin = 0; ipin < num_pins; ipin++) {
		iblk = cluster_ctx.clbs_nlist.net[inet].pins[ipin].block;

		node_block_pin = cluster_ctx.clbs_nlist.net[inet].pins[ipin].block_pin;

		bool is_global_net = static_cast<bool>(cluster_ctx.clbs_nlist.net[inet].is_global);
		if (cluster_ctx.blocks[iblk].type->is_global_pin[node_block_pin] != is_global_net 
            && cluster_ctx.blocks[iblk].type != device_ctx.IO_TYPE) {

			/* Allow a CLB output pin to drive a global net (warning only). */

			if (ipin == 0 && cluster_ctx.clbs_nlist.net[inet].is_global) {
				vtr::printf_warning(__FILE__, __LINE__, 
						"in check_connections_to_global_clb_pins:\n");
				vtr::printf_warning(__FILE__, __LINE__, 
						"\tnet #%d (%s) is driven by CLB output pin (#%d) on block #%d (%s).\n", 
						inet, cluster_ctx.clbs_nlist.net[inet].name, node_block_pin, iblk, cluster_ctx.blocks[iblk].name);
			} else { /* Otherwise -> Error */
				vtr::printf_error(__FILE__, __LINE__, 
						"in check_connections_to_global_clb_pins:\n");
				vtr::printf_error(__FILE__, __LINE__, 
						"\tpin %d on net #%d (%s) connects to CLB input pin (#%d) on block #%d (%s).\n", 
						ipin, inet, cluster_ctx.clbs_nlist.net[inet].name, node_block_pin, iblk, cluster_ctx.blocks[iblk].name);
				error++;
			}

			if (cluster_ctx.clbs_nlist.net[inet].is_global)
				vtr::printf_info("Net is global, but CLB pin is not.\n");
			else
				vtr::printf_info("CLB pin is global, but net is not.\n");
			vtr::printf_info("\n");
		}
	} /* End for all pins */

	return (error);
}

static int check_clb_conn(int iblk, int num_conn) {

	/* Checks that the connections into and out of the clb make sense.  */

	int iclass, ipin, error;
	t_type_ptr type;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	error = 0;
	type = cluster_ctx.blocks[iblk].type;

	if (type == device_ctx.IO_TYPE) {
	    /*
		//This triggers incorrectly if other blocks (e.g. I/O buffers) are included in the iopads
		if (num_conn != 1) {
			vtr::printf_error(__FILE__, __LINE__, 
					"IO blk #%d (%s) has %d pins.\n", iblk, cluster_ctx.blocks[iblk].name, num_conn);
			error++;
		}
             */
	} else if (num_conn < 2) {
		vtr::printf_warning(__FILE__, __LINE__, 
				"Logic block #%d (%s) has only %d pin.\n", iblk, cluster_ctx.blocks[iblk].name, num_conn);

		/* Allow the case where we have only one OUTPUT pin connected to continue. *
		 * This is used sometimes as a constant generator for a primary output,    *
		 * but I will still warn the user.  If the only pin connected is an input, *
		 * abort.                                                                  */

		if (num_conn == 1) {
			for (ipin = 0; ipin < type->num_pins; ipin++) {
				if (cluster_ctx.blocks[iblk].nets[ipin] != OPEN) {
					iclass = type->pin_class[ipin];

					if (type->class_inf[iclass].type != DRIVER) {
						vtr::printf_info("Pin is an input -- this whole block is hanging logic that should be swept in logic synthesis.\n");
						vtr::printf_info("\tNon-fatal, but check this.\n");
					} else {
						vtr::printf_info("Pin is an output -- may be a constant generator.\n");
						vtr::printf_info("\tNon-fatal, but check this.\n");
					}

					break;
				}
			}
		}
	}

	/* This case should already have been flagged as an error -- this is *
	 * just a redundant double check.                                    */

	if (num_conn > type->num_pins) {
		vtr::printf_error(__FILE__, __LINE__, 
				"logic block #%d with output %s has %d pins.\n", iblk, cluster_ctx.blocks[iblk].name, num_conn);
		error++;
	}

	return (error);
}


/* Check that internal-to-logic-block connectivity is continuous and logically consistent
*/
static int check_clb_internal_nets(unsigned int iblk) {
	
    auto& cluster_ctx = g_vpr_ctx.clustering();
	
	int error = 0;
	t_pb_route * pb_route = cluster_ctx.blocks[iblk].pb_route;
	int num_pins_in_block = cluster_ctx.blocks[iblk].pb->pb_graph_node->total_pb_pins;

	t_pb_graph_pin** pb_graph_pin_lookup = alloc_and_load_pb_graph_pin_lookup_from_index(cluster_ctx.blocks[iblk].type);

	for (int i = 0; i < num_pins_in_block; i++) {
		if (pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id != OPEN) {
			if ((pb_graph_pin_lookup[i]->port->type == IN_PORT && pb_graph_pin_lookup[i]->parent_node->parent_pb_graph_node == NULL) ||
				(pb_graph_pin_lookup[i]->port->type == OUT_PORT && pb_graph_pin_lookup[i]->parent_node->pb_type->num_modes == 0)
				) {
				if (pb_route[i].driver_pb_pin_id != OPEN) {
					vtr::printf_error(__FILE__, __LINE__,
						"Internal connectivity error in logic block #%d with output %s.  Internal node %d driven when it shouldn't be driven \n", iblk, cluster_ctx.blocks[iblk].name, i);
					error++;
				}
			} else {
				if (!pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id == OPEN) {
					vtr::printf_error(__FILE__, __LINE__,
						"Internal connectivity error in logic block #%d with output %s.  Internal node %d dangling\n", iblk, cluster_ctx.blocks[iblk].name, i);
					error++;
				} else {
					int prev_pin = pb_route[i].driver_pb_pin_id;
					if (pb_route[prev_pin].atom_net_id != pb_route[i].atom_net_id) {
						vtr::printf_error(__FILE__, __LINE__,
							"Internal connectivity error in logic block #%d with output %s.  Internal node %d driven by different net than internal node %d\n", iblk, cluster_ctx.blocks[iblk].name, i, prev_pin);
						error++;
					}
				}
			}
		}
	}

	free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
	return error;
}

static int check_for_duplicated_names(void) {
	int iblk, error;
	int clb_count;
	t_hash **clb_hash_table, *clb_h_ptr;
	
    auto& cluster_ctx = g_vpr_ctx.clustering();

	clb_hash_table = alloc_hash_table();
	
	error = clb_count = 0;

	for (iblk = 0; iblk < cluster_ctx.num_blocks; iblk++)
	{
		clb_h_ptr = insert_in_hash_table(clb_hash_table, cluster_ctx.blocks[iblk].name, clb_count);
		if (clb_h_ptr->count > 1) {
			vtr::printf_error(__FILE__, __LINE__, 
					"Block %s has duplicated name.\n", cluster_ctx.blocks[iblk].name);
			error++;
		} else {
			clb_count++;
		}
	}

	free_hash_table(clb_hash_table);

	return error;
}

static int get_num_conn(int bnum) {

	/* This routine returns the number of connections to a block. */

	int i, num_conn;
	t_type_ptr type;

    auto& cluster_ctx = g_vpr_ctx.clustering();

	type = cluster_ctx.blocks[bnum].type;

	num_conn = 0;

	for (i = 0; i < type->num_pins; i++) {
		if (cluster_ctx.blocks[bnum].nets[i] != OPEN)
			num_conn++;
	}

	return (num_conn);
}

