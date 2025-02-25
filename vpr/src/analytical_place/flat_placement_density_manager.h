/**
 * @file
 * @author  Alex Singer
 * @date    February 2024
 * @brief   Manager class for how density is calculated in the AP flow.
 *
 * This class decides how the FPGA grid is partitioned into bins and what
 * defines a bin that is "overfilled".
 */

#pragma once

#include <tuple>
#include <unordered_set>
#include <vector>
#include "flat_placement_bins.h"
#include "flat_placement_mass_calculator.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"

class APNetlist;
class AtomNetlist;
class DeviceGrid;
class Prepacker;
struct PartialPlacement;
struct t_logical_block_type;
struct t_physical_tile_type;

/**
 * @brief Manager class for computing the density of a flat placement.
 *
 * Density is a function of mass and volume. Since a flat placement is a
 * continuous placement of discrete objects, the space the objects are placed
 * over needs to be partitioned into discrete bins. Regions that are too dense
 * are called overfilled bins and regions that may hold more (not too dense)
 * are called underfilled bins. This class manages the construction of these
 * bins and how overfilled / underfilled they are.
 *
 * Currently, a bin is created for each tile in the FPGA grid (with a unique
 * root tile location). For example, a CLB taking up a single tile would be a
 * 1x1 bin, while a DSP block taking up multiple tiles may be a 4x1 bin. The
 * capacity of each bin is the capacity of the tile it represents (as computed
 * by the flat placement mass calculator). When AP blocks are added / removed
 * from bins, this class will maintain the current utilization of the bin. Since
 * these masses / capacities are repesented by M-dimensional quantities (where
 * M is the number of models in the architecture), the overfill and underfill of
 * each bin is given as an M-dimensional vector. For example, in an architecture
 * of only LUTs and FFs, an overfill of <3, 1> means that a bin has 3 too many
 * LUTs and 1 too many FFs.
 *
 * This class is able to answer questions about the current density of the flat
 * placement such as which bins are currently overfilled, what bin is at the
 * given location, etc.
 *
 * TODO: Add an option to this class to change the granularity of the bins. This
 *       may allow us to trade off quality and runtime.
 */
class FlatPlacementDensityManager {
public:
    /**
     * @brief Construct the density manager.
     *
     *  @param ap_netlist
     *  @param prepacker
     *  @param atom_netlist
     *  @param device_grid
     *  @param logical_block_types
     *  @param physical_tile_types
     *  @param log_verbosity
     */
    FlatPlacementDensityManager(const APNetlist& ap_netlist,
                                const Prepacker& prepacker,
                                const AtomNetlist& atom_netlist,
                                const DeviceGrid& device_grid,
                                const std::vector<t_logical_block_type>& logical_block_types,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                int log_verbosity);

    /**
     * @brief Returns a reference to the bins the manager has created.
     */
    inline const FlatPlacementBins& flat_placement_bins() const {
        return bins_;
    }

    /**
     * @brief Returns a reference to the mass calculator that the manager class
     *        is using to detect overfilled / undefilled bins.
     */
    inline const FlatPlacementMassCalculator& mass_calculator() const {
        return mass_calculator_;
    }

    /**
     * @brief Returns the bin located at the given (x, y, layer) position.
     */
    FlatPlacementBinId get_bin(double x, double y, double layer) const;

    /**
     * @brief Returns the size of the placeable region, i.e. the region that
     *        contains all bins.
     */
    inline std::tuple<double, double, double> get_overall_placeable_region_size() const {
        return std::make_tuple(bin_spatial_lookup_.dim_size(1),  // width
                               bin_spatial_lookup_.dim_size(2),  // height
                               bin_spatial_lookup_.dim_size(0)); // depth
    }

    /**
     * @brief Insert the given block into the given bin.
     *
     * As well as updating the bin's contents, also maintains the utilization
     * of the bins.
     */
    void insert_block_into_bin(APBlockId blk_id, FlatPlacementBinId bin_id);

    /**
     * @brief Remove the given block from the given bin.
     *
     * Like insertion, this maintains the utilization of bins.
     */
    void remove_block_from_bin(APBlockId blk_id, FlatPlacementBinId bin_id);

    /**
     * @brief Returns the current utilization of the given bin.
     *
     * This is the sum of the mass of each atoms in the given bin.
     */
    inline const PrimitiveVector& get_bin_utilization(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_utilization_[bin_id];
    }

    /**
     * @brief Returns the capacity of the given bin.
     *
     * This is an approximation of the amount of mass that the tile that this
     * bin represents can hold.
     */
    inline const PrimitiveVector& get_bin_capacity(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_capacity_[bin_id];
    }

    /**
     * @brief Returns how overfilled the given bin is.
     *
     * This cannot be negative. This is how much over the capacity the current
     * utilization is. An overfill of 0 implies that the bin is not overfilled.
     */
    inline const PrimitiveVector& get_bin_overfill(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_overfill_[bin_id];
    }

    /**
     * @brief Returns how underfilled the given bin is.
     *
     * This cannot be negative. This is how much more mass the given bin can
     * hold without going over capactiy.
     */
    inline const PrimitiveVector& get_bin_underfill(FlatPlacementBinId bin_id) const {
        VTR_ASSERT(bin_id.is_valid());
        return bin_underfill_[bin_id];
    }

    /**
     * @brief Returns true of the given bin is overfilled (it contains too much
     *        mass and is over capacity).
     */
    inline bool bin_is_overfilled(FlatPlacementBinId bin_id) const {
        // A bin is overfilled if the overfill is non-zero.
        return get_bin_overfill(bin_id).is_non_zero();
    }

    /**
     * @brief Returns a list of all overfilled bins.
     */
    inline const std::unordered_set<FlatPlacementBinId>& get_overfilled_bins() const {
        return overfilled_bins_;
    }

    /**
     * @brief Import the given flat placement into the bins.
     *
     * This will place AP blocks into the bins that they are placed over.
     */
    void import_placement_into_bins(const PartialPlacement& p_placement);

    /**
     * @brief Exports the placement of blocks in bins to a flat placement.
     *
     * This will move each block to the position closest to the original flat
     * placement that is still within the bin the block was placed into.
     */
    void export_placement_from_bins(PartialPlacement& p_placement) const;

    /**
     * @brief Gets the position of the AP block within the bin it contains.
     *
     * This will return the position of the block that is closest to the position
     * in the given flat placement, while still being within the the bin region.
     *
     * For example, if the block is located within the bin, its position will
     * be returned (unmodified). If the block is located to the left of the bin
     * (y coordinate is within the bounds of the bin), then this will return
     * the point on the left edge of the bin with the same y coordinate as the
     * block.
     *
     * TODO: It may be a good idea to investigate placing blocks at the input
     *       or output pin locations of the bin.
     */
    vtr::Point<double> get_block_location_in_bin(APBlockId blk_id,
                                                 const vtr::Rect<double>& bin_region,
                                                 const PartialPlacement& p_placement) const;

    /**
     * @brief Resets all bins by emptying them.
     */
    void empty_bins();

    /**
     * @brief Verifies that the bins were constructed correctly and that the
     *        utilization, overfill, underfill, and capacity are all correct.
     *        Returns false if there are any issues.
     */
    bool verify() const;

    /**
     * @brief Debug printer which prints a simple representation of the bins
     *        and their capacity to the log file.
     */
    void print_bin_grid() const;

private:
    /// @brief The AP netlist of blocks which are filling the bins.
    const APNetlist& ap_netlist_;

    /// @brief The bins created by this class.
    FlatPlacementBins bins_;

    /// @brief The mass calculator used to compute the mass of the blocks and
    ///        physical tiles.
    FlatPlacementMassCalculator mass_calculator_;

    /// @brief Spatial lookup for an (layer, x, y) position to the bin at that
    ///        location.
    ///
    /// Access: [0..grid.num_layers-1][0..grid.width-1][0..grid.height-1]
    vtr::NdMatrix<FlatPlacementBinId, 3> bin_spatial_lookup_;

    /// @brief The capacity of each bin.
    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_capacity_;

    /// @brief The utilization of each bin.
    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_utilization_;

    /// @brief The overfill of each bin.
    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_overfill_;

    /// @brief The underfill of each bin.
    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_underfill_;

    /// @brief The set of overfilled bins.
    std::unordered_set<FlatPlacementBinId> overfilled_bins_;

    /// @brief The verbosity of log messages in this class.
    const int log_verbosity_;
};

