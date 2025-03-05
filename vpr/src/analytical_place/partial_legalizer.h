/**
 * @file
 * @author  Alex Singer and Robert Luo
 * @date    October 2024
 * @brief   The declarations of the Partial Legalizer base class which is used
 *          to define the functionality of all partial legalizers in the AP
 *          flow.
 *
 * Partial Legalizers are parts of the flow which take in an illegal Partial
 * Placemenent and produce a more legal Partial Placement (according to
 * constraints of the architecture).
 */

#pragma once

#include <memory>
#include <vector>
#include "ap_netlist_fwd.h"
#include "flat_placement_bins.h"
#include "flat_placement_density_manager.h"
#include "primitive_vector.h"
#include "vtr_vector.h"

// Forward declarations
class APNetlist;
class Prepacker;
struct PartialPlacement;

/**
 * @brief Enumeration of all of the partial legalizers currently implemented in
 *        VPR.
 */
enum class e_partial_legalizer {
    FLOW_BASED,      // Multi-commodity flow-based partial legalizer.
    BI_PARTITIONING  // Bi-partitioning partial legalizer.
};

/**
 * @brief The Partial Legalizer base class
 *
 * This provied functionality that all Partial Legalizers will use.
 *
 * It provides a standard interface that all Partial Legalizers must implement
 * so thet can be used interchangably. This makes it very easy to test and
 * compare different solvers.
 */
class PartialLegalizer {
public:
    virtual ~PartialLegalizer() {}

    /**
     * @brief Constructor of the base PartialLegalizer class
     *
     * Currently just copies the parameters into the class as member varaibles.
     */
    PartialLegalizer(const APNetlist& netlist, int log_verbosity)
                        : netlist_(netlist),
                          log_verbosity_(log_verbosity) {}

    /**
     * @brief Partially legalize the given partial placement.
     *
     * This method will take in the Partial Placement as input and write a
     * more legal solution into this same object. Here we define legal as it
     * pertains to the constraints of the device.
     *
     * This class expects to receive a valid Partial Placement as input and will
     * generate a valid Partial Placement.
     *
     *  @param p_placement  The placement to legalize. Will be filled with the
     *                      legalized placement.
     */
    virtual void legalize(PartialPlacement &p_placement) = 0;

protected:

    /// @brief The APNetlist the legalizer will be legalizing the placement of.
    ///        It is implied that the netlist is not being modified during
    ///        global placement.
    const APNetlist& netlist_;

    /// @brief The verbosity of the log statements within the partial legalizer.
    ///        0 would be no log messages, 10 would print per-iteration status,
    ///        20 would print logs messages within each iteration.
    int log_verbosity_;
};

/**
 * @brief A factory method which creates a Partial Legalizer of the given type.
 */
std::unique_ptr<PartialLegalizer> make_partial_legalizer(e_partial_legalizer legalizer_type,
                                                         const APNetlist& netlist,
                                                         std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                                         int log_verbosity);

/**
 * @brief A multi-commodity flow-based spreading partial legalizer.
 *
 * This puts the current blocks into bins based on the given placement. It then
 * finds paths from bins that are overfilled to bins that are underfilled. Then
 * it flows blocks along these paths. Each iteration, the maximum distance that
 * blocks can flow is increased. This tries to spread out blocks by moving them
 * the smallest distance that it can.
 *
 * This technique is a modified version of the algorithm proposed by Darav et
 * al. Their algorithm was tailored for their Microsemi FPGA. This code extends
 * on their work by generalizing it to any theoretical architecture which can be
 * expressed in VPR.
 *          https://doi.org/10.1145/3289602.3293896
 */
class FlowBasedLegalizer : public PartialLegalizer {
private:
    /// @brief The maximum number of iterations the legalizer can take. This
    ///        prevents the legalizer from never converging if there is not
    ///        enough space to flow blocks.
    static constexpr size_t max_num_iterations_ = 100;

    /// @brief The maximum number of hops away a neighbor of a bin can be. Where
    ///        a hop is the minimum number of bins you need to pass through to
    ///        get to this neighbor (manhattan distance in bins-space).
    ///
    /// This is used to speed up the computation of the neighbors of bins since
    /// it reduces the amount of the graph that needs to be explored.
    ///
    /// TODO: This may need to be made per primitive type since some types may
    ///       need to explore more of the architecture than others to find
    ///       sufficient neighbors.
    static constexpr unsigned max_bin_neighbor_dist_ = 4;

    /// @brief The density manager which manages how the bins are constructed
    ///        and maintains how overfilled bins are.
    std::shared_ptr<FlatPlacementDensityManager> density_manager_;

    /// @brief The neighbors of each bin.
    ///
    /// These are the closest bins in each direction for each model type to flow
    /// from this bin into.
    vtr::vector<FlatPlacementBinId, std::vector<FlatPlacementBinId>> bin_neighbors_;

    /**
     * @brief Get the supply of the given bin. Supply is how much over-capacity
     *        the bin is.
     */
    inline const PrimitiveVector& get_bin_supply(FlatPlacementBinId bin_id) const {
        // Supply is defined as the overfill of the bin.
        return density_manager_->get_bin_overfill(bin_id);
    }

    /**
     * @brief Get the demand of the given bin. Demand is how much under-capacity
     *        the bin is.
     */
    inline const PrimitiveVector& get_bin_demand(FlatPlacementBinId bin_id) const {
        // Demand is defined as the underfill of the bin.
        return density_manager_->get_bin_underfill(bin_id);
    }

    /**
     * @brief Computes the neighbors of the given bin.
     *
     * This is different from the algorithm proposed by Darav et al.
     *
     * Each bin needs to be connected to every type of block. This is because,
     * due to the placement being able to place blocks anywhere on the grid, it
     * is possible that any type of block can be in any bin. If a bin has a
     * block of a given type and no neighbor of the same type, the algorithm
     * will never converge.
     *
     * It is also important that every bin have many different "directions" that
     * it can flow blocks for each block type so it can legalize quickly.
     *
     * The original paper has a fixed architecture, so it builds the bin graph
     * directly for their architecture. For VPR, a BFS is performed which finds
     * bins in each of the four cardinal directions with the minimum manhattan
     * distance for all of the different types of blocks.
     *
     *  @param src_bin_id   The bin to compute the neighbors for.
     *  @param num_models   The number of models in the architecture.
     */
    void compute_neighbors_of_bin(FlatPlacementBinId src_bin_id, size_t num_models);

    /**
     * @brief Debugging method which verifies that all the bins are valid.
     *
     * The bins are valid if:
     *  - All blocks are in bins
     *  - Every tile is represented by a bin
     *  - Every bin has the correct utilization, supply, and demand
     *  - The overfilled bins are correct
     */
    bool verify() const;

    /**
     * @brief Gets paths to flow blocks from the src_bin_id at a maximum cost
     *        of psi.
     *
     *  @param src_bin_id   The bin that all paths will originate from.
     *  @param p_placement  The placement being legalized (used for cost
     *                      calculations).
     *  @param psi          An algorithm parameter that increases over many
     *                      iterations. The "max-cost" a path can be.
     */
    std::vector<std::vector<FlatPlacementBinId>> get_paths(FlatPlacementBinId src_bin_id,
                                                           const PartialPlacement& p_placement,
                                                           float psi);

    /**
     * @brief Flows the blocks along the given path.
     *
     * The blocks do a conga line maneuver where blocks move towards the end
     * of the path.
     *
     *  @param path         The path to flow blocks along.
     *  @param p_placement  The placement being legalized (used for cost
     *                      calculations).
     *  @param psi          An algorithm parameter that increases over many
     *                      iterations. The "max-cost" a path can be.
     */
    void flow_blocks_along_path(const std::vector<FlatPlacementBinId>& path,
                                const PartialPlacement& p_placement,
                                float psi);

public:

    /**
     * @brief Constructor for the flow-based legalizer.
     *
     * Builds all of the bins, computing their capacities based on the device
     * description. Builds the connectivity of bins. Computes the mass of all
     * blocks in the netlist.
     */
    FlowBasedLegalizer(const APNetlist& netlist,
                       std::shared_ptr<FlatPlacementDensityManager> density_manager,
                       int log_verbosity);

    /**
     * @brief Performs flow-based spreading on the given partial placement.
     *
     *  @param p_placement  The placmeent to legalize. The result of the partial
     *                      legalizer will be stored in this object.
     */
    void legalize(PartialPlacement &p_placement) final;
};

/**
 * @brief A bi-paritioning spreading full legalizer.
 *
 * This creates minimum spanning windows around overfilled bins in the device
 * such that the capacity of the bins within the window is just higher than the
 * current utilization of the bins within the window. These windows are then
 * split in both region and contained atoms. This spatially spreads out the
 * atoms within each window. This splitting continues until the windows are
 * small enough and the atoms are placed. The benefit of this approach is that
 * it cuts the problem size for each partition, which can yield improved
 * performance when there is a lot of overfill.
 *
 * This technique is based on the lookahead legalizer in SimPL and the window-
 * based legalization found in GPlace3.0.
 *          SimPL: https://doi.org/10.1145/2461256.2461279
 *          GPlace3.0: https://doi.org/10.1145/3233244
 */
class BiPartitioningPartialLegalizer : public PartialLegalizer {
public:
    /**
     * @brief Constructor for the bi-partitioning partial legalizer.
     *
     * Uses the provided denisity manager to identify the capacity and
     * utilization of regions of the device.
     */
    BiPartitioningPartialLegalizer(const APNetlist& netlist,
                                   std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                   int log_verbosity);

    /**
     * @brief Perform bi-partitioning spreading on the given partial placement.
     *
     *  @param p_placement
     *          The placement to legalize. The result of the partial legalizer
     *          will be stored in this object.
     */
    void legalize(PartialPlacement& p_placement) final;

private:
    /// @brief The density manager which manages the capacity and utilization
    ///        of regions of the device.
    std::shared_ptr<FlatPlacementDensityManager> density_manager_;
};

