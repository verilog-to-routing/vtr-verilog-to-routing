#include "device_size_estimate.h"

#include "vpr_utils.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "globals.h"   // for g_vpr_ctx
#include "physical_types.h"
#include "setup_grid.h"
#include "cluster_util.h"
#include "vtr_math.h"

std::map<t_logical_block_type_ptr, size_t> DeviceSizeEstimator::estimate_resource_requirement(const Prepacker& prepacker,
                                                                                              const t_packer_opts& packer_opts,
                                                                                              const LogicalModels& models) const {
    vtr::ScopedStartFinishTimer timer("Estimate Resource Requirement");

    VTR_LOG("num_type_instances is empty; faking demand of 10 instances for each logical block type.\n");

    auto& device_ctx = g_vpr_ctx.mutable_device();

    std::map<t_logical_block_type_ptr, size_t> num_type_instances_capacity;

    const auto& atom_ctx = g_vpr_ctx.atom();

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types = identify_primitive_candidate_block_types();

    //VTR_LOG("Iterating over all atoms once.\n");
    // My naive first implementation idea is to use the first candidate block
    // type always. Another naive part will be assuming that clusters will
    // contain only the same lowest cost primitive. Then summing those numbers
    // at the end. This is of course different not how the packer works but I
    // assume this can give us a good estimation number as a starting point.
    std::map<const t_pb_graph_node*, size_t> primitive_usage_count;
    std::map<const t_pb_graph_node*, t_logical_block_type_ptr> primitive_to_logical_type;
    for (AtomBlockId atom_blk_id: atom_ctx.netlist().blocks()) {
        VTR_LOG("\tCurrrent atom id is %zu\n", atom_blk_id);
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);

        auto root_model_id = atom_ctx.netlist().block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        
        // TODO: Currrently, I will try to map the first type always. However,
        //       we have multiple candidates in some architectures. So this
        //       should be extended to multiple case and the logic of this loop
        //       should be adapted accordingly.
        // for (t_logical_block_type_ptr candidate_type: candidate_types)
        {
            t_logical_block_type_ptr candidate_type = candidate_types[0];
            std::vector<t_logical_block_type> candidate_logical_type = {*candidate_type};
            const t_pb_graph_node* candidate_prim = prepacker.get_expected_lowest_cost_primitive_for_atom_block(atom_blk_id, candidate_logical_type);
            
            primitive_usage_count[candidate_prim]++;
            primitive_to_logical_type[candidate_prim] = candidate_type;

            int capacity = candidate_prim->total_primitive_count;
            VTR_LOG("\t\tCandidate type %s can get max of %d of that atom.\n", candidate_type->name.c_str(), capacity);
        }
    }

    // TODO: Here we may want to check the clustering options like pin
    //       utilization or in-cluster utilization. It can be directly
    //       reflected here as an inverse multiplier.
    VTR_LOG("Primitive usage summary by capacity:\n");
    for (auto& [prim, count] : primitive_usage_count) {
        t_logical_block_type_ptr logical_type = primitive_to_logical_type[prim];
        size_t inferred_number = std::ceil(vtr::safe_ratio<float>(count, prim->total_primitive_count));
        VTR_LOG("  Primitive %s[%d] used by %zu atoms. One cluster (%s) can hold %d of it. Inferring %zu clusters.\n",
                prim->pb_type->name,
                prim->placement_index,
                count,
                logical_type->name.c_str(),
                prim->total_primitive_count,
                inferred_number);
        num_type_instances_capacity[logical_type] += inferred_number;
    }

    // Above is for the cluster capacity case. Now, i will try only with the external input pin counts.
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_input_pin;
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_output_pin;
    std::map<t_logical_block_type_ptr, std::vector<PackMoleculeId>> logical_type_molecules;
    for (PackMoleculeId mol_id: prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_atom = mol.atom_block_ids[mol.root];
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);

        auto root_model_id = atom_ctx.netlist().block_model(root_atom);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        // TODO: Currrently, I will try to map the first type always. However,
        //       we have multiple candidates in some architectures. So this
        //       should be extended to multiple case and the logic of this loop
        //       should be adapted accordingly.
        // for (t_logical_block_type_ptr candidate_type: candidate_types)
        {
            t_logical_block_type_ptr candidate_type = candidate_types[0];
            logical_type_molecules[candidate_type].push_back(mol_id);
        }
    }

    VTR_LOG("Inferring cluster counts by used external input pin numbers:\n");
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        int input_pin_number_of_type = logical_type->pb_type->num_input_pins;
        int used_inputs = 0;
        for (PackMoleculeId mol_id: mol_ids) {
            int num_ext_inputs = prepacker.calc_molecule_stats(mol_id, atom_ctx.netlist(), models).num_used_ext_inputs;
            used_inputs += num_ext_inputs;
            if (used_inputs == input_pin_number_of_type) {
                num_type_instances_by_input_pin[logical_type] ++;
                used_inputs = 0;
            } else if (used_inputs > input_pin_number_of_type) {
                num_type_instances_by_input_pin[logical_type] ++;
                used_inputs = num_ext_inputs;
            }
        }
        if (used_inputs > 0) {
            num_type_instances_by_input_pin[logical_type] ++;
        }
        int output_pin_number_of_type = logical_type->pb_type->num_output_pins;
        int used_outputs = 0;
        for (PackMoleculeId mol_id: mol_ids) {
            int num_ext_outputs = prepacker.calc_molecule_stats(mol_id, atom_ctx.netlist(), models).num_used_ext_outputs;
            used_outputs += num_ext_outputs;
            if (used_outputs == output_pin_number_of_type) {
                num_type_instances_by_output_pin[logical_type] ++;
                used_outputs = 0;
            } else if (used_outputs > output_pin_number_of_type) {
                num_type_instances_by_output_pin[logical_type] ++;
                used_outputs = num_ext_outputs;
            }
        }
        if (used_outputs > 0) {
            num_type_instances_by_output_pin[logical_type] ++;
        }
    }

    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_avg_pin;
    VTR_LOG("Logic type usage summary by external pins:\n");
    for (auto& [logical_type, inferred_num]: num_type_instances_by_input_pin) {
        VTR_LOG("  %s (input):     %zu\n", logical_type->name.c_str(), inferred_num);
        VTR_LOG("  %s (output):    %zu\n", logical_type->name.c_str(), num_type_instances_by_output_pin[logical_type]);
        size_t avg_number = std::ceil(vtr::safe_ratio<float>(num_type_instances_by_input_pin[logical_type] + num_type_instances_by_output_pin[logical_type], 2));
        num_type_instances_by_avg_pin[logical_type] = avg_number;
        VTR_LOG("    %s (average): %zu\n", logical_type->name.c_str(), avg_number);
    }

    // For the io constrained devices, i found the number pin number constraint is too loose.
    // So i will be maxing the average pin number and capacity above.
    VTR_LOG("Taking max of capacity and pin number:\n");
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_pin_and_capacity;
    for (auto& [logical_type, inferred_num]: num_type_instances_by_avg_pin) {
        VTR_LOG("  %s (pin):          %zu\n", logical_type->name.c_str(), inferred_num);
        VTR_LOG("  %s (capacity):     %zu\n", logical_type->name.c_str(), num_type_instances_capacity[logical_type]);
        size_t max_num = std::max(num_type_instances_by_avg_pin[logical_type], num_type_instances_capacity[logical_type]);
        num_type_instances_pin_and_capacity[logical_type] = max_num;
        VTR_LOG("    %s (max):        %zu\n", logical_type->name.c_str(), max_num);
    }



    return num_type_instances_pin_and_capacity;
}

DeviceSizeEstimator::DeviceSizeEstimator(const std::string& device_layout,
                                         const t_packer_opts& packer_opts,
                                         const std::vector<t_grid_def>& grid_layouts,
                                         const LogicalModels& models,
                                         const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estime Device Size");
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed (%s), no need to estimate.\n", device_layout.c_str());
        return;
    }

    VTR_LOG("Device is not fixed, need to estimate the resource requirement.\n");
    std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker, packer_opts, models);
    
    VTR_LOG("Estimated resoure requirement:\n");
    for (auto& [type_ptr, count] : num_type_instances) {
        VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);
    }

    // Not sure about do we always want to cast this to global.
    VTR_LOG("Cast the update to global device\n");
    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.grid = create_device_grid(device_layout, grid_layouts, num_type_instances, packer_opts.target_device_utilization);

    // Report the updated device.
    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n", device_ctx.grid.width(), device_ctx.grid.height(), num_grid_tiles, device_ctx.grid.name().c_str());
}