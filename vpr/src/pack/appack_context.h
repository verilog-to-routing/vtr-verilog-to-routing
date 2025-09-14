#pragma once
/**
 * @file
 * @author  Alex Siner
 * @date    March 2025
 * @brief   Declaration of the APPack Context object which stores all the
 *          information used to configure APPack in the packer.
 */

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
    t_appack_options(const FlatPlacementInfo& flat_placement_info) {
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

    // =========== Candidate gain attenuation ============================== //
    // These terms are used to update the gain of a given candidate based on
    // its distance (d) relative to the location of the cluster being constructed.
    //      gain_new = attenuation * gain_original
    // We use the following gain attenuation function:
    //      attenuation = { 1 - (quad_fac * d)^2    if d < dist_th
    //                    { 1 / sqrt(d - sqrt_offset)  if d >= dist_th
    // The numbers below were empirically found to work well.

    // Distance threshold which decides when to use quadratic decay or inverted
    // sqrt decay. If the distance is less than this threshold, quadratic decay
    // is used. Inverted sqrt is used otherwise.
    static constexpr float dist_th = 2.0f;
    // Attenuation value at the threshold.
    static constexpr float attenuation_th = 0.25f;

    // Using the distance threshold and the attenuation value at that point, we
    // can compute the other two terms. This is to keep the attenuation function
    // smooth.
    // Horizontal offset to the inverted sqrt decay.
    static constexpr float sqrt_offset = dist_th - ((1.0f / attenuation_th) * (1.0f / attenuation_th));
    // Squared scaling factor for the quadratic decay term.
    static constexpr float quad_fac_sqr = (1.0f - attenuation_th) / (dist_th * dist_th);

    // TODO: Investigate adding flat placement info to seed selection.
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
                  const std::vector<t_logical_block_type> logical_block_types,
                  const DeviceGrid& device_grid)
        : appack_options(fplace_info)
        , flat_placement_info(fplace_info) {

        // If the flat placement info has been provided, calculate max distance
        // thresholds for all logical block types and the unrelated clustering
        // arguments.
        if (fplace_info.valid) {
            max_distance_threshold_manager.init(ap_opts.appack_max_dist_th,
                                                logical_block_types,
                                                device_grid);

            unrelated_clustering_manager.init(ap_opts.appack_unrelated_clustering_args,
                                              logical_block_types);
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

    // When selecting candidates, what distance from the cluster will we
    // consider? Any candidate beyond this distance will not be proposed.
    APPackMaxDistThManager max_distance_threshold_manager;

    // When performing unrelated clustering, the following manager class decides
    // how far we should search for unrelated candidates and how many attempts
    // we should perform.
    APPackUnrelatedClusteringManager unrelated_clustering_manager;
};
