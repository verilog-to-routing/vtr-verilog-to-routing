/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Implementation of the density manager class.
 */

#include "flat_placement_density_manager.h"
#include <tuple>
#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "atom_netlist.h"
#include "flat_placement_bins.h"
#include "flat_placement_mass_calculator.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "prepack.h"
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

FlatPlacementDensityManager::FlatPlacementDensityManager(
                const APNetlist & ap_netlist,
                const Prepacker& prepacker,
                const AtomNetlist& atom_netlist,
                const DeviceGrid& device_grid,
                const std::vector<t_logical_block_type>& logical_block_types,
                const std::vector<t_physical_tile_type>& physical_tile_types,
                int log_verbosity)
                    : ap_netlist_(ap_netlist)
                    , bins_(ap_netlist)
                    , mass_calculator_(ap_netlist, prepacker, atom_netlist, logical_block_types, physical_tile_types, log_verbosity)
                    , log_verbosity_(log_verbosity) {
    // Initialize the bin spatial lookup object.
    size_t num_layers, width, height;
    std::tie(num_layers, width, height) = device_grid.dim_sizes();
    bin_spatial_lookup_.resize({num_layers, width, height});

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
                vtr::Rect<double> new_bin_region(vtr::Point<double>(x, y),
                                                 vtr::Point<double>(x + tw,
                                                                    y + th));
                FlatPlacementBinId new_bin_id = bins_.create_bin(new_bin_region);

                // Add the bin to the spatial lookup
                bin_spatial_lookup_[layer][x][y] = new_bin_id;

                // Store the index of the physical tile type into a map to be
                // used to compute the capacity.
                bin_phy_tile_type_idx.insert(new_bin_id, tile_type->index);
            }
        }
    }

    // Initialize the bin capacities to the mass capacity of the physical tile
    // this bin represents.
    bin_capacity_.resize(bins_.bins().size());
    for (FlatPlacementBinId bin_id : bins_.bins()) {
        size_t physical_tile_type_index = bin_phy_tile_type_idx[bin_id];
        bin_capacity_[bin_id] = mass_calculator_.get_physical_tile_type_capacity(physical_tile_type_index);
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
    // TODO: Maybe import the fixed block locations in the constructor and then
    //       only import the moveable block locations.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        FlatPlacementBinId bin_id = get_bin(p_placement.block_x_locs[blk_id],
                                            p_placement.block_y_locs[blk_id],
                                            p_placement.block_layer_nums[blk_id]);
        insert_block_into_bin(blk_id, bin_id);
    }
}

vtr::Point<double> FlatPlacementDensityManager::get_block_location_in_bin(
                                                    APBlockId blk_id,
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
        bin_utilization_[bin_id] = PrimitiveVector();
        bin_overfill_[bin_id] = calc_bin_overfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
        bin_underfill_[bin_id] = calc_bin_underfill(bin_utilization_[bin_id], bin_capacity_[bin_id]);
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

