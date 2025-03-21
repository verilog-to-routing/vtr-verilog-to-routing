/**
 * @file
 * @author  Alex Siner
 * @date    March 2025
 * @brief   Declaration of the APPack Context object which stores all the
 *          information used to configure APPack in the packer.
 */

#pragma once

#include <algorithm>
#include <limits>
#include "device_grid.h"
#include "flat_placement_types.h"
#include "vpr_context.h"

/**
 * @brief Configuration options for APPack.
 *
 * APPack is an upgrade to the AAPack algorithm which uses an atom-level placement
 * to inform the packer into creating better clusters. These options configure
 * how APPack interprets the flat placement information.
 */
struct t_appack_options {
    // Constructor for the appack options.
    t_appack_options(const FlatPlacementInfo& flat_placement_info,
                     const DeviceGrid& device_grid) {
        // If the flat placement info is valid, we want to use APPack.
        // TODO: Should probably check that all the information is valid here.
        use_appack = flat_placement_info.valid;

        // Set the max candidate distance as being some fraction of the longest
        // distance on the device (from the bottom corner to the top corner).
        // We also use an offset for the minimum this distance can be to prevent
        // small devices from finding candidates.
        float max_candidate_distance_scale = 0.5f;
        float max_candidate_distance_offset = 20.f;
        // Longest L1 distance on the device.
        float longest_distance = device_grid.width() + device_grid.height();
        max_candidate_distance = std::max(max_candidate_distance_scale * longest_distance,
                                          max_candidate_distance_offset);
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
    e_cl_loc_ty cluster_location_ty = e_cl_loc_ty::CENTROID;

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
    float dist_th = 5.0f;
    // Horizontal offset to the inverted sqrt decay.
    float sqrt_offset = -1.1f;
    // Scaling factor for the quadratic decay term.
    float quad_fac = 0.1543f;

    // =========== Candidate selection distance ============================ //
    // When selecting candidates, what distance from the cluster will we
    // consider? Any candidate beyond this distance will not be proposed.
    // This is set in the constructor.
    // TODO: It may be a good idea to have max different distances for different
    //       types of molecules / clusters. For example, CLBs vs DSPs
    float max_candidate_distance = std::numeric_limits<float>::max();

    // TODO: Investigate adding flat placement info to unrelated clustering.

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
    APPackContext(const FlatPlacementInfo& fplace_info, const DeviceGrid& device_grid)
        : appack_options(fplace_info, device_grid)
        , flat_placement_info(fplace_info) {}

    /**
     * @brief Options used to configure APPack.
     */
    t_appack_options appack_options;

    /**
     * @brief The flat placement information passed into APPack.
     */
    const FlatPlacementInfo& flat_placement_info;
};
