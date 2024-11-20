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
#include <unordered_set>
#include <vector>
#include "ap_netlist_fwd.h"
#include "primitive_vector.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_ndmatrix.h"
#include "vtr_strong_id.h"
#include "vtr_vector_map.h"

// Forward declarations
class APNetlist;
struct PartialPlacement;

/**
 * @brief Enumeration of all of the partial legalizers currently implemented in
 *        VPR.
 */
enum class e_partial_legalizer {
    FLOW_BASED      // Multi-commodity flow-based partial legalizer.
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
    PartialLegalizer(const APNetlist& netlist, int log_verbosity = 1)
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
                                                         const APNetlist& netlist);

/**
 * @brief A strong ID for the bins used in the partial legalizer.
 *
 * This allows a separation between the legalizers and tiles such that a bin may
 * represent multiple tiles.
 */
struct legalizer_bin_tag {};
typedef vtr::StrongId<legalizer_bin_tag, size_t> LegalizerBinId;

/**
 * @brief A bin used to contain blocks in the partial legalizer.
 *
 * Bins can be thought of as generalized tiles which have a capacity of blocks
 * (and their types) and a current utilization of the bin. A bin may represent
 * multiple tiles.
 *
 * The capacity, utilization, supply, and demand of the bin are stored as
 * M-dimensional vectors; where M is the number of models (primitives) in the
 * device. This allows the bin to quickly know how much of each types of
 * primitives it can contain and how much of each type it currently contains.
 */
struct LegalizerBin {
    /// @brief The blocks currently contained in this bin.
    std::unordered_set<APBlockId> contained_blocks;

    /// @brief The maximum mass of each primitive type this bin can contain.
    PrimitiveVector capacity;

    /// @brief The current mass of each primitive type this bin contains.
    PrimitiveVector utilization;

    /// @brief The current over-utilization of the bin. This is defined as:
    ///             elementwise_max(utilization - capacity, 0)
    PrimitiveVector supply;

    /// @brief The current under-utilization of the bin. This is defined as:
    ///             elementwise_max(capacity - utilization, 0)
    PrimitiveVector demand;

    /// @brief The bounding box of the bin on the device grid. This is the
    /// positions on the grid the blocks will exist.
    ///
    /// For example, if the tile at location (2,3) was turned directly into a
    /// bin, the bounding box of that bin would be [(2.0, 3.0), (3.0, 4.0))
    /// Notice the notation here. The left and bottom edges are included in the
    /// set.
    /// It is implied that blocks cannot be placed on the right or top edges of
    /// the bounding box (since then they may be in another bin!).
    ///
    /// NOTE: This uses a double to match the precision of the positions of
    ///       APBlocks (which are doubles). The use of a double here also allows
    ///       bins to represent partial tiles which may be useful.
    vtr::Rect<double> bounding_box;

    /// @brief The neighbors of this bin. These are neighboring bins that this
    ///        bin can flow blocks to.
    std::vector<LegalizerBinId> neighbors;

    /**
     * @brief Helper method to compute the supply of the bin.
     */
    void compute_supply() {
        supply = utilization - capacity;
        supply.relu();
        VTR_ASSERT_DEBUG(supply.is_non_negative());
    }

    /**
     * @brief Helper method to compute the demand of the bin.
     */
    void compute_demand() {
        demand = capacity - utilization;
        demand.relu();
        VTR_ASSERT_DEBUG(demand.is_non_negative());
    }
};

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
 *
 *
 * TODO: Make the bin size a parameter for the legalizer somehow. That way we
 *       can make 1x1 bins for very accurate legalizers and larger (clamped) for
 *       less accurate legalizers.
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

    /// @brief A vector of all the bins in the legalizer.
    vtr::vector_map<LegalizerBinId, LegalizerBin> bins_;

    /// @brief A reverse lookup between every block and the bin they are
    ///        currently in.
    vtr::vector_map<APBlockId, LegalizerBinId> block_bins_;

    /// @brief The mass of each APBlock, represented as a primitive vector.
    vtr::vector_map<APBlockId, PrimitiveVector> block_masses_;

    /// @brief A lookup that gets the bin that represents every tile (and
    ///        sub-tile).
    vtr::NdMatrix<LegalizerBinId, 2> tile_bin_;

    /// @brief A set of overfilled bins. Instead of computing this when needed,
    ///        this list is maintained whenever a block is moved from one bin to
    ///        another.
    std::unordered_set<LegalizerBinId> overfilled_bins_;

    /**
     * @brief Returns true if the given bin is overfilled.
     */
    inline bool bin_is_overfilled(LegalizerBinId bin_id) const {
        VTR_ASSERT_DEBUG(bin_id.is_valid());
        VTR_ASSERT_DEBUG(bins_[bin_id].supply.is_non_negative());
        // By definition, a bin is overfilled if its supply is non-zero.
        return bins_[bin_id].supply.is_non_zero();
    }

    /**
     * @brief Helper method to insert a block into a bin.
     *
     * This method maintains all the necessary state of the class and updates
     * the bin the block is being inserted into.
     *
     * This method assumes that the given block is not currently in a bin.
     */
    inline void insert_blk_into_bin(APBlockId blk_id, LegalizerBinId bin_id) {
        VTR_ASSERT_DEBUG(blk_id.is_valid());
        VTR_ASSERT_DEBUG(bin_id.is_valid());
        // Make sure that this block is not anywhere else.
        VTR_ASSERT(block_bins_[blk_id] == LegalizerBinId::INVALID());
        // Insert the block into the bin.
        block_bins_[blk_id] = bin_id;
        LegalizerBin& bin = bins_[bin_id];
        bin.contained_blocks.insert(blk_id);
        // Update the utilization, supply, and demand.
        const PrimitiveVector& blk_mass = block_masses_[blk_id];
        bin.utilization += blk_mass;
        bin.compute_supply();
        bin.compute_demand();
        // Update the overfilled bins since this bin may have become overfilled.
        if (bin_is_overfilled(bin_id))
            overfilled_bins_.insert(bin_id);
    }

    /**
     * @brief Helper method to remove a block from a bin.
     *
     * This method maintains all the necessary state of the class and updates
     * the bin the block is being removed from.
     *
     * This method assumes that the given block is currently in the given bin.
     */
    inline void remove_blk_from_bin(APBlockId blk_id, LegalizerBinId bin_id) {
        VTR_ASSERT_DEBUG(blk_id.is_valid());
        VTR_ASSERT_DEBUG(bin_id.is_valid());
        // Make sure that this block is in this bin.
        VTR_ASSERT(block_bins_[blk_id] == bin_id);
        LegalizerBin& bin = bins_[bin_id];
        VTR_ASSERT_DEBUG(bin.contained_blocks.count(blk_id) == 1);
        // Remove the block from the bin.
        block_bins_[blk_id] = LegalizerBinId::INVALID();
        bin.contained_blocks.erase(blk_id);
        // Update the utilization, supply, and demand.
        const PrimitiveVector& blk_mass = block_masses_[blk_id];
        bin.utilization -= blk_mass;
        bin.compute_supply();
        bin.compute_demand();
        // Update the overfilled bins since this bin may no longer be
        // overfilled.
        if (!bin_is_overfilled(bin_id))
            overfilled_bins_.erase(bin_id);
    }

    /**
     * @brief Helper method to get the bin at the current device x and y tile
     *        coordinate.
     */
    inline LegalizerBinId get_bin(size_t x, size_t y) const {
        VTR_ASSERT_DEBUG(x < tile_bin_.dim_size(0));
        VTR_ASSERT_DEBUG(y < tile_bin_.dim_size(1));
        return tile_bin_[x][y];
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
    void compute_neighbors_of_bin(LegalizerBinId src_bin_id, size_t num_models);

    /**
     * @brief Debugging method which verifies that all the bins are valid.
     *
     * The bins are valid if:
     *  - All blocks are in bins
     *  - Every tile is represented by a bin
     *  - Every bin has the correct utilization, supply, and demand
     *  - The overfilled bins are correct
     */
    bool verify_bins() const;

    /**
     * @brief Resets all of the bins from a previous call to partial legalize.
     *
     * This removes all of the blocks from the bins.
     */
    void reset_bins();

    /**
     * @brief Import the given partial placement into bins.
     *
     * This is called at the beginning of legalize to prepare the bins with the
     * current placement.
     */
    void import_placement_into_bins(const PartialPlacement& p_placement);

    /**
     * @brief Export the placement found from spreading the bins.
     *
     * This is called at the end of legalize to write back the result of the
     * legalizer.
     */
    void export_placement_from_bins(PartialPlacement& p_placement) const;

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
    std::vector<std::vector<LegalizerBinId>> get_paths(LegalizerBinId src_bin_id,
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
    void flow_blocks_along_path(const std::vector<LegalizerBinId>& path,
                                const PartialPlacement& p_placement,
                                float psi);

public:

    /**
     * @brief Construcotr for the flow-based legalizer.
     *
     * Builds all of the bins, computing their capacities based on the device
     * description. Builds the connectivity of bins. Computes the mass of all
     * blocks in the netlist.
     */
    FlowBasedLegalizer(const APNetlist& netlist);

    /**
     * @brief Performs flow-based spreading on the given partial placement.
     *
     *  @param p_placement  The placmeent to legalize. The result of the partial
     *                      legalizer will be stored in this object.
     */
    void legalize(PartialPlacement &p_placement) final;
};

