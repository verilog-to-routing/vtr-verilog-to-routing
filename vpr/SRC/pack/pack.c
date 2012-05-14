#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "read_xml_arch_file.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "prepack.h"
#include "pack.h"
#include "read_blif.h"
#include "ff_pack.h"
#include "cluster.h"
#include "output_clustering.h"
#include "ReadOptions.h"

/* #define DUMP_PB_GRAPH 1 */
/* #define DUMP_BLIF_INPUT 1 */

static void unclustered_stats (int max_lut_size);

void try_pack(INP struct s_packer_opts *packer_opts, INP const t_arch * arch, INP t_model *user_models, INP t_model *library_models) {
	boolean *is_clock;
	int num_models;
	t_model *cur_model;
	t_pack_patterns *list_of_packing_patterns;
	int num_packing_patterns;
	t_pack_molecule *list_of_pack_molecules;
	int num_pack_molecules;


	printf("Begin packing of %s \n", packer_opts->blif_file_name);

	/* determine number of models in the architecture */
	num_models = 0;
	cur_model = user_models;
	while(cur_model) {
		num_models++;
		cur_model = cur_model->next;
	}
	cur_model = library_models;
	while(cur_model) {
		num_models++;
		cur_model = cur_model->next;
	}

	/* begin parsing blif input file */
	read_blif (packer_opts->blif_file_name, 
				packer_opts->sweep_hanging_nets_and_inputs,
				user_models,
				library_models);
/* TODO: Do check blif here 
eg. 
 for(i = 0; i < num_logical_blocks; i++) {
	 if(logical_block[i].model->num_inputs > max_subblock_inputs) {
		 printf(ERRTAG "logical_block %s of model %s has %d inputs but architecture only supports subblocks up to %d inputs\n",
			 logical_block[i].name, logical_block[i].model->name, logical_block[i].model->num_inputs, max_subblock_inputs);
		 exit(1);
	 }
 }


*/
if (GetEchoOption()){
	echo_input (packer_opts->blif_file_name, "blif_input.echo", library_models);
}else;

	absorb_buffer_luts ();
	compress_netlist (); /* remove unused inputs */
	/* NB:  It's important to mark clocks and such *after* compressing the   *
	* netlist because the vpack_net numbers, etc. may be changed by removing      *
	* unused inputs .                                      */

	is_clock = alloc_and_load_is_clock (packer_opts->global_clocks);

	printf("\nAfter removing unused inputs:\n");
	printf("Total Blocks: %d.  Total Nets: %d.  Total inputs %d ouptuts %d\n", num_logical_blocks, num_logical_nets, 
	   num_p_inputs, num_p_outputs);

	printf("Begin prepacking\n");
	list_of_packing_patterns = alloc_and_load_pack_patterns(&num_packing_patterns);
	list_of_pack_molecules = alloc_and_load_pack_molecules(list_of_packing_patterns, num_packing_patterns, &num_pack_molecules);
	printf("Finish prepacking\n");


	/* Uncomment line below if you want a dump of compressed netlist. */
	/* if (GetEchoOption()){
		echo_input (packer_opts->blif_file_name, packer_opts->lut_size, "packed.echo"); 
	}else; */

	if (packer_opts->skip_clustering == FALSE) {
		do_clustering (arch,
					list_of_pack_molecules,
					num_models,
					packer_opts->global_clocks, 
					is_clock, 
					packer_opts->hill_climbing_flag,
					packer_opts->output_file, 
					packer_opts->timing_driven, 
					packer_opts->cluster_seed_type, 
					packer_opts->alpha, 
					packer_opts->beta,
					packer_opts->recompute_timing_after, 
					packer_opts->block_delay, 
					packer_opts->intra_cluster_net_delay, 
					packer_opts->inter_cluster_net_delay, 
					packer_opts->aspect,
					packer_opts->allow_unrelated_clustering, 
					packer_opts->allow_early_exit, 
					packer_opts->connection_driven,
					packer_opts->packer_algorithm);
	}
	else {
		printf("Skip clustering not supported\n");
		exit(1);
	}

	free (is_clock);

	saved_logical_blocks = logical_block;
	logical_block = NULL;
	num_saved_logical_blocks = num_logical_blocks;
	saved_logical_nets = vpack_net;
	vpack_net = NULL;
	num_saved_logical_nets = num_logical_nets; /* Todo: Use this for error checking */
	
	printf("\nNetlist conversion complete.\n\n");
}


void unclustered_stats (int max_lut_size) {

/* Dumps out statistics on an unclustered netlist -- i.e. it is just *
 * packed into VPACK_LUT + FF logic blocks, but not local routing from     *
 * output to input etc. is assumed.                                  */
	assert(0);
#if 0
 int iblk, num_logic_blocks, num_subckts;
 int min_inputs_used, min_clocks_used;
 int max_inputs_used, max_clocks_used;
 int summ_inputs_used, summ_clocks_used;
 int inputs_used, clocks_used;

 printf("\nUnclustered Netlist Statistics:\n\n");
 num_logic_blocks = num_logical_blocks - num_p_inputs - num_p_outputs;
 printf("%d Logic Blocks.\n", num_logic_blocks);

 min_inputs_used = max_lut_size+1;
 min_clocks_used = 2;
 max_inputs_used = -1;
 max_clocks_used = -1;
 summ_inputs_used = 0;
 summ_clocks_used = 0;
 num_subckts = 0;

 for (iblk=0;iblk<num_logical_blocks;iblk++) {
	 if (strcmp(logical_block[iblk].model->name, MODEL_LOGIC) == 0 || 
		 strcmp(logical_block[iblk].model->name, MODEL_LATCH) == 0) {
	   assert(logical_block[iblk].type == VPACK_COMB || logical_block[iblk].type != VPACK_LATCH);
       inputs_used = logical_block[iblk].num_input_nets;
	   if (logical_block[iblk].clock_net != OPEN)
          clocks_used = 1;
       else 
          clocks_used = 0;

       min_inputs_used = min (min_inputs_used, inputs_used);
       max_inputs_used = max (max_inputs_used, inputs_used);
       summ_inputs_used += inputs_used;

       min_clocks_used = min (min_clocks_used, clocks_used);
       max_clocks_used = max (max_clocks_used, clocks_used);
       summ_clocks_used += clocks_used;
    }
	 else if (strcmp(logical_block[iblk].model->name, MODEL_INPUT) != 0 &&
			  strcmp(logical_block[iblk].model->name, MODEL_OUTPUT) != 0 ) {
		assert(logical_block[iblk].type == VPACK_COMB);
		num_subckts++;
	}
 }

 printf("\n\t\t\tAverage\t\tMin\tMax\n");
 printf("Logic Blocks / Cluster\t%f\t%d\t%d\n", 1., 1, 1);
 printf("Used Inputs / Cluster\t%f\t%d\t%d\n", (float) summ_inputs_used /
        (float) num_logic_blocks, min_inputs_used, max_inputs_used);
 printf("Used Clocks / Cluster\t%f\t%d\t%d\n", (float) summ_clocks_used /
        (float) num_logic_blocks, min_clocks_used, max_clocks_used);
 printf("Subcircuits in design \t%d\n", num_subckts);
 printf("\n");
#endif
}

