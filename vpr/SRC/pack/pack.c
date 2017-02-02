#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"

#include "vpr_error.h"
#include "vpr_types.h"

#include "read_xml_arch_file.h"
#include "globals.h"
#include "atom_netlist.h"
#include "prepack.h"
#include "pack_types.h"
#include "pack.h"
#include "read_blif.h"
#include "cluster.h"
#include "output_clustering.h"
#include "ReadOptions.h"

/* #define DUMP_PB_GRAPH 1 */
/* #define DUMP_BLIF_INPUT 1 */

static std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks);

void try_pack(struct s_packer_opts *packer_opts, const t_arch * arch,
		const t_model *user_models, const t_model *library_models, float interc_delay, vector<t_lb_type_rr_node> *lb_type_rr_graphs) {
    std::unordered_set<AtomNetId> is_clock;
    std::multimap<AtomBlockId,t_pack_molecule*> atom_molecules; //The molecules associated with each atom block
    std::unordered_map<AtomBlockId,t_pb_graph_node*> expected_lowest_cost_pb_gnode; //The molecules associated with each atom block
	int num_models;
	const t_model *cur_model;
	t_pack_patterns *list_of_packing_patterns;
	int num_packing_patterns;
	t_pack_molecule *list_of_pack_molecules, * cur_pack_molecule;

	vtr::printf_info("Begin packing '%s'.\n", packer_opts->blif_file_name);

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

    size_t num_p_inputs = 0;
    size_t num_p_outputs = 0;
    for(auto blk_id : g_atom_nl.blocks()) {
        auto type = g_atom_nl.block_type(blk_id);
        if(type == AtomBlockType::INPAD) {
            ++num_p_inputs;
        } else if(type == AtomBlockType::OUTPAD) {
            ++num_p_outputs;
        }
    }
	
	vtr::printf_info("\n");
	vtr::printf_info("After removing unused inputs...\n");
	vtr::printf_info("\ttotal blocks: %zu, total nets: %zu, total inputs: %zu, total outputs: %zu\n",
		g_atom_nl.blocks().size(), g_atom_nl.nets().size(), num_p_inputs, num_p_outputs);

	vtr::printf_info("Begin prepacking.\n");
	list_of_packing_patterns = alloc_and_load_pack_patterns(&num_packing_patterns);
    list_of_pack_molecules = alloc_and_load_pack_molecules(list_of_packing_patterns, 
                                atom_molecules,
                                expected_lowest_cost_pb_gnode,
                                num_packing_patterns);
	vtr::printf_info("Finish prepacking.\n");

	if(packer_opts->auto_compute_inter_cluster_net_delay) {
		packer_opts->inter_cluster_net_delay = interc_delay;
		vtr::printf_info("Using inter-cluster delay: %g\n", packer_opts->inter_cluster_net_delay);
	}

	if (packer_opts->skip_clustering == false) {
		do_clustering(arch, list_of_pack_molecules, num_models,
				packer_opts->global_clocks, is_clock, 
                atom_molecules,
                expected_lowest_cost_pb_gnode,
				packer_opts->hill_climbing_flag, packer_opts->output_file,
				packer_opts->timing_driven, packer_opts->cluster_seed_type,
				packer_opts->alpha, packer_opts->beta,
				packer_opts->inter_cluster_net_delay, packer_opts->aspect,
				packer_opts->allow_unrelated_clustering,
				packer_opts->connection_driven,
				packer_opts->packer_algorithm,
				lb_type_rr_graphs);
	} else {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, 
				"Skip clustering no longer supported.\n");
	}

	/*free list_of_pack_molecules*/
	free_list_of_pack_patterns(list_of_packing_patterns, num_packing_patterns);

	cur_pack_molecule = list_of_pack_molecules;
	while (cur_pack_molecule != NULL){
		cur_pack_molecule = list_of_pack_molecules->next;
		delete list_of_pack_molecules;
		list_of_pack_molecules = cur_pack_molecule;
	}

	vtr::printf_info("\n");
	vtr::printf_info("Netlist conversion complete.\n");
	vtr::printf_info("\n");
}

float get_arch_switch_info(short switch_index, int switch_fanin, float &Tdel_switch, float &R_switch, float &Cout_switch){
	/* Fetches delay, resistance and output capacitance of the architecture switch at switch_index. 
	Returns the total delay through the switch. Used to calculate inter-cluster net delay. */

	/* The intrinsic delay may depend on fanin to the switch. If the delay map of a 
	   switch from the architecture file has multiple (#inputs, delay) entries, we
	   interpolate/extrapolate to get the delay at 'switch_fanin'. */
	std::map<int, double> *Tdel_map = &g_arch_switch_inf[switch_index].Tdel_map;
	if (Tdel_map->size() == 1){
		Tdel_switch = (Tdel_map->begin())->second;
	} else {
		Tdel_switch = vtr::linear_interpolate_or_extrapolate(Tdel_map, switch_fanin);
	}
	R_switch = g_arch_switch_inf[switch_index].R;
	Cout_switch = g_arch_switch_inf[switch_index].Cout;

	/* The delay through a loaded switch is its intrinsic (unloaded) 
	delay plus the product of its resistance and output capacitance. */
	return Tdel_switch + R_switch * Cout_switch;
}

std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks) {

	/* Looks through all the atom blocks to find and mark all the clocks, by setting
	 * the corresponding entry by adding the clock to is_clock.
     * global_clocks is used 
	 * only for an error check.                                                */

	int num_clocks = 0;
    std::unordered_set<AtomNetId> is_clock;

	/* Want to identify all the clock nets.  */

    for(auto blk_id : g_atom_nl.blocks()) {
        for(auto pin_id : g_atom_nl.block_clock_pins(blk_id)) {
            auto net_id = g_atom_nl.pin_net(pin_id);
            if (!is_clock.count(net_id)) {
                is_clock.insert(net_id);
                num_clocks++;
            }
        }
	}

	/* If we have multiple clocks and we're supposed to declare them global, *
	 * print a warning message, since it looks like this circuit may have    *
	 * locally generated clocks.                                             */

	if (num_clocks > 1 && global_clocks) {
		vtr::printf_warning(__FILE__, __LINE__, 
				"All %d clocks will be treated as global.\n", num_clocks);
	}

	return (is_clock);
}


