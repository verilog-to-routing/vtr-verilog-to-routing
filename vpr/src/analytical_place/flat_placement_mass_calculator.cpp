/**
 * @file
 * @author  Alex Singer
 * @date    February 2024
 * @brief   Implementation of the mass calculator used in the AP flow.
 */

#include "flat_placement_mass_calculator.h"
#include <cstring>
#include <queue>
#include <vector>
#include "ap_mass_report.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "atom_netlist_fwd.h"
#include "logic_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
#include "vtr_log.h"
#include "vtr_vector.h"

/**
 * @brief Returns true if the given pb_type is a primitive and is a memory class.
 */
static bool is_primitive_memory_pb_type(const t_pb_type* pb_type) {
    VTR_ASSERT_SAFE(pb_type != nullptr);

    if (!pb_type->is_primitive())
        return false;

    if (pb_type->class_type != MEMORY_CLASS)
        return false;

    return true;
}

/**
 * @brief Calculate the cost of the given pb_type. This cost is a value that
 *        represents how much mass in a specific dimension a physical block
 *        takes up.
 */
static float calc_pb_type_cost(const t_pb_type* pb_type) {
    // If this primitive represents a memory, count the number of bits this memory
    // can store and return that as the cost.
    if (is_primitive_memory_pb_type(pb_type)) {
        // Get the number of address and data pins to compute the number of bits
        // this memory can store.
        //      number_bits = num_data_out_pins * 2^num_address_pins
        int num_address_pins = 0;
        int num_data_out_pins = 0;
        int num_address1_pins = 0;
        int num_data_out1_pins = 0;
        int num_address2_pins = 0;
        int num_data_out2_pins = 0;
        for (int i = 0; i < pb_type->num_ports; i++) {
            if (pb_type->ports[i].port_class == nullptr)
                continue;

            std::string port_class = pb_type->ports[i].port_class;
            if (port_class == "address")
                num_address_pins += pb_type->ports[i].num_pins;
            if (port_class == "data_out")
                num_data_out_pins += pb_type->ports[i].num_pins;
            if (port_class == "address1")
                num_address1_pins += pb_type->ports[i].num_pins;
            if (port_class == "data_out1")
                num_data_out1_pins += pb_type->ports[i].num_pins;
            if (port_class == "address2")
                num_address2_pins += pb_type->ports[i].num_pins;
            if (port_class == "data_out2")
                num_data_out2_pins += pb_type->ports[i].num_pins;
        }

        // Compute the number of bits this could store if it was a single port
        // memory.
        int single_port_num_bits = 0;
        if (num_address_pins > 0) {
            VTR_ASSERT_MSG(num_address1_pins == 0 && num_address2_pins == 0,
                           "Cannot be a single port and dual port memory");
            single_port_num_bits = (1 << num_address_pins) * num_data_out_pins;
        }

        // Compute the number of bits this could store if it was a dual port memory.
        // Need to compute the number of bits for each of the ports.
        int dual_port1_num_bits = 0;
        if (num_address1_pins > 0) {
            // FIXME: Found that the Titan architecture fails this test! Raise
            //        an issue on this to maybe update the documentation or fix
            //        the architecture.
            //        See: prim_ram_1Kx9
            //        Perhaps the architecture parsing file should check for this.
            // VTR_ASSERT_MSG(num_address_pins == 0 && num_address2_pins > 0,
            //                "Ill-formed dual port memory");
            dual_port1_num_bits = (1 << num_address1_pins) * num_data_out1_pins;
        }
        int dual_port2_num_bits = 0;
        if (num_address2_pins > 0) {
            VTR_ASSERT_MSG(num_address_pins == 0 && num_address1_pins > 0,
                           "Ill-formed dual port memory");
            dual_port2_num_bits = (1 << num_address2_pins) * num_data_out2_pins;
        }
        // Note: We take the max of the two dual port num bits since dual ports
        //       access the same memory but with different address bits.
        int total_dual_port_num_bits = std::max(dual_port1_num_bits, dual_port2_num_bits);

        // Get the total number of bits. We assume that this memory will be either
        // a single port or dual port memory, therefore we can just sum these two
        // terms. If it was a single port, the dual port num bits will be 0 and
        // vice-versa.
        int total_num_bits = single_port_num_bits + total_dual_port_num_bits;
        VTR_ASSERT_MSG(total_num_bits != 0,
                       "Cannot deduce number of bits in memory primitive.");
        return total_num_bits;
    }

    // If this is not a memory, count the number of input and output pins.
    // This makes physical blocks with more pins have more size than those with
    // less. For example, a 4 lut would have a lower cost than a 5 lut.
    float pb_cost = pb_type->num_input_pins + pb_type->num_output_pins + pb_type->num_clock_pins;
    return pb_cost;
}

/**
 * @brief Returns the mass of the given atom block.
 */
static float get_atom_mass(AtomBlockId blk_id, const Prepacker& prepacker, const AtomNetlist& atom_netlist) {

    // Get the physical block that this block will most likely be packed into.
    // This will give us a better estimate for how many resources this atom will
    // use. For example, a LUT atom may only use 2 input pins; however, the
    // architecture may only have 4-LUTs and 5-LUTs available. This method should
    // return the smallest compatible resource for this atom (i.e. the 4-LUT).
    const t_pb_graph_node* primitive = prepacker.get_expected_lowest_cost_pb_gnode(blk_id);

    // Get the physical cost of this atom. This is the cost of the atom when it
    // is physically implemented on the device. For example, the 2-LUT above may
    // be at best implemented by a 4-LUT which may give it a cost of 5 pins.
    float physical_cost = calc_pb_type_cost(primitive->pb_type);

    // Get the mass of this atom (i.e. how much resources on the device this atom
    // will likely use).
    float mass = 0.0f;
    if (!is_primitive_memory_pb_type(primitive->pb_type)) {
        // If this is not a memory, we use a weighted sum of the physical cost
        // (how much resources we expect it to use physically on the device) and
        // the logical cost (how much resources is it actually using).

        // The logical cost is the sum of the used pins on the atom block. For
        // the 2-LUT example, this would have a logical cost of 3 (2 inputs and
        // 1 output).
        size_t num_logical_input_pins = atom_netlist.block_input_pins(blk_id).size();
        size_t num_logical_clock_pins = atom_netlist.block_clock_pins(blk_id).size();
        size_t num_logical_output_pins = atom_netlist.block_output_pins(blk_id).size();
        float logical_cost = num_logical_input_pins + num_logical_clock_pins + num_logical_output_pins;

        // We take a weighted sum of the physical and logical cost. The physical
        // cost is an over-estimation of how many resources the atom will use
        // since, if less pins are used, the atom may be able to "share" its
        // inputs. The logical cost is closer to an underestimate. We use a weighted
        // sum to try and account for the unknowns about the eventual packing.
        mass = (0.8f * physical_cost) + (0.2f * logical_cost);
    } else {
        // For memories, mass is just the physical cost.
        // NOTE: It is important for the physical and logical costs to have
        //       "the same units". Since memories have a cost proportional to
        //       the number of bits within the memory, we cannot do a weighted
        //       sum with the number of pins.
        // TODO: Should use the number of input and output pins to compute
        //       logical memory cost (i.e. how many bits the atom actually needs).
        mass = physical_cost;
    }

    // Currently, the code does not handle well with fractional masses. Rounding
    // up to the nearest whole number.
    mass = std::ceil(mass);

    return mass;
}

// This method is being forward-declared due to the double recursion below.
// Eventually this should be made into a non-recursive algorithm for performance,
// however this is not in a performance critical part of the code.
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type,
                                             std::set<PrimitiveVectorDim>& memory_model_dims,
                                             const PrimitiveDimManager& dim_manager);

/**
 * @brief Get the amount of primitives this mode can contain.
 *
 * This is part of a double recursion, since a mode contains primitives which
 * themselves have modes.
 */
static PrimitiveVector calc_mode_capacity(const t_mode& mode,
                                          std::set<PrimitiveVectorDim>& memory_model_dims,
                                          const PrimitiveDimManager& dim_manager) {
    // Accumulate the capacities of all the pbs in this mode.
    PrimitiveVector capacity;
    for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
        const t_pb_type& pb_type = mode.pb_type_children[pb_child_idx];
        PrimitiveVector pb_capacity = calc_pb_type_capacity(&pb_type, memory_model_dims, dim_manager);
        capacity += pb_capacity;
    }
    return capacity;
}

/**
 * @brief Get the amount of primitives this pb can contain.
 *
 * This is the other part of the double recursion. A pb may have multiple modes.
 * Modes are made of pbs.
 */
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type,
                                             std::set<PrimitiveVectorDim>& memory_model_dims,
                                             const PrimitiveDimManager& dim_manager) {
    // Since a pb cannot be multiple modes at the same time, we do not
    // accumulate the capacities of the mode. Instead we need to "mix" the two
    // capacities as if the pb could choose either one.
    PrimitiveVector capacity;
    // If this is a leaf / primitive, create the base PrimitiveVector capacity.
    if (pb_type->is_primitive()) {
        LogicalModelId model_id = pb_type->model_id;
        VTR_ASSERT_SAFE(model_id.is_valid());
        PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
        VTR_ASSERT(dim.is_valid());
        if (pb_type->class_type == MEMORY_CLASS)
            memory_model_dims.insert(dim);
        float pb_type_cost = calc_pb_type_cost(pb_type);
        capacity.add_val_to_dim(pb_type_cost * pb_type->num_pb, dim);
        return capacity;
    }
    // For now, we simply mix the capacities of modes by taking the max of each
    // dimension of the capcities. This provides an upper-bound on the amount of
    // primitives this pb can contain.
    for (int mode = 0; mode < pb_type->num_modes; mode++) {
        PrimitiveVector mode_capacity = calc_mode_capacity(pb_type->modes[mode], memory_model_dims, dim_manager);
        capacity = PrimitiveVector::max(capacity, mode_capacity);
    }

    // A pb_type represents a heirarchy of physical blocks that can be implemented,
    // with leaves of primitives at the bottom of the heirarchy. A pb_type will have
    // many children, each with their own physical cost; however, a parent pb_type
    // should not have higher cost than its children. For example, here we use
    // pin counts to represent cost. The children of the pb_type cannot use more
    // pins than the parent has available. Therefore, each dimension of the
    // pb_type cannot have a higher cost than the parent.
    // Clamp the capacity by the physical cost of this pb_type.
    float pb_type_physical_cost = calc_pb_type_cost(pb_type);
    for (PrimitiveVectorDim dim : capacity.get_non_zero_dims()) {
        // If this dimension corresponds to a memory, do not clamp the capacity.
        // Memories count bits, so clamping by the number of pins of the parent
        // does not really make sense.
        if (memory_model_dims.count(dim) != 0)
            continue;
        capacity.set_dim_val(dim, std::min(capacity.get_dim_val(dim), pb_type_physical_cost));
    }

    // A pb_type may contain multiple copies of the same PB. Multiply the capacity
    // by the number of pb.
    capacity *= pb_type->num_pb;

    return capacity;
}

/**
 * @brief Calculate the cpacity of the given logical block type.
 */
static PrimitiveVector calc_logical_block_type_capacity(const t_logical_block_type& logical_block_type,
                                                        const PrimitiveDimManager& dim_manager) {
    // If this logical block is empty, it cannot contain any primitives.
    if (logical_block_type.is_empty())
        return PrimitiveVector();

    std::set<PrimitiveVectorDim> memory_model_dims;
    PrimitiveVector capacity = calc_pb_type_capacity(logical_block_type.pb_type,
                                                     memory_model_dims,
                                                     dim_manager);

    // The primitive capacity of a logical block is the primitive capacity of
    // its root pb.
    return capacity;
}

/**
 * @brief Get the primitive capacity of the given sub_tile.
 *
 * Sub_tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param sub_tile                         The sub_tile to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static PrimitiveVector calc_sub_tile_capacity(const t_sub_tile& sub_tile,
                                              const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Similar to getting the primitive capacity of the pb, sub_tiles have many
    // equivalent sites, but it can only be one of them at a time. Need to "mix"
    // the capacities of the different sites this sub_tile may be.
    PrimitiveVector capacity;
    for (t_logical_block_type_ptr block_type : sub_tile.equivalent_sites) {
        const PrimitiveVector& block_capacity = logical_block_type_capacities[block_type->index];
        // Currently, we take the max of each primitive dimension as an upper
        // bound on the capacity of the sub_tile.
        capacity = PrimitiveVector::max(capacity, block_capacity);
    }
    return capacity;
}

/**
 * @brief Get the primitive capacity of a tile of the given type.
 *
 * Tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param tile_type                        The tile type to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static PrimitiveVector calc_physical_tile_type_capacity(const t_physical_tile_type& tile_type,
                                                        const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Accumulate the capacities of all the sub_tiles in the given tile type.
    PrimitiveVector capacity;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        PrimitiveVector sub_tile_capacity = calc_sub_tile_capacity(sub_tile, logical_block_type_capacities);
        // A tile may contain many sub_tiles of the same type. Multiply by the
        // number of sub_tiles of this type.
        sub_tile_capacity *= sub_tile.capacity.total();
        capacity += sub_tile_capacity;
    }
    return capacity;
}

/**
 * @brief Get the primitive mass of the given block.
 *
 * This returns an M-dimensional vector with each entry indicating the mass of
 * that primitive type in this block. M is the number of unique models
 * (primitive types) in the architecture.
 */
static PrimitiveVector calc_block_mass(APBlockId blk_id,
                                       const PrimitiveDimManager& dim_manager,
                                       const APNetlist& netlist,
                                       const Prepacker& prepacker,
                                       const AtomNetlist& atom_netlist) {
    PrimitiveVector mass;
    PackMoleculeId mol_id = netlist.block_molecule(blk_id);
    const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
    for (AtomBlockId atom_blk_id : mol.atom_block_ids) {
        // See issue #2791, some of the atom_block_ids may be invalid. They can
        // safely be ignored.
        if (!atom_blk_id.is_valid())
            continue;

        // Get the dimension in the vector to add value to.
        LogicalModelId model_id = atom_netlist.block_model(atom_blk_id);
        VTR_ASSERT_SAFE(model_id.is_valid());
        PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
        VTR_ASSERT(dim.is_valid());

        // Get the amount of mass in this dimension to add.
        float atom_mass = get_atom_mass(atom_blk_id, prepacker, atom_netlist);

        // Add mass to the dimension.
        mass.add_val_to_dim(atom_mass, dim);
    }
    return mass;
}

namespace {

/**
 * @brief A struct to hold information on pb types which act like one-hot primitves.
 */
struct OneHotPbType {
    /// @brief The root pb type which contains the modes which act in a one-hot
    ///        fashion.
    t_pb_type* pb_type;

    /// @brief The models which will all share a dimension due to being a part
    ///        of this pb type.
    std::set<LogicalModelId> shared_models;
};

} // namespace

/**
 * @brief Search the architecture for pb types which implement models in a "one-hot" way.
 *
 * A one-hot pb type is a pb_type which effectively acts like a one-hot
 * encoding for a set of models. A good example of this is IO pb types. An
 * IO pb type can either be an input or an output but not both. These pb_types
 * should be treated as a single dimension in the primitive vector since they
 * are mutually exclusive (i.e. using the resource of one is the same as using
 * the resource of another).
 */
static std::vector<OneHotPbType> find_one_hot_pb_types(const std::vector<t_logical_block_type>& logical_block_types,
                                                       const LogicalModels& models,
                                                       int log_verbosity) {
    // Populate a lookup between each model and the primitives that implement them.
    // This is simply done by doing a BFS on the complex graph and accumulating
    // any pb_types that implement each of the models.
    VTR_LOGV(log_verbosity >= 10, "Creating lookup for the model primitives...\n");
    vtr::vector<LogicalModelId, std::set<t_pb_type*>> model_primitives(models.all_models().size());

    // Initialize a queue for the pb_types to check.
    std::queue<t_pb_type*> pb_type_queue;
    for (const t_logical_block_type& block_type : logical_block_types) {
        pb_type_queue.push(block_type.pb_type);
    }

    // BFS over the pb_types.
    while (!pb_type_queue.empty()) {
        t_pb_type* pb_type = pb_type_queue.front();
        pb_type_queue.pop();
        if (pb_type == nullptr)
            continue;

        // If this pb_type is a primitive, add it to the model primitives of the
        // model that implements it.
        if (pb_type->is_primitive()) {
            model_primitives[pb_type->model_id].insert(pb_type);
            continue;
        }

        // Explore all of the children of this pb_type by adding them to the queue.
        for (int mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
            const t_mode& mode = pb_type->modes[mode_idx];
            for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
                pb_type_queue.push(&mode.pb_type_children[pb_child_idx]);
            }
        }
    }

    // Search for the one-hot pb types.
    // Some properties of the one-hot pb types that this search is looking for:
    //      - it is a pb_type
    //      - it has more than one mode
    //      - each mode implements a single, unique primitive
    //          - single: num_pb == 1 (this makes the cost function easier)
    //          - unique: this primitive implements a model that is not used anywhere else.
    // This search is done using a BFS similar to above.
    VTR_LOGV(log_verbosity >= 10, "Searching for one-hot primitives...\n");
    std::vector<OneHotPbType> one_hot_pb_types;

    // Initialize the queue with the pb type for each of the logical blocks.
    VTR_ASSERT(pb_type_queue.empty());
    for (const t_logical_block_type& block_type : logical_block_types) {
        pb_type_queue.push(block_type.pb_type);
    }

    // Perform a BFS over the queue.
    while (!pb_type_queue.empty()) {
        t_pb_type* pb_type = pb_type_queue.front();
        pb_type_queue.pop();
        if (pb_type == nullptr)
            continue;

        // If this is a primitive, there is nothing to do, skip it.
        if (pb_type->is_primitive()) {
            continue;
        }

        // The one-hot pb type must have more than 1 mode.
        if (pb_type->num_modes > 1) {
            bool is_one_hot = true;
            std::set<LogicalModelId> contained_models;
            // Check if every pb_type in each mode is single and unique.
            for (int mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
                const t_mode& mode = pb_type->modes[mode_idx];
                // Check if the mode contains only a single pb_type.
                if (mode.num_pb_type_children != 1) {
                    is_one_hot = false;
                    break;
                }
                // Check if the mode contains a singular primitive.
                t_pb_type* mode_child_pb = &mode.pb_type_children[0];
                if (mode_child_pb->num_pb > 1 || !mode_child_pb->is_primitive()) {
                    is_one_hot = false;
                    break;
                }
                // Check if the primitive is unique (i.e. there is no other model
                // like it anywhere in the architecture).
                VTR_ASSERT_SAFE(mode_child_pb->is_primitive());
                LogicalModelId child_model = mode_child_pb->model_id;
                if (model_primitives[child_model].size() > 1) {
                    is_one_hot = false;
                    break;
                }
                // Keep track of the contained models.
                contained_models.insert(mode_child_pb->model_id);
            }

            // If this pb type is one-hot, store its info.
            if (is_one_hot) {
                OneHotPbType one_hot_pb_type_info;
                one_hot_pb_type_info.pb_type = pb_type;
                one_hot_pb_type_info.shared_models = std::move(contained_models);
                one_hot_pb_types.push_back(std::move(one_hot_pb_type_info));

                // Do not explore the children of this pb_type.
                continue;
            }
        }

        // Add the pb_type children of this pb_type to the queue.
        for (int mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
            const t_mode& mode = pb_type->modes[mode_idx];
            for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
                pb_type_queue.push(&mode.pb_type_children[pb_child_idx]);
            }
        }
    }

    // Log the one-hot pb types for debugging.
    if (log_verbosity >= 10) {
        VTR_LOG("One-Hot PB Types:\n");
        for (const OneHotPbType& one_hot_pb_type_info : one_hot_pb_types) {
            VTR_LOG("\t%s:\n", one_hot_pb_type_info.pb_type->name);
            for (LogicalModelId model_id : one_hot_pb_type_info.shared_models) {
                VTR_LOG("\t\t%s\n", models.model_name(model_id).c_str());
            }
        }
    }

    return one_hot_pb_types;
}

/**
 * @brief Initialize the dim manager such that every model in the architecture
 *        has a valid dimension in the primitive vector.
 */
static void initialize_dim_manager(PrimitiveDimManager& dim_manager,
                                   const LogicalModels& models,
                                   const std::vector<t_logical_block_type>& logical_block_types,
                                   const AtomNetlist& atom_netlist,
                                   int log_verbosity) {
    // Set the mapping between model IDs and Primitive Vector IDs

    // Find all of the pb types that act like a one-hot primitive.
    std::vector<OneHotPbType> one_hot_pb_types = find_one_hot_pb_types(logical_block_types,
                                                                       models,
                                                                       log_verbosity);

    // For each model, label them with their shared model ID if they are part
    // of a one-hot pb_type, -1 otherwise.
    vtr::vector<LogicalModelId, int> model_one_hot_id(models.all_models().size(), -1);
    for (size_t one_hot_id = 0; one_hot_id < one_hot_pb_types.size(); one_hot_id++) {
        for (LogicalModelId model_id : one_hot_pb_types[one_hot_id].shared_models) {
            model_one_hot_id[model_id] = one_hot_id;
        }
    }

    // Count the number of occurences of each model in the netlist.
    vtr::vector<LogicalModelId, unsigned> num_model_occurence(models.all_models().size(), 0);
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        LogicalModelId model_id = atom_netlist.block_model(blk_id);

        // If this model is not part of a shared dimension, just accumulate its
        // number of occurences.
        int one_hot_id = model_one_hot_id[model_id];
        if (one_hot_id == -1) {
            num_model_occurence[model_id]++;
            continue;
        }

        // If this model is part of a shared dimension, only accumulate into the
        // first shared model. This creates an accurate count of the number of
        // occurences of the overall shared dimension in this first model id.
        const OneHotPbType& one_hot_pb_type = one_hot_pb_types[one_hot_id];
        LogicalModelId first_model_id = *one_hot_pb_type.shared_models.begin();
        num_model_occurence[first_model_id]++;
    }

    // Create a list of models, sorted by their frequency in the netlist.
    // By sorting by frequency, we make the early dimensions more common,
    // which can reduce the overall size of the sparse vector.
    // NOTE: We use stable sort here to keep the order of models the same
    //       as what the user provided in the arch file in the event of a tie.
    std::vector<LogicalModelId> logical_models(models.all_models().begin(), models.all_models().end());
    std::stable_sort(logical_models.begin(), logical_models.end(), [&](LogicalModelId a, LogicalModelId b) {
        return num_model_occurence[a] > num_model_occurence[b];
    });

    // Create a primitive vector dim for each model that occurs in the netlist
    // and is not part of a shared dimension.
    for (LogicalModelId model_id : logical_models) {
        // If this model is not part of a shared dimension, create a single
        // dimension just for it.
        int one_hot_id = model_one_hot_id[model_id];
        if (one_hot_id == -1) {
            dim_manager.create_dim(model_id, models.model_name(model_id));
            continue;
        }

        // If this model is part of a shared dimension, check if this is the first
        // model ID in the list, if it isn't skip it. We only want to create one
        // dimension for these models.
        const OneHotPbType& one_hot_pb_type = one_hot_pb_types[one_hot_id];
        LogicalModelId first_model_id = *one_hot_pb_type.shared_models.begin();
        if (model_id != first_model_id)
            continue;

        // Create the shared dimension.

        // Create a unique name for the dim. This is used for debugging.
        std::string dim_name = one_hot_pb_type.pb_type->name;
        dim_name += "_ap_shared[";
        size_t index = 0;
        for (LogicalModelId shared_model_id : one_hot_pb_type.shared_models) {
            dim_name += models.model_name(shared_model_id);
            dim_name += "]";
            if (index != one_hot_pb_type.shared_models.size() - 1)
                dim_name += "[";
            index++;
        }

        // Create the new dimension.
        PrimitiveVectorDim new_dim = dim_manager.create_empty_dim(dim_name);

        // Add all of the models that are part of the one-hot to the new dimension.
        for (LogicalModelId shared_model_id : one_hot_pb_type.shared_models) {
            VTR_ASSERT(!dim_manager.get_model_dim(shared_model_id).is_valid());
            dim_manager.add_model_to_dim(shared_model_id, new_dim);
        }
    }
}

FlatPlacementMassCalculator::FlatPlacementMassCalculator(const APNetlist& ap_netlist,
                                                         const Prepacker& prepacker,
                                                         const AtomNetlist& atom_netlist,
                                                         const std::vector<t_logical_block_type>& logical_block_types,
                                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                                         const LogicalModels& models,
                                                         int log_verbosity)
    : physical_tile_type_capacity_(physical_tile_types.size())
    , logical_block_type_capacity_(logical_block_types.size())
    , block_mass_(ap_netlist.blocks().size())
    , log_verbosity_(log_verbosity) {

    // Initialize the mapping between model IDs and Primitive Vector dims
    initialize_dim_manager(primitive_dim_manager_,
                           models,
                           logical_block_types,
                           atom_netlist,
                           log_verbosity_);

    // Precompute the capacity of each logical block type.
    for (const t_logical_block_type& logical_block_type : logical_block_types) {
        logical_block_type_capacity_[logical_block_type.index] = calc_logical_block_type_capacity(logical_block_type, primitive_dim_manager_);
    }

    // Precompute the capacity of each physical tile type.
    for (const t_physical_tile_type& physical_tile_type : physical_tile_types) {
        physical_tile_type_capacity_[physical_tile_type.index] = calc_physical_tile_type_capacity(physical_tile_type, logical_block_type_capacity_);
    }

    // Precompute the mass of each block in the APNetlist
    VTR_LOGV(log_verbosity_ >= 10, "Pre-computing the block masses...\n");
    for (APBlockId ap_block_id : ap_netlist.blocks()) {
        block_mass_[ap_block_id] = calc_block_mass(ap_block_id,
                                                   primitive_dim_manager_,
                                                   ap_netlist,
                                                   prepacker,
                                                   atom_netlist);
    }
    VTR_LOGV(log_verbosity_ >= 10, "Finished pre-computing the block masses.\n");
}

void FlatPlacementMassCalculator::generate_mass_report(const APNetlist& ap_netlist) const {
    generate_ap_mass_report(logical_block_type_capacity_,
                            physical_tile_type_capacity_,
                            block_mass_,
                            primitive_dim_manager_,
                            ap_netlist);
}
