/* TODO: Consider deleting this file altogether.  Many netlist checks now done during parsing.  Also, the checks here are too strict.  For example, we may actually want to allow mixing of local/global signals */

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "check_netlist.h"
#include "assert.h"
#include "read_xml_arch_file.h"

#define ERROR_THRESHOLD 100

/**************** Subroutines local to this module **************************/

static int check_connections_to_global_clb_pins(int inet);

static int check_for_duplicated_names(void);

static int check_clb_conn(int iblk, int num_conn);

static int check_clb_internal_nets(int iblk);

static int check_subblock_internal_nets(int iblk, int isub);

static int get_num_conn(int bnum);

static int check_subblocks(int iblk);

static int check_primitives(int iblk, int isub);

/*********************** Subroutine definitions *****************************/

void check_netlist() {
	int i, error, num_conn;
	struct s_hash **net_hash_table, *h_net_ptr;

	/* TODO: Remove the following the function calls after these functions have 
	 been fleshed and are legitimately used in the code!!! They are called here so that 
	 the compiler will not throw an error for an unused function				*/

	int unused_var;
	unused_var = check_subblock_internal_nets(0, 0);
	unused_var = check_primitives(0, 0);
	if (unused_var)
		vpr_printf(TIO_MESSAGE_INFO, "Please go to the check_netlist() function in check_netlist.c and remove the first section as needed.");

	/* This routine checks that the netlist makes sense         */

	net_hash_table = alloc_hash_table();

	error = 0;

	/* Check that nets fanout and have a driver. */
	for (i = 0; i < num_nets; i++) {
		h_net_ptr = insert_in_hash_table(net_hash_table, clb_net[i].name, i);
		if (h_net_ptr->count != 1) {
			vpr_printf(TIO_MESSAGE_ERROR, "Net %s has multiple drivers.\n", clb_net[i].name);
			error++;
		}
		error += check_connections_to_global_clb_pins(i);
		if (error >= ERROR_THRESHOLD) {
			vpr_printf(TIO_MESSAGE_ERROR, "Too many errors in netlist, exiting.\n");
		}
	}
	free_hash_table(net_hash_table);

	/* Check that each block makes sense. */
	for (i = 0; i < num_blocks; i++) {
		num_conn = get_num_conn(i);
		error += check_clb_conn(i, num_conn);
		error += check_clb_internal_nets(i);
		error += check_subblocks(i);
		if (error >= ERROR_THRESHOLD) {
			vpr_printf(TIO_MESSAGE_ERROR, "Too many errors in netlist, exiting.\n");
			exit(1);
		}
	}

	error += check_for_duplicated_names();

	if (error != 0) {
		vpr_printf(TIO_MESSAGE_ERROR, "Found %d fatal Errors in the input netlist.\n", error);
		exit(1);
	}

	/* HACK: Jason Luu January 17, 2011 Do not route common constants gnd and vcc
	 Todo: Need to make architecture driven.
	 */
	for (i = 0; i < num_nets; i++) {
		if (strcmp(clb_net[i].name, "vcc") == 0) {
			clb_net[i].is_global = TRUE;
		} else if (strcmp(clb_net[i].name, "gnd") == 0) {
			clb_net[i].is_global = TRUE;
		}
	}
}

static int check_connections_to_global_clb_pins(int inet) {

	/* Checks that a global net (inet) connects only to global CLB input pins  *
	 * and that non-global nets never connects to a global CLB pin.  Either    *
	 * global or non-global nets are allowed to connect to pads.               */

	int ipin, num_pins, iblk, node_block_pin, error;

	num_pins = (clb_net[inet].num_sinks + 1);
	error = 0;

	/* For now global signals can be driven by an I/O pad or any CLB output       *
	 * although a CLB output generates a warning.  I could make a global CLB      *
	 * output pin type to allow people to make architectures that didn't have     *
	 * this warning.                                                              */

	for (ipin = 0; ipin < num_pins; ipin++) {
		iblk = clb_net[inet].node_block[ipin];

		node_block_pin = clb_net[inet].node_block_pin[ipin];

		if (block[iblk].type->is_global_pin[node_block_pin]
				!= clb_net[inet].is_global && block[iblk].type != IO_TYPE) {

			/* Allow a CLB output pin to drive a global net (warning only). */

			if (ipin == 0 && clb_net[inet].is_global) {
				vpr_printf(TIO_MESSAGE_WARNING, "in check_connections_to_global_clb_pins:\n");
				vpr_printf(TIO_MESSAGE_WARNING, "\tnet #%d (%s) is driven by CLB output pin (#%d) on block #%d (%s).\n", 
						inet, clb_net[inet].name, node_block_pin, iblk, block[iblk].name);
			} else { /* Otherwise -> Error */
				vpr_printf(TIO_MESSAGE_ERROR, "in check_connections_to_global_clb_pins:\n");
				vpr_printf(TIO_MESSAGE_ERROR, "\tpin %d on net #%d (%s) connects to CLB input pin (#%d) on block #%d (%s).\n", 
						ipin, inet, clb_net[inet].name, node_block_pin, iblk, block[iblk].name);
				error++;
			}

			if (clb_net[inet].is_global)
				vpr_printf(TIO_MESSAGE_INFO, "Net is global, but CLB pin is not.\n");
			else
				vpr_printf(TIO_MESSAGE_INFO, "CLB pin is global, but net is not.\n");
			vpr_printf(TIO_MESSAGE_INFO, "\n");
		}
	} /* End for all pins */

	return (error);
}

static int check_clb_conn(int iblk, int num_conn) {

	/* Checks that the connections into and out of the clb make sense.  */

	int iclass, ipin, error;
	t_type_ptr type;

	error = 0;
	type = block[iblk].type;

	if (type == IO_TYPE) {
	    /*
		//This triggers incorrectly if other blocks (e.g. I/O buffers) are included in the iopads
		if (num_conn != 1) {
			vpr_printf(TIO_MESSAGE_ERROR, "IO blk #%d (%s) has %d pins.\n", 
					iblk, block[iblk].name, num_conn);
			error++;
		}
             */
	} else if (num_conn < 2) {
		vpr_printf(TIO_MESSAGE_WARNING, "Logic block #%d (%s) has only %d pin.\n", 
				iblk, block[iblk].name, num_conn);

		/* Allow the case where we have only one OUTPUT pin connected to continue. *
		 * This is used sometimes as a constant generator for a primary output,    *
		 * but I will still warn the user.  If the only pin connected is an input, *
		 * abort.                                                                  */

		if (num_conn == 1) {
			for (ipin = 0; ipin < type->num_pins; ipin++) {
				if (block[iblk].nets[ipin] != OPEN) {
					iclass = type->pin_class[ipin];

					if (type->class_inf[iclass].type != DRIVER) {
						vpr_printf(TIO_MESSAGE_INFO, "Pin is an input -- this whole block is hanging logic that should be swept in logic synthesis.\n");
						vpr_printf(TIO_MESSAGE_INFO, "\tNon-fatal, but check this.\n");
					} else {
						vpr_printf(TIO_MESSAGE_INFO, "Pin is an output -- may be a constant generator.\n");
						vpr_printf(TIO_MESSAGE_INFO, "\tNon-fatal, but check this.\n");
					}

					break;
				}
			}
		}
	}

	/* This case should already have been flagged as an error -- this is *
	 * just a redundant double check.                                    */

	if (num_conn > type->num_pins) {
		vpr_printf(TIO_MESSAGE_ERROR, "logic block #%d with output %s has %d pins.\n", 
				iblk, block[iblk].name, num_conn);
		error++;
	}

	return (error);
}

static int check_clb_internal_nets(int iblk) {
	/** TODO:
	 * Check if the internal CLB nets makes sense and are connected properly 
	 *  Consists of 3 main loops
	 * 1. a) Check name uniqueness
	 b) Check all net connections are to CLB pins or subblock pins and that they match the net examined
	 * 2. Check all connected CLB pins are connected to valid internal nets
	 * 3. Check all connected subblock pins are connected to valid internal nets and that these match the net indexes
	 */
	return 0;
}

static int check_subblock_internal_nets(int iblk, int isub) {
	/**
	 * TODO
	 * Check if the internal CLB nets makes sense and are connected properly 
	 *  Consists of 3 main checks
	 * 1. a) Check name uniqueness
	 b) Check all net connections are to CLB pins or subblock pins and that they match the net examined
	 * 2. Check all connected CLB pins are connected to valid internal nets
	 * 3. Check all connected subblock pins are connected to valid internal nets and that these match the net indexes
	 */
	return 0;
}

static int check_subblocks(int iblk) {
	/* TODO */
	/* This routine checks the subblocks of iblk (which must be a CLB).  It    *
	 * returns the number of errors found.                                     */
	return 0;
}

static int check_primitives(int iblk, int isub) {

	/* TODO:
	 This routine checks the subblocks of iblk (which must be a CLB).  It    *
	 * returns the number of errors found.                                     */
	return 0;

}

static int check_for_duplicated_names(void) {
#if 0
	int iblk, isub, iprim, error;
	int clb_count, sub_count, prim_count;
	struct s_hash **clb_hash_table, *clb_h_ptr;
	struct s_hash **sub_hash_table, *sub_h_ptr;
	struct s_hash **prim_hash_table, *prim_h_ptr;

	clb_hash_table = alloc_hash_table();
	sub_hash_table = alloc_hash_table();
	prim_hash_table = alloc_hash_table();

	error = clb_count = sub_count = prim_count = 0;

	for (iblk = 0; iblk < num_blocks; iblk++)
	{
		clb_h_ptr = insert_in_hash_table(clb_hash_table, block[iblk].name, clb_count);
		if (clb_h_ptr->count > 1) {
			vpr_printf(TIO_MESSAGE_ERROR, "Block %s has duplicated name.\n", 
					block[iblk].name);
			error++;
		} else {
			clb_count++;
		}
		for (isub = 0; isub < block[iblk].num_subblocks; isub++)
		{
			sub_h_ptr = insert_in_hash_table(sub_hash_table, block[iblk].subblocks[isub].name, sub_count);
			if (sub_h_ptr->count > 1) {
				vpr_printf(TIO_MESSAGE_ERROR, "Subblock %s has duplicated name.\n", 
						block[iblk].subblocks[isub].name);
				error++;
			} else {
				sub_count++;
			}
			for (iprim = 0; iprim < block[iblk].subblocks[isub].num_primitives; iprim++)
			{
				prim_h_ptr = insert_in_hash_table(prim_hash_table, block[iblk].subblocks[isub].primitives[iprim].name, prim_count);
				if (prim_h_ptr->count > 1) {
					vpr_printf(TIO_MESSAGE_ERROR, "Primitive %s has duplicated name.\n", 
							block[iblk].subblocks[isub].primitives[iprim].name);
					error++;
				} else {
					prim_count++;
				}
			}
		}
	}
	return error;
#endif
	return 0;
}

static int get_num_conn(int bnum) {

	/* This routine returns the number of connections to a block. */

	int i, num_conn;
	t_type_ptr type;

	type = block[bnum].type;

	num_conn = 0;

	for (i = 0; i < type->num_pins; i++) {
		if (block[bnum].nets[i] != OPEN)
			num_conn++;
	}

	return (num_conn);
}

