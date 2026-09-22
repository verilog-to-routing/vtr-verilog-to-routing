#pragma once
/**
 * @file
 * @author  Alex Siner
 * @date    March 2025
 * @brief   Declaration of the APPack Context object which stores all the
 *          information used to configure APPack in the packer.
 */

#include <cstddef>
#include <map>
#include <vector>
#include "appack_gain_attenuation_manager.h"
#include "appack_max_dist_th_manager.h"
#include "appack_unrelated_clustering_manager.h"
#include "device_grid.h"
#include "flat_placement_types.h"
#include "physical_types.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vpr_utils.h"

/**
 * @brief Configuration options for APPack.
 *
 * APPack is an upgrade to the AAPack algorithm which uses an atom-level placement
 * to inform the packer into creating better clusters. These options configure
 * how APPack interprets the flat placement information.
 */
struct t_appack_options {
    // Constructor for the appack options.
    explicit t_appack_options(const FlatPlacementInfo& flat_placement_info) {
        // If the flat placement info is valid, we want to use APPack.
        // TODO: Should probably check that all the information is valid here.
        use_appack = flat_placement_info.valid;
    }

    // Whether to use APPack or not.
    // This is initialized in the constructor based on if the flat placement
    // info is valid or not.
    bool use_appack = false;

    // =========== Cluster location ======================================== //
    // What is the location of the cluster being created relative to the
    // molecules being packed into it.
    enum class e_cl_loc_ty {
        CENTROID, /**< The location of the cluster is the centroid of the molecules which have been packed into it. */
        SEED      /**< The location of the cluster is the location of the first molecule packed into it. */
    };
    static constexpr e_cl_loc_ty cluster_location_ty = e_cl_loc_ty::CENTROID;

    // TODO: Investigate adding flat placement info to seed selection.
};

/**
 * @brief Result of APPackContext::adjust_for_device_size_estimate.
 *
 * When adjusting the parameters of APPack according to the device size, we
 * sometimes want to change the overall packing algorithm. This cannot be done
 * by a method in this class, so we need to return the actions the packer needs
 * to take.
 */
struct t_appack_device_size_adjustment {
    /// @brief Whether unrelated clustering should be enabled globally (for all
    ///        block types) from the start of packing.
    bool allow_unrelated_clustering = false;
};

/**
 * @brief State relating to APPack.
 *
 * This class is intended to contain information on using flat placement
 * information in packing.
 */
struct APPackContext : public Context {
    /**
     * @brief Constructor for the APPack context.
     */
    APPackContext(const FlatPlacementInfo& fplace_info,
                  const t_ap_opts& ap_opts,
                  const std::vector<t_logical_block_type>& logical_block_types,
                  const DeviceGrid& device_grid)
        : appack_options(fplace_info)
        , flat_placement_info(fplace_info)
        , gain_attenuation_manager(ap_opts.appack_gain_attenuation_fn,
                                    ap_opts.appack_inter_die_gain_multiplier) {

        // If the flat placement info has been provided, calculate max distance
        // thresholds for all logical block types and the unrelated clustering
        // arguments.
        if (fplace_info.valid) {
            max_distance_threshold_manager.init(ap_opts.appack_max_dist_th,
                                                logical_block_types,
                                                device_grid);

            unrelated_clustering_manager.init(ap_opts.appack_unrelated_clustering_args,
                                              logical_block_types,
                                              device_grid);
        }
    }

    /**
     * @brief Options used to configure APPack.
     */
    t_appack_options appack_options;

    /**
     * @brief The flat placement information passed into APPack.
     */
    const FlatPlacementInfo& flat_placement_info;

    // When calculating the gain of candidate primitives to pack into the wip
    // cluster, primitives which are farther from the centroid of the cluster
    // are penalized. This manager class computes how much those primitives
    // should be penalized as a function of their distance.
    APPackGainAttenuationManager gain_attenuation_manager;

    // When selecting candidates, what distance from the cluster will we
    // consider? Any candidate beyond this distance will not be proposed.
    APPackMaxDistThManager max_distance_threshold_manager;

    // When performing unrelated clustering, the following manager class decides
    // how far we should search for unrelated candidates and how many attempts
    // we should perform.
    APPackUnrelatedClusteringManager unrelated_clustering_manager;

    // ============ Device size estimate reaction ========================== //
    // Tuning constants for adjust_for_device_size_estimate. "Utilization" here
    // means a block type's estimated instance count (from the pre-packing
    // device size estimate) divided by the number of instances available on
    // the device.

    /// @brief Minimum estimated utilization of a block type before its max
    ///        candidate distance threshold is widened.
    static constexpr float device_size_min_utilization_for_th_bump = 0.5f;

    /// @brief Largest multiplier applied to a block type's max candidate
    ///        distance threshold. Reached once the estimated utilization is at
    ///        (or above) device_size_severe_utilization_cutoff.
    static constexpr float device_size_max_dist_th_scale_multiplier = 10.0f;

    /// @brief Estimated utilization at (or above) which a block type is
    ///        considered severely over capacity.
    static constexpr float device_size_severe_utilization_cutoff = 1.5f;

    /**
     * @brief Adjusts the APPack parameters according to how dense the device is
     *        expected to be.
     *
     *  @param estimated_type_instance_counts
     *      Estimated number of instances of each logical block type needed by
     *      the netlist, computed before packing.
     *  @param logical_block_types
     *      All logical block types in the architecture.
     *  @param device_grid
     *      The device grid, used to count available instances of each type.
     */
    void adjust_for_device_size_estimate(
        const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts,
        const std::vector<t_logical_block_type>& logical_block_types,
        const DeviceGrid& device_grid);
};
