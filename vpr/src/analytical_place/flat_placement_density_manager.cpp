/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Implementation of the density manager class.
 */

#include "flat_placement_density_manager.h"
#include <tuple>
#include <unordered_map>
#include "ap_argparse_utils.h"
#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "atom_netlist.h"
#include "flat_placement_bins.h"
#include "flat_placement_mass_calculator.h"
#include "logic_types.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

/**
 * @brief Calculates how over-capacity the given utilization vector is.
 */
static PrimitiveVector calc_bin_overfill(const PrimitiveVector& bin_utilization,
                                         const PrimitiveVector& bin_capacity) {
    PrimitiveVector overfill = bin_utilization - bin_capacity;
    overfill.relu();
    VTR_ASSERT_DEBUG(overfill.is_non_negative());
    return overfill;
}

/**
 * @brief Calculates how under-capacity the given utilization vector is.
 */
static PrimitiveVector calc_bin_underfill(const PrimitiveVector& bin_utilization,
                                          const PrimitiveVector& bin_capacity) {
    PrimitiveVector underfill = bin_capacity - bin_utilization;
    underfill.relu();
    VTR_ASSERT_DEBUG(underfill.is_non_negative());
    return underfill;
}

/**
 * @brief Get the physical type target densities given the user arguments.
 *
 * This will automatically select good target densisities, but will allow the
 * user to override these values from the command line.
 *
 *  @param target_density_arg_strs
 *      The command-line arguments provided by the user.
 *  @param physical_tile_types
 *      A vector of all physical tile types in the architecture.
 */
static std::vector<float> get_physical_type_target_densities(const std::vector<std::string>& target_density_arg_strs,
                                                             const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Get the target densisty of each physical block type.
    // TODO: Create auto feature to automatically select target densities based
    //       on properties of the architecture. Need to sweep to find reasonable
    //       values.
    std::vector<float> phy_ty_target_density(physical_tile_types.size(), 1.0f);

    // Set to auto if no user args are provided.
    if (target_density_arg_strs.size() == 0)
        return phy_ty_target_density;
    if (target_density_arg_strs.size() == 1 && target_density_arg_strs[0] == "auto")
        return phy_ty_target_density;

    // Parse the user args. The physical type names are expected to be used as keys.
    std::vector<std::string> phy_ty_names;
    phy_ty_names.reserve(physical_tile_types.size());
    std::unordered_map<std::string, int> phy_ty_name_to_index;
    for (const t_physical_tile_type& phy_ty : physical_tile_types) {
        phy_ty_names.push_back(phy_ty.name);
        phy_ty_name_to_index[phy_ty.name] = phy_ty.index;
    }
    auto phy_ty_name_to_tar_density = key_to_float_argument_parser(target_density_arg_strs,
                                                                   phy_ty_names,
                                                                   1);

    // Update the target densities based on the user args.
    for (const auto& phy_ty_name_to_density_pair : phy_ty_name_to_tar_density) {
        const std::string& phy_ty_name = phy_ty_name_to_density_pair.first;
        VTR_ASSERT(phy_ty_name_to_density_pair.second.size() == 1);
        float target_density = phy_ty_name_to_density_pair.second[0];
        if (target_density < 0.0f) {
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Cannot have negative target density");
        }

        int phy_ty_index = phy_ty_name_to_index[phy_ty_name];
        phy_ty_target_density[phy_ty_index] = target_density;
    }

    return phy_ty_target_density;
}

FlatPlacementDensityManager::FlatPlacementDensityManager(const APNetlist& ap_netlist,
                                                         const Prepacker& prepacker,
                                                         const AtomNetlist& atom_netlist,
                                                         const DeviceGrid& device_grid,
                                                         const std::vector<t_logical_block_type>& logical_block_types,
                                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                                         const LogicalModels& models,
                                                         const std::vector<std::string>& target_density_arg_strs,
                                                         int log_verbosity)
    : ap_netlist_(ap_netlist)
    , bins_(ap_netlist)
    , mass_calculator_(ap_netlist, prepacker, atom_netlist, logical_block_types, physical_tile_types, models, log_verbosity)
    , log_verbosity_(log_verbosity) {
    // Initialize the bin spatial lookup object.
    size_t num_layers, width, height;
    std::tie(num_layers, width, height) = device_grid.dim_sizes();
    bin_spatial_lookup_.resize({num_layers, width, height});

    // Get the target densisty of each physical block type.
    std::vector<float> phy_ty_target_densities = get_physical_type_target_densities(target_density_arg_strs,
                                                                                    physical_tile_types);
    VTR_LOG("Partial legalizer is using target densities:");
    for (const t_physical_tile_type& phy_ty : physical_tile_types) {
        VTR_LOG(" %s:%.1f", phy_ty.name.c_str(), phy_ty_target_densities[phy_ty.index]);
    }
    VTR_LOG("\n");

    // Create a bin for each tile. This will create one bin for each root tile
    // location.
    vtr::vector_map<FlatPlacementBinId, size_t> bin_phy_tile_type_idx;
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                // Only create bins for root tile locations.
                auto tile_loc = t_physical_tile_loc(x, y, layer);
                int w_offset = device_grid.get_width_offset(tile_loc);
                int h_offset = device_grid.get_height_offset(tile_loc);
                if (w_offset != 0 || h_offset != 0) {
                    // If this is not a root tile location, set the spatial bin
                    // lookup to point to the root tile location's bin.
                    FlatPlacementBinId root_bin_id = bin_spatial_lookup_[layer][x - w_offset][y - h_offset];
                    bin_spatial_lookup_[layer][x][y] = root_bin_id;
                    continue;
                }

                // Create a bin for this tile.
                auto tile_type = device_grid.get_physical_type(tile_loc);
                int tw = tile_type->width;
                int th = tile_type->height;
                VTR_ASSERT_SAFE(tw != 0 && th != 0);
                vtr::Rect<double> new_bin_region(vtr::Point<double>(x, y),
                                                 vtr::Point<double>(x + tw,
                                                                    y + th));
                FlatPlacementBinId new_bin_id = bins_.create_bin(new_bin_region);

                // Add the bin to the spatial lookup
                bin_spatial_lookup_[layer][x][y] = new_bin_id;

                // Store the index of the physical tile type into a map to be
                // used to compute the capacity.
                bin_phy_tile_type_idx.insert(new_bin_id, tile_type->index);

                // Set the target density for this bin based on the physical
                // tile type..
                float target_density = phy_ty_target_densities[tile_type->index];
                bin_target_density_.push_back(target_density);
                VTR_ASSERT(bin_target_density_[new_bin_id] = target_density);
            }
        }
    }

    // Get the used primitive dims. This is used to ignore the unused dims during
    // partial legalization.
    const PrimitiveDimManager& dim_manager = mass_calculator_.get_dim_manager();
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        LogicalModelId model_id = atom_netlist.block_model(blk_id);
        PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
        used_dims_mask_.set_dim_val(dim, 1);
    }

    // Initialize the bin capacities to the mass capacity of the physical tile
    // this bin represents.
    bin_capacity_.resize(bins_.bins().size());
    for (FlatPlacementBinId bin_id : bins_.bins()) {
        size_t physical_tile_type_index = bin_phy_tile_type_idx[bin_id];
        bin_capacity_[bin_id] = mass_calculator_.get_physical_tile_type_capacity(physical_tile_type_index);
        // Only allocate capacity to dims which are actually used. This prevents
        // the capacity vectors from getting too large, saving run time.
        bin_capacity_[bin_id].project(used_dims_mask_);
    }

    // Initialize the bin utilizations to be zero (there is nothing in the bin
    // currently).
    bin_utilization_.resize(bins_.bins().size(), PrimitiveVector());

    // Initialize the bin underfill and overfill.
    bin_underfill_.resize(bins_.bins().size());
    bin_overfill_.resize(bins_.bins().size());
    for (FlatPlacementBinId bin_id : bins_.bins()) {
        bin_underfill_[bin_id] = calc_bin_underfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
        bin_overfill_[bin_id] = calc_bin_overfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
    }

    // Note: The overfilled_bins_ are left empty. All bins are empty, therefore
    //       no bin is overfilled.
}

FlatPlacementBinId FlatPlacementDensityManager::get_bin(double x, double y, double layer) const {
    size_t layer_pos = std::floor(layer);
    size_t x_pos = std::floor(x);
    size_t y_pos = std::floor(y);
    VTR_ASSERT(layer_pos < bin_spatial_lookup_.dim_size(0));
    VTR_ASSERT(x_pos < bin_spatial_lookup_.dim_size(1));
    VTR_ASSERT(y_pos < bin_spatial_lookup_.dim_size(2));
    return bin_spatial_lookup_[layer_pos][x][y];
}

void FlatPlacementDensityManager::insert_block_into_bin(APBlockId blk_id,
                                                        FlatPlacementBinId bin_id) {
    VTR_ASSERT(blk_id.is_valid());
    VTR_ASSERT(bin_id.is_valid());
    // Add the block to the bin.
    bins_.add_block_to_bin(blk_id, bin_id);
    // Update the bin utilization.
    bin_utilization_[bin_id] += mass_calculator_.get_block_mass(blk_id);
    // Update the bin overfill and underfill
    bin_overfill_[bin_id] = calc_bin_overfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
    bin_underfill_[bin_id] = calc_bin_underfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
    // Insert the bin into the overfilled bin set if it is overfilled.
    if (bin_is_overfilled(bin_id))
        overfilled_bins_.insert(bin_id);
}

void FlatPlacementDensityManager::remove_block_from_bin(APBlockId blk_id,
                                                        FlatPlacementBinId bin_id) {
    VTR_ASSERT(blk_id.is_valid());
    VTR_ASSERT(bin_id.is_valid());
    // Remove the block from the bin.
    bins_.remove_block_from_bin(blk_id, bin_id);
    // Update the bin utilization.
    bin_utilization_[bin_id] -= mass_calculator_.get_block_mass(blk_id);
    // Update the bin overfill and underfill.
    bin_overfill_[bin_id] = calc_bin_overfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
    bin_underfill_[bin_id] = calc_bin_underfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
    // Remove from overfilled bins set if it is not overfilled.
    if (!bin_is_overfilled(bin_id))
        overfilled_bins_.erase(bin_id);
}

void FlatPlacementDensityManager::import_placement_into_bins(const PartialPlacement& p_placement) {
    // Empty the bins such that all blocks are no longer within the bins.
    empty_bins();

    // Insert each block in the netlist into their bin based on their placement.
    // TODO: Maybe import the fixed block locations in the constructor and then
    //       only import the moveable block locations.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        FlatPlacementBinId bin_id = get_bin(p_placement.block_x_locs[blk_id],
                                            p_placement.block_y_locs[blk_id],
                                            p_placement.block_layer_nums[blk_id]);
        insert_block_into_bin(blk_id, bin_id);
    }
}

vtr::Point<double> FlatPlacementDensityManager::get_block_location_in_bin(APBlockId blk_id,
                                                                          const vtr::Rect<double>& bin_region,
                                                                          const PartialPlacement& p_placement) const {
    // A block should not be placed on the edges of the region
    // of a bin; however they can be infinitely close to these sides. It is
    // arbitrary how close to the edge we place the blocks; opted to place them
    // as close as possible.
    double epsilon = 0.0001;
    double x = std::clamp<double>(p_placement.block_x_locs[blk_id],
                                  bin_region.bottom_left().x() + epsilon,
                                  bin_region.top_right().x() - epsilon);
    double y = std::clamp<double>(p_placement.block_y_locs[blk_id],
                                  bin_region.bottom_left().y() + epsilon,
                                  bin_region.top_right().y() - epsilon);
    return vtr::Point<double>(x, y);
}

void FlatPlacementDensityManager::export_placement_from_bins(PartialPlacement& p_placement) const {
    // Updates the partial placement with the location of the blocks in the bin
    // by moving the blocks to the point with the bin closest to where they
    // were originally.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        // Only the moveable block locations should be exported.
        if (ap_netlist_.block_mobility(blk_id) == APBlockMobility::FIXED)
            continue;
        // Project the coordinate of the block in the partial placement to the
        // closest point in the bin.
        FlatPlacementBinId blk_bin_id = bins_.block_bin(blk_id);
        VTR_ASSERT_DEBUG(blk_bin_id.is_valid());
        vtr::Point<double> new_blk_pos = get_block_location_in_bin(blk_id,
                                                                   bins_.bin_region(blk_bin_id),
                                                                   p_placement);
        p_placement.block_x_locs[blk_id] = new_blk_pos.x();
        p_placement.block_y_locs[blk_id] = new_blk_pos.y();
        // NOTE: This code currently does not support 3D FPGAs.
        VTR_ASSERT(std::floor(p_placement.block_layer_nums[blk_id]) == 0.0);
    }
}

void FlatPlacementDensityManager::empty_bins() {
    // Reset all of the bins and their utilizations.
    for (FlatPlacementBinId bin_id : bins_.bins()) {
        bins_.remove_all_blocks_from_bin(bin_id);
        bin_utilization_[bin_id].clear();
        bin_overfill_[bin_id].clear();
        bin_underfill_[bin_id] = bin_capacity_[bin_id];
    }
    // Once all the bins are reset, all bins should be empty; therefore no bins
    // are overfilled.
    overfilled_bins_.clear();
}

bool FlatPlacementDensityManager::verify() const {
    // Verify the bins for consistency.
    if (!bins_.verify()) {
        VTR_LOG("Bins failed to verify.\n");
        return false;
    }
    // Make sure that every block has a bin.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        if (!bins_.block_bin(blk_id).is_valid()) {
            VTR_LOG("Bin Verify: Found a block that is not in a bin.\n");
            return false;
        }
    }
    // Make sure that every bin has the correct utilization, supply, and demand.
    for (FlatPlacementBinId bin_id : bins_.bins()) {
        PrimitiveVector calc_utilization;
        for (APBlockId blk_id : bins_.bin_contained_blocks(bin_id)) {
            calc_utilization += mass_calculator_.get_block_mass(blk_id);
        }
        if (bin_utilization_[bin_id] != calc_utilization) {
            VTR_LOG("Bin Verify: Found a bin with incorrect utilization.\n");
            return false;
        }
        PrimitiveVector calc_overfill = bin_utilization_[bin_id] - bin_capacity_[bin_id];
        calc_overfill.relu();
        if (bin_overfill_[bin_id] != calc_overfill) {
            VTR_LOG("Bin Verify: Found a bin with incorrect overfill.\n");
            return false;
        }
        PrimitiveVector calc_underfill = bin_capacity_[bin_id] - bin_utilization_[bin_id];
        calc_underfill.relu();
        if (bin_underfill_[bin_id] != calc_underfill) {
            VTR_LOG("Bin Verify: Found a bin with incorrect underfill.\n");
            return false;
        }
        if (!bin_overfill_[bin_id].is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative overfill.\n");
            return false;
        }
        if (!bin_underfill_[bin_id].is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative underfill.\n");
            return false;
        }
        if (!bin_capacity_[bin_id].is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative capacity.\n");
            return false;
        }
        if (!bin_utilization_[bin_id].is_non_negative()) {
            VTR_LOG("Bin Verify: Found a bin with a negative utilization.\n");
            return false;
        }
    }
    // Make sure all overfilled bins are actually overfilled.
    // TODO: Need to make sure that all non-overfilled bins are actually not
    //       overfilled.
    for (FlatPlacementBinId bin_id : overfilled_bins_) {
        if (bin_overfill_[bin_id].is_zero()) {
            VTR_LOG("Bin Verify: Found an overfilled bin that was not overfilled.\n");
            return false;
        }
    }
    // If all above passed, then the bins are valid.
    return true;
}

void FlatPlacementDensityManager::print_bin_grid() const {
    size_t width = bin_spatial_lookup_.dim_size(1);
    size_t height = bin_spatial_lookup_.dim_size(2);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            FlatPlacementBinId bin_id = get_bin(x, y, 0.0);
            VTR_LOG("%3zu ",
                    bins_.bin_contained_blocks(bin_id).size());
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

void FlatPlacementDensityManager::generate_mass_report() const {
    mass_calculator_.generate_mass_report(ap_netlist_);
}
