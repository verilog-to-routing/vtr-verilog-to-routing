#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "read_xml_arch_file.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "prepack.h"
#include "pack_types.h"
#include "pack.h"
#include "read_blif.h"
#include "cluster.h"
#include "output_clustering.h"
#include "ReadOptions.h"

/* #define DUMP_PB_GRAPH 1 */
/* #define DUMP_BLIF_INPUT 1 */

static boolean *alloc_and_load_is_clock(boolean global_clocks);

void try_pack(INP struct s_packer_opts *packer_opts, INP const t_arch * arch,
		INP t_model *user_models, INP t_model *library_models, t_timing_inf timing_inf, float interc_delay, vector<t_lb_type_rr_node> *lb_type_rr_graphs) {
	boolean *is_clock;
	int num_models;
	t_model *cur_model;
	t_pack_patterns *list_of_packing_patterns;
	int num_packing_patterns;
	t_pack_molecule *list_of_pack_molecules, * cur_pack_molecule;
	int num_pack_molecules;

	vpr_printf_info("Begin packing '%s'.\n", packer_opts->blif_file_name);

	/* determine number of models in the architecture */
	num_models = 0;
	cur_model = user_models;
	while (cur_model) {
		num_models++;
		cur_model = cur_model->next;
	}
	cur_model = library_models;
	while (cur_model) {
		num_models++;
		cur_model = cur_model->next;
	}


	is_clock = alloc_and_load_is_clock(packer_opts->global_clocks);
	
	vpr_printf_info("\n");
	vpr_printf_info("After removing unused inputs...\n");
	vpr_printf_info("\ttotal blocks: %d, total nets: %d, total inputs: %d, total outputs: %d\n",
		num_logical_blocks, (int) g_atoms_nlist.net.size(), num_p_inputs, num_p_outputs);

	vpr_printf_info("Begin prepacking.\n");
	list_of_packing_patterns = alloc_and_load_pack_patterns(
			&num_packing_patterns);
	list_of_pack_molecules = alloc_and_load_pack_molecules(
			list_of_packing_patterns, num_packing_patterns,
			&num_pack_molecules);
	vpr_printf_info("Finish prepacking.\n");

	if(packer_opts->auto_compute_inter_cluster_net_delay) {
		packer_opts->inter_cluster_net_delay = interc_delay;
		vpr_printf_info("Using inter-cluster delay: %g\n", packer_opts->inter_cluster_net_delay);
	}

	/* Uncomment line below if you want a dump of compressed netlist. */
	/* if (getEchoEnabled()){
	 echo_input (packer_opts->blif_file_name, packer_opts->lut_size, "packed.echo"); 
	 }else; */

	if (packer_opts->skip_clustering == FALSE) {
		do_clustering(arch, list_of_pack_molecules, num_models,
				packer_opts->global_clocks, is_clock,
				packer_opts->hill_climbing_flag, packer_opts->output_file,
				packer_opts->timing_driven, packer_opts->cluster_seed_type,
				packer_opts->alpha, packer_opts->beta,
				packer_opts->recompute_timing_after, packer_opts->block_delay,
				packer_opts->intra_cluster_net_delay,
				packer_opts->inter_cluster_net_delay, packer_opts->aspect,
				packer_opts->allow_unrelated_clustering,
				packer_opts->allow_early_exit, packer_opts->connection_driven,
				packer_opts->packer_algorithm, timing_inf,
				lb_type_rr_graphs);
	} else {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, 
				"Skip clustering no longer supported.\n");
	}

	free(is_clock);
	
	/*free list_of_pack_molecules*/
	free_list_of_pack_patterns(list_of_packing_patterns, num_packing_patterns);

	cur_pack_molecule = list_of_pack_molecules;
	while (cur_pack_molecule != NULL){
		if (cur_pack_molecule->logical_block_ptrs != NULL)
			free(cur_pack_molecule->logical_block_ptrs);
		cur_pack_molecule = list_of_pack_molecules->next;
		free(list_of_pack_molecules);
		list_of_pack_molecules = cur_pack_molecule;
	}

	vpr_printf_info("\n");
	vpr_printf_info("Netlist conversion complete.\n");
	vpr_printf_info("\n");
}

float get_switch_info(short switch_index, float &Tdel_switch, float &R_switch, float &Cout_switch) {
	/* Fetches delay, resistance and output capacitance of the switch at switch_index. 
	Returns the total delay through the switch. Used to calculate inter-cluster net delay. */

	Tdel_switch = switch_inf[switch_index].Tdel; /* Delay when unloaded */
	R_switch = switch_inf[switch_index].R;
	Cout_switch = switch_inf[switch_index].Cout;

	/* The delay through a loaded switch is its intrinsic (unloaded) 
	delay plus the product of its resistance and output capacitance. */
	return Tdel_switch + R_switch * Cout_switch;
}

boolean *alloc_and_load_is_clock(boolean global_clocks) {

	/* Looks through all the logical_block to find and mark all the clocks, by setting *
	 * the corresponding entry in is_clock to true.  global_clocks is used     *
	 * only for an error check.                                                */

	int num_clocks, bnum, clock_net;
	boolean * is_clock;

	num_clocks = 0;

	is_clock = (boolean *) my_calloc(g_atoms_nlist.net.size(), sizeof(boolean));

	/* Want to identify all the clock nets.  */

	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_LATCH) {
			clock_net = logical_block[bnum].clock_net;
			assert(clock_net != OPEN);
			if (is_clock[clock_net] == FALSE) {
				is_clock[clock_net] = TRUE;
				num_clocks++;
			}
		} else {
			if (logical_block[bnum].clock_net != OPEN) {
				clock_net = logical_block[bnum].clock_net;
				if (is_clock[clock_net] == FALSE) {
					is_clock[clock_net] = TRUE;
					num_clocks++;
				}
			}
		}
	}

	/* If we have multiple clocks and we're supposed to declare them global, *
	 * print a warning message, since it looks like this circuit may have    *
	 * locally generated clocks.                                             */

	if (num_clocks > 1 && global_clocks) {
		vpr_printf_warning(__FILE__, __LINE__, 
				"Circuit contains %d clocks. All clocks will be marked global.\n", num_clocks);
	}

	return (is_clock);
}


