#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <stdlib.h>
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

#ifdef USE_HMETIS
#include "hmetis_graph_writer.h"
static vtr::vector_map<AtomBlockId, int> read_hmetis_graph(string &hmetis_output_file_name, const int num_parts);
//TODO: CHANGE THIS HARDCODING
static string hmetis("/cygdrive/c/Source/Repos/vtr-verilog-to-routing/vpr/hmetis-1.5-WIN32/shmetis.exe ");
#endif

/* #define DUMP_PB_GRAPH 1 */
/* #define DUMP_BLIF_INPUT 1 */

static std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks);

void try_pack(t_packer_opts *packer_opts,
        const t_arch * arch,
		const t_model *user_models,
        const t_model *library_models,
        float interc_delay,
        vector<t_lb_type_rr_node> *lb_type_rr_graphs
#ifdef ENABLE_CLASSIC_VPR_STA
        , t_timing_inf timing_inf
#endif
        ) {
    std::unordered_set<AtomNetId> is_clock;
    std::multimap<AtomBlockId,t_pack_molecule*> atom_molecules; //The molecules associated with each atom block
    std::unordered_map<AtomBlockId,t_pb_graph_node*> expected_lowest_cost_pb_gnode; //The molecules associated with each atom block
	const t_model *cur_model;
	int num_models;
	t_pack_patterns *list_of_packing_patterns;
	int num_packing_patterns;
	t_pack_molecule *list_of_pack_molecules, * cur_pack_molecule;
#ifdef USE_HMETIS
	vtr::vector_map<AtomBlockId,int> partitions;
#endif
	vtr::printf_info("Begin packing '%s'.\n", packer_opts->blif_file_name.c_str());

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

    auto& atom_ctx = g_vpr_ctx.atom();

    size_t num_p_inputs = 0;
    size_t num_p_outputs = 0;
    for(auto blk_id : atom_ctx.nlist.blocks()) {
        auto type = atom_ctx.nlist.block_type(blk_id);
        if(type == AtomBlockType::INPAD) {
            ++num_p_inputs;
        } else if(type == AtomBlockType::OUTPAD) {
            ++num_p_outputs;
        }
    }

	vtr::printf_info("\n");
	vtr::printf_info("After removing unused inputs...\n");
	vtr::printf_info("\ttotal blocks: %zu, total nets: %zu, total inputs: %zu, total outputs: %zu\n",
		atom_ctx.nlist.blocks().size(), atom_ctx.nlist.nets().size(), num_p_inputs, num_p_outputs);

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

#ifdef USE_HMETIS
	if (!packer_opts->hmetis_input_file.empty()) {
		/*	hmetis.exe <GraphFile>			 <Nparts> <UBfactor> <Nruns> <Ctype> <RType> <VCycle> <Reconst> <dbglvl>
			  or
			hmetis.exe <GraphFile> <FixFile> <Nparts> <UBfactor> <Nruns> <Ctype> <RType> <VCycle> <Reconst> <dbglvl>
			 GraphFile: name of the hypergraph file
			 FixFile  : name of the file containing pre-assignment of vertices to partitions
			 Nparts   : number of partitions desired
			 UBfactor : balance between the partitions (e.g., use 5 for a 45-55 bisection balance)
			 Nruns    : Number of Iterations (shmetis defaults to 10)
			 CType    : HFC(1), FC(2), GFC(3), HEDGE(4), EDGE(5)
			 RType    : FM(1), 1WayFM(2), EEFM(3)
			 VCycle   : No(0), @End(1), ForMin(2), All(3)
			 Reconst  : NoReconstruct_HE(0), Reconstruct_HE (1)
			 dbglvl   : debug level*/

		vtr::printf_info("Writing hmetis hypergraph to %s\n", packer_opts->hmetis_input_file);

		// Write AtomNetlist into hGraph format
		write_hmetis_graph(packer_opts->hmetis_input_file);

		// Set the rest of the arguments
		// For a reference to what the string arguments mean, refer to the manual

		// The number of partitions is determined in one of two methods:
		// 1. Partition tree would result in the size of subcircuits ~= 4 (Elias Vansteenkiste, et al.)
		// 2. Partition depth = 5, i.e. num_parts = 32 (Doris Chen, et al.)
		//int num_parts = atom_ctx.nlist.blocks().size() / 4; // Method 1.
		//TODO: Find an appropriate value (may be from packer_opts) for num_parts
		int num_parts = 32; //Method 2.

		string run_hmetis = hmetis + packer_opts->hmetis_input_file + " " + to_string(num_parts) + " 5";

		// Check if OS is Windows or Linux and run hMetis accordingly
		//TODO: USE HMETIS WITH ALL ARGUMENTS INSTEAD FOR FURTHER REFINEMENT
		//Using shmetis (standard hmetis) to simplify
		system(run_hmetis.c_str());

		/* Parse the output file from hMetis, contains |V| lines, ith line = partition of V_i.
		*  Store the results into a vector_map.
		*/
		string hmetis_output_file(packer_opts->hmetis_input_file + ".part." + to_string(num_parts));

		partitions = read_hmetis_graph(hmetis_output_file, num_parts);

		// Print each block's partition
		vtr::printf_info("Partitioning complete\n");

		vector<vector<AtomBlockId>> print_partitions(num_parts);
		for (auto blk_id : atom_ctx.nlist.blocks()) {
			print_partitions[partitions[blk_id]].push_back(blk_id);
		}
		for (int i = 0; i < (int)print_partitions.size(); i++) {
			vtr::printf_info("Blocks in partition %zu:\n\t", i);
			for (int j = 0; j < (int)print_partitions[i].size(); j++)
				vtr::printf_info("%zu ", size_t(print_partitions[i][j]));
			vtr::printf_info("\n");
		}
	}
#endif

    do_clustering(arch, list_of_pack_molecules, num_models,
            packer_opts->global_clocks, is_clock,
            atom_molecules,
            expected_lowest_cost_pb_gnode,
            packer_opts->hill_climbing_flag, packer_opts->output_file.c_str(),
            packer_opts->timing_driven, packer_opts->cluster_seed_type,
            packer_opts->alpha, packer_opts->beta,
            packer_opts->inter_cluster_net_delay,
            packer_opts->target_device_utilization,
            packer_opts->allow_unrelated_clustering,
            packer_opts->connection_driven,
            packer_opts->packer_algorithm,
            lb_type_rr_graphs,
            packer_opts->device_layout,
            packer_opts->debug_clustering,
            packer_opts->enable_pin_feasibility_filter,
            packer_opts->target_external_pin_util
#ifdef USE_HMETIS
			, partitions
#endif
#ifdef ENABLE_CLASSIC_VPR_STA
            , timing_inf
#endif
            );

	/*free list_of_pack_molecules*/
	free_list_of_pack_patterns(list_of_packing_patterns, num_packing_patterns);

	cur_pack_molecule = list_of_pack_molecules;
	while (cur_pack_molecule != nullptr){
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
    auto& device_ctx = g_vpr_ctx.device();

    Tdel_switch = device_ctx.arch_switch_inf[switch_index].Tdel(switch_fanin);
	R_switch = device_ctx.arch_switch_inf[switch_index].R;
	Cout_switch = device_ctx.arch_switch_inf[switch_index].Cout;

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
    auto& atom_ctx = g_vpr_ctx.atom();

    for(auto blk_id : atom_ctx.nlist.blocks()) {
        for(auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
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

#ifdef USE_HMETIS
/* Reads in the output of the hMetis partitioner and returns
*  A 2-D vector that contains all the blocks in each partition
*/
static vtr::vector_map<AtomBlockId, int> read_hmetis_graph(string &hmetis_output_file, const int num_parts) {
	ifstream fp(hmetis_output_file.c_str());
	vtr::vector_map<AtomBlockId, int> partitions;
	string line;
	int block_num = 0; //Indexing for CLB's start at 0

	// Check that # of lines in output file matches # of blocks/vertices
	auto& atom_ctx = g_vpr_ctx.atom();
	VTR_ASSERT((int)atom_ctx.nlist.blocks().size() == count(istreambuf_iterator<char>(fp), istreambuf_iterator<char>(), '\n'));

	partitions.resize(atom_ctx.nlist.blocks().size());

	// Reset the filestream to the beginning and reread
	fp.clear();
	fp.seekg(0, fp.beg);

	while (getline(fp, line)) {
		if (stoi(line) >= num_parts) {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"Partition for line '%s' exceeds the set number of partitions %d" , line, num_parts);
		}

		partitions[AtomBlockId(block_num)] = stoi(line);
		block_num++;
	}

	fp.close();

	return partitions;
}
#endif
