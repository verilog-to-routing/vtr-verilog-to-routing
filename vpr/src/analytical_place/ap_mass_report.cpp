/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @brief   Implementation of the AP mass report generator.
 */

#include "ap_mass_report.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>
#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "globals.h"
#include "logic_types.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "primitive_vector.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_time.h"
#include "vtr_vector.h"

namespace {

/**
 * @brief A node in a printing tree.
 */
struct PrintingTreeNode {
    /// @brief The name of this node. This will be printed when the tree is printed.
    std::string name;
    /// @brief The children of this node in the tree.
    std::vector<PrintingTreeNode> children;
};

/**
 * @brief A printing tree. This tree contains basic information which will be
 *        used to print data in a good-looking form.
 */
struct PrintingTree {
    /// @brief The root node of the tree.
    PrintingTreeNode root;
};

/**
 * @brief Print the given tree node.
 *
 *  @param node
 *      The node to print.
 *  @param os
 *      The output file stream to print the node to.
 *  @param prefix
 *      The prefix that all children of this node will print before their names.
 */
void print_tree_node(const PrintingTreeNode& node,
                     std::ofstream& os,
                     const std::string& prefix) {
    // Print the name of the node here and start a new line.
    os << node.name << "\n";

    // Print the children of this node.
    size_t num_children = node.children.size();
    for (size_t child_idx = 0; child_idx < num_children; child_idx++) {
        if (child_idx != num_children - 1) {
            // If this is not the last child, print a vertical line which will
            // be connected by the next child. This will be printed just before
            // the node name of this child.
            os << prefix << "├── ";
            // Print the child node and update the prefix for any of its children.
            // This prefix will connect the lines in a good looking way.
            print_tree_node(node.children[child_idx], os, prefix + "│   ");
        } else {
            // If this is the last child, we print an L shape to signify that
            // there are no further children.
            os << prefix << "└── ";
            // Print the child node, and set the prefix to basically just be
            // an indent.
            print_tree_node(node.children[child_idx], os, prefix + "    ");
        }
    }
}

/**
 * @brief Helper function to print an entire printing tree.
 *
 * This method begins the recursion of printing using the proper prefix.
 *
 *  @param tree
 *      The tree to print.
 *  @param os
 *      The output file stream to print to.
 */
void print_tree(const PrintingTree& tree, std::ofstream& os) {
    print_tree_node(tree.root, os, "");
}

/**
 * @brief Generate the printing tree node for the given pb type.
 *
 *  @param pb_type
 *      The pb type to generate the tree node for.
 *  @param models
 *      The logical models in the architecture.
 */
PrintingTreeNode gen_pb_printing_tree_node(const t_pb_type* pb_type, const LogicalModels& models);

/**
 * @brief Generate the printing tree node for the given mode.
 *
 *  @param mode
 *      The mode to generate the tree node for.
 *  @param models
 *      The logical models in the architecture.
 */
PrintingTreeNode gen_mode_printing_tree_node(const t_mode& mode, const LogicalModels& models) {
    // Create the node with the mode name.
    PrintingTreeNode mode_node;
    mode_node.name = std::string(mode.name) + " (mode)";

    // Create the children. There will be one child for each pb in this mode.
    mode_node.children.reserve(mode.num_pb_type_children);
    for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
        // Generate the child pb's node.
        const t_pb_type& pb_type = mode.pb_type_children[pb_child_idx];
        PrintingTreeNode pb_node = gen_pb_printing_tree_node(&pb_type, models);
        // Insert the node into the list of children.
        mode_node.children.emplace_back(std::move(pb_node));
    }

    return mode_node;
}

PrintingTreeNode gen_pb_printing_tree_node(const t_pb_type* pb_type, const LogicalModels& models) {
    // Create the node with the pb name and the number of pbs.
    PrintingTreeNode pb_node;
    pb_node.name = std::string(pb_type->name) + " [" + std::to_string(pb_type->num_pb) + "]";
    if (!pb_type->is_primitive()) {
        pb_node.name += " (pb_type)";
    } else {
        // If this pb type is a primitive, print the name of the model as well.
        LogicalModelId model_id = pb_type->model_id;
        std::string model_name = models.model_name(model_id);
        pb_node.name += " (primitive pb_type | model: " + model_name + ")";
    }

    // Create the children. There will be one child for each mode of this pb.
    pb_node.children.reserve(pb_type->num_modes);
    for (int mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
        // Generate the child mode's node.
        const t_mode& mode = pb_type->modes[mode_idx];
        PrintingTreeNode mode_node = gen_mode_printing_tree_node(mode, models);
        // Insert the node into the list of children.
        pb_node.children.emplace_back(std::move(mode_node));
    }

    return pb_node;
}

/**
 * @brief Print the logical block type graph.
 *
 * This graph is a forest, as such we can print the forest in a pretty way in
 * a text file.
 */
void print_logical_block_graph(std::ofstream& os,
                               const std::vector<t_logical_block_type>& logical_block_types,
                               const LogicalModels& models) {
    // Generate the internal complex block trees.
    // This is a DFS traversal of each complex block root in the forest.
    os << "=================================================================\n";
    os << "Logical (Complex) Block Graph:\n";
    os << "=================================================================\n";
    os << "\n";

    for (const t_logical_block_type& block_type : logical_block_types) {
        // Set the root of the complex block tree to be the name of this logical
        // block type.
        PrintingTree complex_block_tree;
        complex_block_tree.root.name = block_type.name + " (root logical block)";

        // If this block has a pb type, generate the pb printing node (including
        // its children) and add to the list of children.
        if (block_type.pb_type != nullptr) {
            PrintingTreeNode root_pb_node = gen_pb_printing_tree_node(block_type.pb_type, models);
            complex_block_tree.root.children.emplace_back(std::move(root_pb_node));
        }

        // Print the tree to the file.
        print_tree(complex_block_tree, os);
        os << "\n";
    }
}

/**
 * @brief Print information on the physical tiles and how they relate to the
 *        logical block types.
 */
void print_physical_tiles(std::ofstream& os,
                          const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Generate the physical tile relationships with the complex blocks.
    os << "=================================================================\n";
    os << "Physical Tile Graph:\n";
    os << "=================================================================\n";
    os << "\n";

    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        // Create a printing tree with a root name of the tile.
        PrintingTree tile_tree;
        tile_tree.root.name = tile_type.name + " (tile)";

        // Create a child for each sub tile.
        tile_tree.root.children.reserve(tile_type.sub_tiles.size());
        for (const auto& sub_tile : tile_type.sub_tiles) {
            // Create a sub tile node with the name of the sub tile and the capacity.
            PrintingTreeNode sub_tile_node;
            sub_tile_node.name = sub_tile.name + " [" + std::to_string(sub_tile.capacity.total()) + "] (sub-tile)";

            // Create a child for each equivalent site.
            sub_tile_node.children.reserve(sub_tile.equivalent_sites.size());
            for (const auto& block_type : sub_tile.equivalent_sites) {
                PrintingTreeNode block_type_node;
                block_type_node.name = block_type->name + " (equiv-site)";

                // Add this equivalent site to the parent sub-tile.
                sub_tile_node.children.push_back(std::move(block_type_node));
            }

            // Add this sub-tile to the tile node.
            tile_tree.root.children.push_back(std::move(sub_tile_node));
        }

        // Print the tree for this tile.
        print_tree(tile_tree, os);
        os << "\n";
    }
}

/**
 * @brief Prints all of the non-zero dimensions of the given primitive vector.
 *
 *  @param os
 *      The output file stream to print the primtive vector to.
 *  @param primitive_vec
 *      The primitive vector to print.
 *  @param models
 *      All models in the architecture (used to get the name of the model.
 *  @param prefix
 *      The prefix to print ahead of each entry in the primitive vector print.
 */
void print_primitive_vector(std::ofstream& os,
                            const PrimitiveVector& primitive_vec,
                            const LogicalModels& models,
                            const std::string& prefix) {
    std::vector<int> contained_models = primitive_vec.get_non_zero_dims();

    // Get the max name length of the contained models for pretty printing.
    size_t max_model_name_len = 0;
    for (int model_id : contained_models) {
        std::string model_name = models.model_name((LogicalModelId)model_id);
        max_model_name_len = std::max(max_model_name_len,
                                      model_name.size());
    }

    // Print the capacity of each model.
    for (int model_id : contained_models) {
        std::string model_name = models.model_name((LogicalModelId)model_id);
        os << prefix << std::setw(max_model_name_len) << model_name;
        os << ": " << primitive_vec.get_dim_val(model_id);
        os << "\n";
    }
}

/**
 * @brief Print information of the capacity of each logical block type.
 */
void print_logical_block_type_capacities(std::ofstream& os,
                                         const std::vector<t_logical_block_type>& logical_block_types,
                                         const std::vector<PrimitiveVector>& logical_block_type_capacities,
                                         const LogicalModels& models) {
    os << "=================================================================\n";
    os << "Logical Block Type Capacities:\n";
    os << "=================================================================\n";
    os << "\n";

    // For each logical block type, print the capacity (in primitive-vector
    // space).
    for (const t_logical_block_type& block_type : logical_block_types) {
        // Print the name of the block.
        os << block_type.name << ":\n";

        // Print the capacity of the logical block type.
        const PrimitiveVector& cap = logical_block_type_capacities[block_type.index];
        print_primitive_vector(os, cap, models, "\t");
        os << "\n";
    }
}

/**
 * @brief Print information of the capacity of each physical tile on the device.
 */
void print_physical_tile_type_capacities(std::ofstream& os,
                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                         const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                                         const LogicalModels& models) {
    os << "=================================================================\n";
    os << "Physical Tile Type Capacities:\n";
    os << "=================================================================\n";
    os << "\n";

    // For each physical tile type, print the capacity (in primitive-vector
    // space).
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        // Print the name of the tile.
        os << tile_type.name << ":\n";

        // Print the capacity of the tile type.
        const PrimitiveVector& cap = physical_tile_type_capacities[tile_type.index];
        print_primitive_vector(os, cap, models, "\t");
        os << "\n";
    }
}

/**
 * @brief Helper method for computing the total primitive vector mass of the
 *        given AP netlist.
 */
PrimitiveVector calc_total_netlist_mass(const APNetlist& ap_netlist,
                                        const vtr::vector<APBlockId, PrimitiveVector>& block_mass) {
    PrimitiveVector total_netlist_mass;
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        total_netlist_mass += block_mass[ap_blk_id];
    }

    return total_netlist_mass;
}

/**
 * @brief Print information on how much mass the AP netlist uses relative to
 *        the overall device.
 */
void print_netlist_mass_utilization(std::ofstream& os,
                                    const APNetlist& ap_netlist,
                                    const vtr::vector<APBlockId, PrimitiveVector>& block_mass,
                                    const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                                    const DeviceGrid& device_grid,
                                    const LogicalModels& models) {
    os << "=================================================================\n";
    os << "Netlist Mass Utilization:\n";
    os << "=================================================================\n";
    os << "\n";

    // Get the capacity of all the physical tiles in the grid.
    PrimitiveVector total_grid_capacity;
    size_t grid_width = device_grid.width();
    size_t grid_height = device_grid.height();
    size_t layer = 0;
    for (size_t x = 0; x < grid_width; x++) {
        for (size_t y = 0; y < grid_height; y++) {
            t_physical_tile_loc tile_loc(x, y, layer);
            if (device_grid.get_width_offset(tile_loc) != 0 || device_grid.get_height_offset(tile_loc) != 0)
                continue;

            auto tile_type = device_grid.get_physical_type(tile_loc);
            total_grid_capacity += physical_tile_type_capacities[tile_type->index];
        }
    }

    // Get the mass of all blocks in the netlist.
    PrimitiveVector total_netlist_mass = calc_total_netlist_mass(ap_netlist, block_mass);

    PrimitiveVector netlist_per_model_counts;
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        for (int dim : block_mass[ap_blk_id].get_non_zero_dims()) {
            netlist_per_model_counts.add_val_to_dim(1, dim);
        }
    }

    // Get the max string length of any model to make the printing prettier.
    size_t max_model_name_len = 0;
    for (LogicalModelId model_id : models.all_models()) {
        max_model_name_len = std::max(max_model_name_len, std::strlen(models.model_name(model_id).c_str()));
    }

    // Print a breakdown of the mass utilization of the netlist.
    os << std::setw(max_model_name_len) << "Model";
    os << ": Total Netlist Mass | Total Grid Mass | Mass Utilization\n";
    for (LogicalModelId model_id : models.all_models()) {
        float model_netlist_mass = total_netlist_mass.get_dim_val((size_t)model_id);
        float model_grid_capacity = total_grid_capacity.get_dim_val((size_t)model_id);
        os << std::setw(max_model_name_len) << models.model_name(model_id);
        os << ": " << std::setw(18) << model_netlist_mass;
        os << " | " << std::setw(15) << model_grid_capacity;
        os << " | " << std::setw(16) << model_netlist_mass / model_grid_capacity;
        os << "\n";
    }
    os << "\n";

    os << std::setw(max_model_name_len) << "Model";
    os << ": Total Netlist Mass | Number of Blocks | Average Mass per Block\n";
    for (LogicalModelId model_id : models.all_models()) {
        float model_netlist_mass = total_netlist_mass.get_dim_val((size_t)model_id);
        float num_blocks = netlist_per_model_counts.get_dim_val((size_t)model_id);
        float average_mass_per_block = 0.0f;
        if (num_blocks > 0.0f) {
            average_mass_per_block = model_netlist_mass / num_blocks;
        }
        os << std::setw(max_model_name_len) << models.model_name(model_id);
        os << ": " << std::setw(18) << model_netlist_mass;
        os << " | " << std::setw(16) << num_blocks;
        os << " | " << std::setw(22) << average_mass_per_block;
        os << "\n";
    }
    os << "\n";
}

/**
 * @brief Uses the mass of the netlist and the mass of the logical block types
 *        to predict the device utilization for the given device grid.
 */
void print_expected_device_utilization(std::ofstream& os,
                                       const APNetlist& ap_netlist,
                                       const vtr::vector<APBlockId, PrimitiveVector>& block_mass,
                                       const std::vector<t_logical_block_type>& logical_block_types,
                                       const std::vector<PrimitiveVector>& logical_block_type_capacities,
                                       const DeviceGrid& device_grid) {
    os << "=================================================================\n";
    os << "Expected Device Utilization:\n";
    os << "=================================================================\n";
    os << "\n";

    // Get the total mass of the netlist.
    PrimitiveVector total_netlist_mass = calc_total_netlist_mass(ap_netlist, block_mass);

    // Get the expected number of instances of each logical block type.
    std::vector<unsigned> num_type_instances(logical_block_types.size(), 0);
    for (const t_logical_block_type& block_type : logical_block_types) {
        // For each logical block type, estimate the number of blocks of that type
        // We can estimate this value as being the maximum required number of
        // instances to support the most utilized model.
        const PrimitiveVector block_type_cap = logical_block_type_capacities[block_type.index];
        unsigned num_blocks_of_this_type = 0;
        for (int model_id : block_type_cap.get_non_zero_dims()) {
            float netlist_model_mass = total_netlist_mass.get_dim_val(model_id);
            float model_mass_per_block = block_type_cap.get_dim_val(model_id);
            unsigned num_blocks_needed_for_model = std::ceil(netlist_model_mass / model_mass_per_block);
            num_blocks_of_this_type = std::max(num_blocks_of_this_type, num_blocks_needed_for_model);
        }

        num_type_instances[block_type.index] = num_blocks_of_this_type;
    }

    // Get the max logical block type name length for pretty printing.
    size_t max_logical_block_name_len = 0;
    for (const t_logical_block_type& block_type : logical_block_types) {
        max_logical_block_name_len = std::max(max_logical_block_name_len, block_type.name.size());
    }

    // Print the expected number of logical blocks and the expected block utilization.
    // Note: These may be innacurate if a model appears in multiple different
    //       logical blocks.
    // TODO: Investigate resolving this issue.
    os << "Expected number of logical blocks:\n";
    for (const t_logical_block_type& block_type : logical_block_types) {
        if (block_type.is_empty())
            continue;
        os << "\t" << std::setw(max_logical_block_name_len) << block_type.name;
        os << ": " << num_type_instances[block_type.index];
        os << "\n";
    }
    os << "\n";

    os << "Expected block utilization:\n";
    for (const t_logical_block_type& block_type : logical_block_types) {
        if (block_type.is_empty())
            continue;
        // Get the number of instances of this logical block in this device.
        size_t num_inst = device_grid.num_instances(pick_physical_type(&block_type), -1);

        // Estimate the utilization as being the expected number of instances
        // divided by the number of instances possible on the device.
        float util = 0.0f;
        if (num_inst > 0) {
            util = static_cast<float>(num_type_instances[block_type.index]);
            util /= static_cast<float>(num_inst);
        }
        os << "\t" << std::setw(max_logical_block_name_len) << block_type.name;
        os << ": " << util;
        os << "\n";
    }
    os << "\n";
}

} // namespace

void generate_ap_mass_report(const std::vector<PrimitiveVector>& logical_block_type_capacities,
                             const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                             const vtr::vector<APBlockId, PrimitiveVector>& block_mass,
                             const APNetlist& ap_netlist) {

    vtr::ScopedStartFinishTimer timer("Generating AP Mass Report");

    // Load device data which is used to calculate the data in the report.
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const std::vector<t_physical_tile_type>& physical_tile_types = device_ctx.physical_tile_types;
    const std::vector<t_logical_block_type>& logical_block_types = device_ctx.logical_block_types;
    const LogicalModels& models = device_ctx.arch->models;
    const DeviceGrid& device_grid = device_ctx.grid;

    // Open the AP mass report file as an output file stream.
    std::string mass_report_file_name = "ap_mass.rpt";
    std::ofstream os(mass_report_file_name);
    if (!os.is_open()) {
        VPR_FATAL_ERROR(VPR_ERROR_AP,
                        "Unable to open AP mass report file");
        return;
    }

    // Print the logical block graph.
    print_logical_block_graph(os, logical_block_types, models);

    // Print information on the physical tiles.
    print_physical_tiles(os, physical_tile_types);

    // TODO: Print a lookup between the model names and IDs

    // Print the computed capacities of each logical block type.
    print_logical_block_type_capacities(os, logical_block_types, logical_block_type_capacities, models);

    // Print the computed capacities of each physical tile type.
    print_physical_tile_type_capacities(os, physical_tile_types, physical_tile_type_capacities, models);

    // Print information on how much mass is utilized by the netlist relative
    // to the device.
    print_netlist_mass_utilization(os, ap_netlist, block_mass, physical_tile_type_capacities, device_grid, models);

    // Print the expected device utilization, given the mass of the netlist
    // and the capacity of the device grid.
    print_expected_device_utilization(os, ap_netlist, block_mass, logical_block_types, logical_block_type_capacities, device_grid);
}
