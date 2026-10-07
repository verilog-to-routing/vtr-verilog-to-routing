#pragma once
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

#include <cmath>
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

#include "ap_netlist_fwd.h"
#include "ap_flow_enums.h"
#include "flat_placement_bins.h"
#include "flat_placement_density_manager.h"
#include "for_each_index.h"
#include "logic_types.h"
#include "model_grouper.h"
#include "primitive_dim_manager.h"
#include "primitive_vector_fwd.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_ndmatrix.h"
#include "vtr_prefix_sum.h"
#include "vtr_vector.h"

// Forward declarations
class APNetlist;
class Prepacker;
struct PartialPlacement;
struct BisectionSegment;
struct WindowBisectionContext;

/**
 * @brief The Partial Legalizer base class
 *
 * This provides functionality that all Partial Legalizers will use.
 *
 * It provides a standard interface that all Partial Legalizers must implement
 * so they can be used interchangeably. This makes it very easy to test and
 * compare different solvers.
 */
class PartialLegalizer {
  public:
    virtual ~PartialLegalizer() {}

    /**
     * @brief Constructor of the base PartialLegalizer class
     *
     * Currently just copies the parameters into the class as member variables.
     */
    PartialLegalizer(const APNetlist& netlist, int log_verbosity)
        : netlist_(netlist)
        , log_verbosity_(log_verbosity) {}

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
    virtual void legalize(PartialPlacement& p_placement) = 0;

    /**
     * @brief Print statistics on the Partial Legalizer.
     *
     * This is expected to be called at the end of Global Placement to provide
     * cumulative information on how much work the partial legalizer performed.
     */
    virtual void print_statistics() = 0;

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
std::unique_ptr<PartialLegalizer> make_partial_legalizer(e_ap_partial_legalizer legalizer_type,
                                                         const APNetlist& netlist,
                                                         std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                                         const Prepacker& prepacker,
                                                         const LogicalModels& models,
                                                         int log_verbosity);

/**
 * @brief A partial legalizer which does not legalize anything. This solver acts
 *        like an identity matrix where it just passes the given solution along.
 *        This partial legalizer should only be used for testing.
 */
class IdentityPartialLegalizer : public PartialLegalizer {
  public:
    IdentityPartialLegalizer(const APNetlist& netlist, int log_verbosity)
        : PartialLegalizer(netlist, log_verbosity) {}

    void legalize(PartialPlacement& p_placement) final {
        (void)p_placement;
        // Do nothing.
    }

    void print_statistics() final {
        // Do nothing.
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
    void compute_neighbors_of_bin(FlatPlacementBinId src_bin_id, const LogicalModels& models);

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
                       const LogicalModels& models,
                       int log_verbosity);

    /**
     * @brief Performs flow-based spreading on the given partial placement.
     *
     *  @param p_placement  The placmeent to legalize. The result of the partial
     *                      legalizer will be stored in this object.
     */
    void legalize(PartialPlacement& p_placement) final;

    void print_statistics() final {}
};

/**
 * @brief A cluster of flat placement bins.
 */
typedef typename std::vector<FlatPlacementBinId> FlatPlacementBinCluster;

/**
 * @brief Enum for the direction of a partition.
 */
enum class e_partition_dir {
    HORIZONTAL, ///< Horizontal cut along the x-axis.
    VERTICAL,   ///< Vertical cut along the y-axis.
    PLANAR,     ///< Planar cut along the z-axis.
};

/**
 * @brief Spatial window used to spread the blocks contained within.
 *
 * This window's region is identified and grown until it has enough space to
 * accommodate the blocks stored within. This window is then successivly
 * partitioned until it is small enough (blocks are not too dense).
 */
struct SpreadingWindow {
    /// @brief The blocks contained within this window.
    std::vector<APBlockId> contained_blocks;

    /// @brief The 2D region of space that this window covers.
    vtr::Rect<double> region;

    /// @brief The range of layers that this window covers. Along with the region,
    ///        this describes a 3D rectangular prism for the spreading window.
    ///        For 2D architectures, layer_low = layer_high = 0.
    size_t layer_low;
    size_t layer_high;

    /**
     * @brief Returns true if this window strictly overlaps with another window.
     *
     * This is used to decide if two windows should be merged or not.
     */
    inline bool overlaps(const SpreadingWindow& other_window) const {
        // If the regions do not overlap, return false.
        if (!region.strictly_overlaps(other_window.region))
            return false;

        // If the layers do not overlap return false.
        if (layer_low > other_window.layer_high)
            return false;
        if (layer_high < other_window.layer_low)
            return false;

        return true;
    }

    /**
     * @brief Get the volume of the 3D rectangular prism that this window covers.
     */
    inline double window_area() const {
        return region.width() * region.height() * (double)(layer_high - layer_low + 1);
    }

    /**
     * @brief Merge the given window into this window such that they occupy the
     *        same volume.
     *
     * NOTE: This does NOT merge the blocks in the window.
     */
    inline void merge_window_area(const SpreadingWindow& other_window) {
        region = vtr::bounding_box(region, other_window.region);
        layer_low = std::min(layer_low, other_window.layer_low);
        layer_high = std::max(layer_high, other_window.layer_high);
    }
};

/**
 * @brief Struct to hold the information from partitioning a window. Contains
 *        the two window partitions and some information about how they were
 *        generated.
 */
struct PartitionedWindow {
    /// @brief The direction of the partition.
    e_partition_dir partition_dir;

    /// @brief The position that the parent window was split at.
    double pivot_pos;

    /// @brief The lower window. This is the left partition when the direction
    ///        is vertical, and the bottom partition when the direction is
    ///        horizontal.
    SpreadingWindow lower_window;

    /// @brief The upper window. This is the right partition when the direction
    ///        is vertical, and the top partition when the direction is
    ///        horizontal.
    SpreadingWindow upper_window;
};

/**
 * @brief Wrapper class around the prefix sum class which creates a prefix sum
 *        for each dim type and has helper methods for getting the sums over
 *        regions.
 */
class PerPrimitiveDimPrefixSum2D {
    // This class stores the prefix sum as a fixed-point number instead of a
    // floating point number. This is to prevent error accumulation which can
    // occur due to numerical imprecisions. This variable selects how many bits
    // the fractional component of the fixed-point number should have.
    static constexpr unsigned num_fractional_bits_ = 10;

    // When converting to/from fixed-point, we need to scale/shrink by 2 to the
    // power of the number of fractional bits.
    static constexpr float fractional_scale_ = 2 << num_fractional_bits_;

  public:
    PerPrimitiveDimPrefixSum2D() = default;

    /**
     * @brief Construct prefix sums for each of the primitive vector dims.
     *
     * Uses the density manager to get the size of the placeable region.
     *
     * The lookup returns the value of a given bin for a given dim. Each bin's
     * value is placed on its center tile.
     */
    template<typename BinLookup>
    PerPrimitiveDimPrefixSum2D(const FlatPlacementDensityManager& density_manager,
                               BinLookup bin_lookup) {
        // Get the size that the prefix sums should be.
        auto [width, height, num_layers] = density_manager.get_overall_placeable_region_size();

        // Find the center tile of each bin and group the bins by layer. The value
        // of a bin is placed on its center tile since the prefix sum assumes a
        // 1x1 grid of values. All other tiles of the bin hold zero.
        struct t_bin_center {
            FlatPlacementBinId bin_id;
            size_t x;
            size_t y;
        };

        const FlatPlacementBins& bins = density_manager.flat_placement_bins();
        std::vector<std::vector<t_bin_center>> layer_bin_centers(num_layers);
        for (FlatPlacementBinId bin_id : bins.bins()) {
            const vtr::Rect<double>& bin_region = bins.bin_region(bin_id);
            size_t center_x = std::floor(bin_region.xmin() + bin_region.width() / 2.0);
            size_t center_y = std::floor(bin_region.ymin() + bin_region.height() / 2.0);
            VTR_ASSERT_SAFE(center_x < width && center_y < height);
            size_t layer = bins.bin_layer(bin_id);
            VTR_ASSERT_SAFE(layer < num_layers);
            layer_bin_centers[layer].push_back({bin_id, center_x, center_y});
        }

        // List the (layer, dim) pairs that need a prefix sum and make room for
        // each of them, so the pairs can be built independently of each other.
        struct t_layer_dim {
            size_t layer;
            PrimitiveVectorDim dim;
        };

        const PrimitiveDimManager& dim_manager = density_manager.mass_calculator().get_dim_manager();
        std::vector<PrimitiveVectorDim> used_dims = density_manager.get_used_dims_mask().get_non_zero_dims();
        std::vector<t_layer_dim> layer_dims;
        layer_dims.reserve(num_layers * used_dims.size());
        layer_dim_prefix_sum_.resize(num_layers);
        for (size_t layer = 0; layer < num_layers; layer++) {
            layer_dim_prefix_sum_[layer].resize(dim_manager.dims().size());
            for (PrimitiveVectorDim dim : used_dims) {
                layer_dims.push_back({layer, dim});
            }
        }

        // Create each of the prefix sums. Each pair writes only its own prefix
        // sum, so the pairs are built in parallel.
        for_each_index_chunk(layer_dims.size(), [&](size_t begin, size_t end) {
            // Dense grid of fixed-point values for one layer and dim. Each
            // chunk has its own grid and refills it for each prefix sum.
            vtr::NdMatrix<uint64_t, 2> vals({width, height}, 0);
            for (size_t i = begin; i < end; i++) {
                size_t layer = layer_dims[i].layer;
                PrimitiveVectorDim dim = layer_dims[i].dim;
                vals.fill(0);
                for (const t_bin_center& bin_center : layer_bin_centers[layer]) {
                    // Convert the floating point value into fixed point to prevent
                    // error accumulation in the prefix sum.
                    // Note: We ceil here since we do not want to lose information
                    //       on numbers that get very close to 0.
                    float val = bin_lookup(dim, bin_center.bin_id);
                    VTR_ASSERT_SAFE_MSG(val >= 0.0f,
                                        "PerPrimitiveDimPrefixSum2D expected to only hold positive values");
                    vals[bin_center.x][bin_center.y] = std::ceil(val * fractional_scale_);
                }
                layer_dim_prefix_sum_[layer][dim] = vtr::PrefixSum2D<uint64_t>(vals);
            }
        });
    }

    /**
     * @brief Get the sum for a given dim over the given region.
     */
    float get_dim_sum(PrimitiveVectorDim dim,
                      const vtr::Rect<double>& region,
                      size_t layer) const;

    /**
     * @brief Get the multi-dimensional sum over the given dims over
     *        the given region.
     */
    PrimitiveVector get_sum(const std::vector<PrimitiveVectorDim>& dims,
                            const vtr::Rect<double>& region,
                            size_t layer) const;

    /**
     * @brief Get the sum over the given dims over the given region as a
     *        single scalar. Same as the Manhattan norm of get_sum() when
     *        all values are non-negative.
     */
    float get_total_sum(const std::vector<PrimitiveVectorDim>& dims,
                        const vtr::Rect<double>& region,
                        size_t layer) const;

  private:
    /// @brief Per-layer, Per-Dim Prefix Sums. These are stored as fixed-point
    ///        numbers to prevent error accumulations due to numerical imprecisions.
    ///         [layer][vector_dim] -> 2d_prefix_sum
    std::vector<vtr::vector<PrimitiveVectorDim, vtr::PrefixSum2D<uint64_t>>> layer_dim_prefix_sum_;
};

/// @brief A unique ID of a group of primitive dims created by the PrimitiveDimGrouper class.
typedef vtr::StrongId<struct primitive_group_id_tag, size_t> PrimitiveGroupId;

/**
 * @brief A manager class for grouping together dimensions of the primitive
 *        vector which must be legalized together in a flat placement due to
 *        how the models they represent being associated with one another.
 */
class PrimitiveDimGrouper {
  public:
    // Iterator for the primitive group IDs.
    typedef typename vtr::vector_map<PrimitiveGroupId, PrimitiveGroupId>::const_iterator prim_group_iterator;

    // Range for the primitive group IDs.
    typedef typename vtr::Range<prim_group_iterator> prim_group_range;

  public:
    PrimitiveDimGrouper() = delete;

    /**
     * @brief Constructor for the primitive grouper class. Groups are formed here.
     *
     *  @param prepacker
     *      The prepacker used to create molecules in the flat placement.
     *  @param models
     *      The logical models in the architecture.
     *  @param density_manager
     *      The density manager object used to manage mass in the legalizer.
     *  @param dim_manager
     *      The primitive vector dimension manager.
     *  @param log_verbosity
     *      The verbosity of log messages in the grouper class.
     */
    PrimitiveDimGrouper(const Prepacker& prepacker,
                        const LogicalModels& models,
                        const FlatPlacementDensityManager& density_manager,
                        const PrimitiveDimManager& dim_manager,
                        int log_verbosity);

    /**
     * @brief Returns a list of all valid primitive group IDs.
     */
    inline prim_group_range groups() const {
        return vtr::make_range(group_ids_.begin(), group_ids_.end());
    }

    /**
     * @brief Gets the primitive group ID of the given primitive dim.
     */
    inline PrimitiveGroupId get_dim_group_id(PrimitiveVectorDim dim) const {
        VTR_ASSERT_SAFE_MSG(dim.is_valid(),
                            "Cannot get the group of an invalid dim");
        PrimitiveGroupId group_id = dim_group_id_[dim];
        VTR_ASSERT_SAFE_MSG(group_id.is_valid(),
                            "Dim is not in a group");
        return group_id;
    }

    /**
     * @brief Gets the primitive dims in the given primitive group.
     */
    inline const std::vector<PrimitiveVectorDim>& get_dims_in_group(PrimitiveGroupId group_id) const {
        VTR_ASSERT_SAFE_MSG(group_id.is_valid(),
                            "Invalid group id");
        VTR_ASSERT_SAFE_MSG(groups_[group_id].size() != 0,
                            "Group is empty");
        return groups_[group_id];
    }

  private:
    /// @brief Grouper object which handles grouping together models which must
    ///        be spread together. Models are grouped based on the pack patterns
    ///        that they can form with each other.
    ModelGrouper model_grouper_;

    /// @brief List of all primitive group IDs.
    vtr::vector_map<PrimitiveGroupId, PrimitiveGroupId> group_ids_;

    /// @brief A lookup between primitive dims and the group ID that contains them.
    vtr::vector<PrimitiveVectorDim, PrimitiveGroupId> dim_group_id_;

    /// @brief A lookup between each primitive group ID and the dims in that group.
    vtr::vector<PrimitiveGroupId, std::vector<PrimitiveVectorDim>> groups_;
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
  private:
    /// @brief The maximum gap between overfilled bins we can have in a flat
    ///        placement bin cluster. For example, if this is set to 1, we will
    ///        allow two overfilled bins to be clustered together if they only
    ///        have 1 non-overfilled bin of gap between them.
    /// The rational behind this is that it allows us to predict that the windows
    /// created for each cluster will overlap if they are within some gap distance.
    /// Increasing this number too much may cluster bins together too much and
    /// create large windows; decreasing this number will put more pressure on
    /// the window generation code, which can increase window size and runtime.
    /// TODO: Should this be distance instead of number of bins?
    static constexpr int max_bin_cluster_gap_ = 2;

    /// @brief The smallest number of blocks a window must contain for its
    ///        bisection to run as its own task. Smaller windows are bisected
    ///        on the thread that created them, since the task overhead would
    ///        outweigh the work.
    static constexpr size_t min_blocks_per_bisection_task_ = 256;

  public:
    /**
     * @brief Constructor for the bi-partitioning partial legalizer.
     *
     * Uses the provided denisity manager to identify the capacity and
     * utilization of regions of the device.
     */
    BiPartitioningPartialLegalizer(const APNetlist& netlist,
                                   std::shared_ptr<FlatPlacementDensityManager> density_manager,
                                   const Prepacker& prepacker,
                                   const LogicalModels& models,
                                   int log_verbosity);

    /**
     * @brief Perform bi-partitioning spreading on the given partial placement.
     *
     *  @param p_placement
     *          The placement to legalize. The result of the partial legalizer
     *          will be stored in this object.
     */
    void legalize(PartialPlacement& p_placement) final;

    /**
     * @brief Print statistics on the BiPartitioning Partial Legalizer.
     */
    void print_statistics() final;

  private:
    // ========================================================================
    //      Identifying spreading windows
    // ========================================================================

    /**
     * @brief Identify spreading windows which contain overfilled bins in the
     *        given model group on the device and do not overlap.
     *
     * This process is split into 4 stages:
     *      1) Overfilled bins are identified and clustered.
     *      2) Grow windows around the overfilled bin clusters. These windows
     *         will grow until there is just enough space to accommodate the blocks
     *         within the window (capacity of the window is larger than the utilization).
     *      3) Merge overlapping windows.
     *      4) Move the blocks within these window regions from their bins into
     *         their windows. This updates the current utilization of bins, making
     *         spreading easier.
     *
     * We identify non-overlapping windows for different model groups independently
     * for a few reasons:
     *  - Each model group, by design, can be spread independent of each other.
     *    This reduces the problem size by the number of groups.
     *  - Without model groups, one block placed on the wrong side of the chip
     *    may create a window the size of the entire chip! This would rip up and
     *    spread all the blocks in the chip, which is very expensive.
     *  - This allows us to ignore block models which are already in legal
     *    positions.
     *
     *  @param group_id The dim group to identify windows for.
     *  @param utilization_prefix_sum Prefix sum of the bin utilization.
     */
    std::vector<SpreadingWindow> identify_non_overlapping_windows(PrimitiveGroupId group_id,
                                                                  const PerPrimitiveDimPrefixSum2D& utilization_prefix_sum);

    /**
     * @brief Identifies clusters of overfilled bins for the given model group.
     *
     * This locates clusters of overfilled bins which are within a given
     * distance from each other.
     */
    std::vector<FlatPlacementBinCluster> get_overfilled_bin_clusters(PrimitiveGroupId group_id);

    /**
     * @brief Creates and grows minimum spanning windows around the given
     *        overfilled bin clusters.
     *
     * Here, minimum means that the windows are just large enough such that the
     * capacity of the bins within the window is larger than the utilization for
     * the given model group.
     *
     *  @param overfilled_bin_clusters The clusters of overfilled bins to grow windows around.
     *  @param group_id The dim group the windows are grown for.
     *  @param utilization_prefix_sum Prefix sum of the bin utilization.
     */
    std::vector<SpreadingWindow> get_min_windows_around_clusters(
        const std::vector<FlatPlacementBinCluster>& overfilled_bin_clusters,
        PrimitiveGroupId group_id,
        const PerPrimitiveDimPrefixSum2D& utilization_prefix_sum);

    /**
     * @brief Merges overlapping windows in the given vector of windows.
     *
     * The resulting merged windows is stored in the given windows object.
     */
    void merge_overlapping_windows(std::vector<SpreadingWindow>& windows);

    /**
     * @brief Moves the blocks out of their bins and into their window.
     *
     * Only blocks in the given model group will be moved.
     */
    void move_blocks_into_windows(std::vector<SpreadingWindow>& non_overlapping_windows,
                                  PrimitiveGroupId group_id);

    // ========================================================================
    //      Spreading blocks over windows
    // ========================================================================

    /**
     * @brief Spread the blocks over each of the given non-overlapping windows.
     *
     * The partial placement solution from the solver is used to decide which
     * window partition to put a block into. The model group this window is
     * spreading over can make it more efficient to make decisions.
     */
    void spread_over_windows(std::vector<SpreadingWindow>& non_overlapping_windows,
                             const PartialPlacement& p_placement,
                             PrimitiveGroupId group_id);

    /**
     * @brief Recursively bisect the given window until every piece covers at
     *        most one tile, appending the finished windows to the segment in
     *        depth-first order together with their depth.
     *
     * When VPR is built with TBB, the upper half of a partition with enough
     * blocks is bisected as a task in the context's task group, writing to a
     * child segment, while the lower half continues on the calling thread.
     * The caller must wait on the task group before reading the segment.
     */
    void bisect_window(SpreadingWindow window,
                       unsigned depth,
                       BisectionSegment& segment,
                       WindowBisectionContext& context);

    /**
     * @brief Partition the given window into two sub-windows.
     *
     * We return extra information about how the window was created; for example,
     * the direction of the partition (vertical / horizontal) and the position
     * of the cut.
     */
    PartitionedWindow partition_window(SpreadingWindow& window, PrimitiveGroupId group_id);

    /**
     * @brief Partition the blocks in the given window into the partitioned
     *        windows.
     *
     * This is kept separate from splitting the physical window region for
     * cleanliness. After this point, the window will not have any atoms in
     * it.
     */
    void partition_blocks_in_window(SpreadingWindow& window,
                                    PartitionedWindow& partitioned_window,
                                    PrimitiveGroupId group_id,
                                    const PartialPlacement& p_placement);

    /**
     * @brief Move the blocks out of the given windows and put them back into
     *        the correct bin according to the window that contains them.
     */
    void move_blocks_out_of_windows(std::vector<SpreadingWindow>& finished_windows);

  private:
    /// @brief The density manager which manages the capacity and utilization
    ///        of regions of the device.
    std::shared_ptr<FlatPlacementDensityManager> density_manager_;

    /// @brief Grouper object which handles grouping together primitive dimensions
    ///        which must be spread together. Dims are grouped based on the model
    ///        pack patterns they represent and how those models are mapped into
    ///        primitive vector dimensions.
    PrimitiveDimGrouper dim_grouper_;

    /// @brief The prefix sum for the capacity of the device, as given by the
    ///        density manager. We will need to get the capacity of 2D regions
    ///        of the device very often for this partial legalizer. This data
    ///        structure greatly improves the time complexity of this operation.
    ///
    /// This is populated in the constructor and not modified.
    PerPrimitiveDimPrefixSum2D capacity_prefix_sum_;

    /// @brief The number of times a window was partitioned in the legalizer.
    unsigned num_windows_partitioned_ = 0;

    /// @brief The number of times a block was partitioned from one window into
    ///        another. This includes blocks which get partitioned multiple times.
    unsigned num_blocks_partitioned_ = 0;
};
