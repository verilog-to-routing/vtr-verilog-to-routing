/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Flat Placement Bin Abstraction
 *
 * This file declares a class which can bin AP Blocks spatially throughout the
 * FPGA.
 */

#pragma once

#include <unordered_set>
#include "ap_netlist.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

// The tag for the flat placement bin.
struct flat_placement_bin_tag {};

/**
 * @brief A unique ID to a flat placement bin.
 */
typedef vtr::StrongId<flat_placement_bin_tag, size_t> FlatPlacementBinId;

/**
 * @brief A container of bins which hold AP blocks and take up space on the FPGA.
 *
 * For flat placement, blocks may be placed anywhere on the FPGA grid. This
 * placement is continuous; however, in order to compute quantities like density
 * and legality, there needs to be a way to bin blocks together spatially.
 *
 * This class maintains bins which hold AP blocks and take up a rectangular
 * amount of space on the FPGA grid.
 *
 * This class is only a container; it leaves how the FPGA is split into bins to
 * higher level classes.
 */
class FlatPlacementBins {
public:
    // Iterator for the flat placement bin IDs
    typedef typename vtr::vector_map<FlatPlacementBinId, FlatPlacementBinId>::const_iterator bin_iterator;

    // Range for the flat placement bin IDs
    typedef typename vtr::Range<bin_iterator> bin_range;

    FlatPlacementBins(const APNetlist& ap_netlist)
        : block_bin_(ap_netlist.blocks().size(), FlatPlacementBinId::INVALID()) {}

    /**
     * @brief Returns a range of all bins that have been created.
     */
    bin_range bins() const {
        return vtr::make_range(bin_ids_.begin(), bin_ids_.end());
    }

    /**
     * @brief Creates a bin which exists in the given bin_region.
     *
     *  @param bin_region
     *      The rectangular region of the FPGA device that this bin will
     *      represent.
     */
    inline FlatPlacementBinId create_bin(const vtr::Rect<double>& bin_region) {
        FlatPlacementBinId new_bin_id = FlatPlacementBinId(bin_ids_.size());
        bin_ids_.push_back(new_bin_id);
        bin_region_.push_back(bin_region);
        bin_contained_blocks_.resize(bin_contained_blocks_.size() + 1);
        return new_bin_id;
    }

    /**
     * @brief Add the given block to the given bin.
     */
    inline void add_block_to_bin(APBlockId blk_id, FlatPlacementBinId bin_id) {
        VTR_ASSERT(blk_id.is_valid());
        VTR_ASSERT(bin_id.is_valid());
        VTR_ASSERT(!block_bin_[blk_id].is_valid());
        bin_contained_blocks_[bin_id].insert(blk_id);
        block_bin_[blk_id] = bin_id;
    }

    /**
     * @brief Remove the given block from the given bin. The bin must contain
     *        this block.
     */
    inline void remove_block_from_bin(APBlockId blk_id, FlatPlacementBinId bin_id) {
        VTR_ASSERT(blk_id.is_valid());
        VTR_ASSERT(bin_id.is_valid());
        VTR_ASSERT(block_bin_[blk_id] == bin_id);
        bin_contained_blocks_[bin_id].erase(blk_id);
        block_bin_[blk_id] = FlatPlacementBinId::INVALID();
    }

    /**
     * @brief Get the blocks contained within the given bin.
     */
    inline const std::unordered_set<APBlockId>& bin_contained_blocks(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_contained_blocks_[bin_id];
    }

    /**
     * @brief Get the region of the FPGA that the given bin covers.
     */
    inline const vtr::Rect<double>& bin_region(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_region_[bin_id];;
    }

    /**
     * @brief Get the bin that contains the given AP block.
     */
    inline FlatPlacementBinId block_bin(APBlockId blk_id) const {
        VTR_ASSERT(blk_id.is_valid());
        return block_bin_[blk_id];
    }

    /**
     * @brief Remove all of the AP blocks from the given bin.
     */
    inline void remove_all_blocks_from_bin(FlatPlacementBinId bin_id) {
        VTR_ASSERT(bin_id.is_valid());
        // Invalidate the block bin lookup for the blocks in the bin.
        for (APBlockId blk_id : bin_contained_blocks_[bin_id]) {
            block_bin_[blk_id] = FlatPlacementBinId::INVALID();
        }
        // Remove all of the blocks from the bin.
        bin_contained_blocks_[bin_id].clear();
    }

    /**
     * @brief Verify the internal members of this class are consistent.
     */
    inline bool verify() const {
        // Ensure all bin IDs are valid and consistent.
        for (FlatPlacementBinId bin_id : bin_ids_) {
            if (!bin_id.is_valid()) {
                VTR_LOG("Bin Verify: Invalid bin ID in bins.\n");
                return false;
            }
            if (bin_ids_.count(bin_id) != 1) {
                VTR_LOG("Bin Verify: Found a bin ID not in the bin IDs array.\n");
                return false;
            }
            if (bin_ids_[bin_id] != bin_id) {
                VTR_LOG("Bin Verify: Bin ID found which is not consistent.\n");
                return false;
            }
        }

        // Ensure the data members of this class are all the correct size.
        size_t num_bins = bin_ids_.size();
        if (bin_contained_blocks_.size() != num_bins) {
            VTR_LOG("Bin Verify: bin_constained_blocks_ not the correct size.\n");
            return false;
        }
        if (bin_region_.size() != num_bins) {
            VTR_LOG("Bin Verify: bin_region_ not the correct size.\n");
            return false;
        }

        // Make sure that the bin_contained_blocks_ and the block_bin_ are
        // consistent.
        for (FlatPlacementBinId bin_id : bin_ids_) {
            for (APBlockId blk_id : bin_contained_blocks_[bin_id]) {
                if (block_bin_[blk_id] != bin_id) {
                    VTR_LOG("Bin Verify: Block is contained within a bin but does not agree.\n");
                    return false;
                }
            }
        }

        return true;
    }

private:
    /// @brief A vector of the Flat Placement Bin IDs. If any of them are invalid,
    ///        then that means that the bin has been destroyed.
    vtr::vector_map<FlatPlacementBinId, FlatPlacementBinId> bin_ids_;

    /// @brief The contained AP blocks of each bin.
    vtr::vector_map<FlatPlacementBinId, std::unordered_set<APBlockId>> bin_contained_blocks_;

    /// @brief The bin that contains each AP block.
    vtr::vector<APBlockId, FlatPlacementBinId> block_bin_;

    /// @brief The region that each bin represents on the FPGA grid.
    // TODO: For 3D FPGAs, this should be a 3D rectangle.
    vtr::vector_map<FlatPlacementBinId, vtr::Rect<double>> bin_region_;
};

