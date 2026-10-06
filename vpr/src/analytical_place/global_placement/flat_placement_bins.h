#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Flat Placement Bin Abstraction
 *
 * This file declares a class which can bin AP Blocks spatially throughout the
 * FPGA.
 */

#include "ap_netlist.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

/**
 * @brief A unique ID to a flat placement bin.
 */
typedef vtr::StrongId<struct flat_placement_bin_tag, size_t> FlatPlacementBinId;

/// @brief The position of an AP block within the contained blocks of its bin.
///        The slot of a block can change when another block leaves its bin.
typedef vtr::StrongId<struct bin_slot_tag, size_t> BinSlotId;

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
        : block_bin_(ap_netlist.blocks().size(), FlatPlacementBinId::INVALID())
        , block_bin_slot_(ap_netlist.blocks().size(), BinSlotId::INVALID()) {}

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
     *  @param bin_layer
     *      The layer that the bin is on. We currently assume that bins do not
     *      cross layers.
     */
    inline FlatPlacementBinId create_bin(const vtr::Rect<double>& bin_region,
                                         size_t bin_layer) {
        FlatPlacementBinId new_bin_id = FlatPlacementBinId(bin_ids_.size());
        bin_ids_.push_back(new_bin_id);
        bin_region_.push_back(bin_region);
        bin_layer_.push_back(bin_layer);
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
        vtr::vector<BinSlotId, APBlockId>& contained_blocks = bin_contained_blocks_[bin_id];
        block_bin_slot_[blk_id] = BinSlotId(contained_blocks.size());
        contained_blocks.push_back(blk_id);
        block_bin_[blk_id] = bin_id;
    }

    /**
     * @brief Remove the given block from the given bin. The bin must contain
     *        this block.
     *
     * The last block of the bin takes the slot of the removed block, so the
     * order of the remaining blocks in the bin changes.
     */
    inline void remove_block_from_bin(APBlockId blk_id, FlatPlacementBinId bin_id) {
        VTR_ASSERT(blk_id.is_valid());
        VTR_ASSERT(bin_id.is_valid());
        VTR_ASSERT(block_bin_[blk_id] == bin_id);
        vtr::vector<BinSlotId, APBlockId>& contained_blocks = bin_contained_blocks_[bin_id];
        BinSlotId blk_slot = block_bin_slot_[blk_id];
        VTR_ASSERT_SAFE(blk_slot.is_valid());
        VTR_ASSERT_SAFE(size_t(blk_slot) < contained_blocks.size());
        VTR_ASSERT_SAFE(contained_blocks[blk_slot] == blk_id);

        // Move the last block of the bin into the slot of the removed block.
        APBlockId last_blk_id = contained_blocks.back();
        contained_blocks[blk_slot] = last_blk_id;
        block_bin_slot_[last_blk_id] = blk_slot;

        // Drop the last slot, which is now a duplicate.
        contained_blocks.pop_back();
        block_bin_[blk_id] = FlatPlacementBinId::INVALID();
        block_bin_slot_[blk_id] = BinSlotId::INVALID();
    }

    /**
     * @brief Get the blocks contained within the given bin.
     *
     * Adding or removing a block from this bin invalidates iteration over the
     * returned vector.
     */
    inline const vtr::vector<BinSlotId, APBlockId>& bin_contained_blocks(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_contained_blocks_[bin_id];
    }

    /**
     * @brief Get the region of the FPGA that the given bin covers.
     */
    inline const vtr::Rect<double>& bin_region(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_region_[bin_id];
    }

    /**
     * @brief Get the layer of the FPGA that the given bin covers.
     */
    inline size_t bin_layer(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_layer_[bin_id];
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
        // Invalidate the block bin and slot lookups for the blocks in the bin.
        for (APBlockId blk_id : bin_contained_blocks_[bin_id]) {
            block_bin_[blk_id] = FlatPlacementBinId::INVALID();
            block_bin_slot_[blk_id] = BinSlotId::INVALID();
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
        if (bin_layer_.size() != num_bins) {
            VTR_LOG("Bin Verify: bin_layer_ not the correct size.\n");
            return false;
        }

        // Make sure that the bin_contained_blocks_, the block_bin_ and the
        // block_bin_slot_ are consistent.
        size_t num_contained_blocks = 0;
        for (FlatPlacementBinId bin_id : bin_ids_) {
            const vtr::vector<BinSlotId, APBlockId>& contained_blocks = bin_contained_blocks_[bin_id];
            num_contained_blocks += contained_blocks.size();
            for (BinSlotId slot : contained_blocks.keys()) {
                APBlockId blk_id = contained_blocks[slot];
                if (block_bin_[blk_id] != bin_id) {
                    VTR_LOG("Bin Verify: Block is contained within a bin but does not agree.\n");
                    return false;
                }
                if (block_bin_slot_[blk_id] != slot) {
                    VTR_LOG("Bin Verify: Block is not at its recorded slot within its bin.\n");
                    return false;
                }
            }
        }

        // Make sure that every block with a bin appears in exactly one bin.
        // The slot check above rules out a block appearing twice, so it is
        // enough to compare the counts.
        size_t num_blocks_with_bin = 0;
        for (APBlockId blk_id : block_bin_.keys()) {
            if (block_bin_[blk_id].is_valid()) {
                num_blocks_with_bin++;
            } else if (block_bin_slot_[blk_id].is_valid()) {
                VTR_LOG("Bin Verify: Block has a slot but is not in a bin.\n");
                return false;
            }
        }
        if (num_blocks_with_bin != num_contained_blocks) {
            VTR_LOG("Bin Verify: Block has a bin but is not contained within it.\n");
            return false;
        }

        return true;
    }

  private:
    /// @brief A vector of the Flat Placement Bin IDs. If any of them are invalid,
    ///        then that means that the bin has been destroyed.
    vtr::vector_map<FlatPlacementBinId, FlatPlacementBinId> bin_ids_;

    /// @brief The contained AP blocks of each bin, in no particular order.
    vtr::vector_map<FlatPlacementBinId, vtr::vector<BinSlotId, APBlockId>> bin_contained_blocks_;

    /// @brief The bin that contains each AP block.
    vtr::vector<APBlockId, FlatPlacementBinId> block_bin_;

    /// @brief The slot of each AP block within the contained blocks of its bin.
    ///        Invalid if the block is not in a bin.
    vtr::vector<APBlockId, BinSlotId> block_bin_slot_;

    /// @brief The 2D region that each bin represents on a layer of the FPGA grid.
    vtr::vector_map<FlatPlacementBinId, vtr::Rect<double>> bin_region_;

    /// @brief The layer of the FPGA that the bin occupies.
    vtr::vector_map<FlatPlacementBinId, size_t> bin_layer_;
};
