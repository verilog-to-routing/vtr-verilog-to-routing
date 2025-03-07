/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Enumerations used by the Analytical Placement Flow.
 */

#pragma once

/**
 * @brief The type of a Full Legalizer.
 *
 * The Analytical Placement flow may implement different Full Legalizers. This
 * enum can select between these different Full Legalizers.
 */
enum class e_ap_full_legalizer {
    Naive,  ///< The Naive Full Legalizer, which clusters atoms placed in the same tile and tries to place them in that tile according to the flat placement.
    APPack,  ///< The APPack Full Legalizer, which uses the flat placement to improve the Packer and Placer.
    Basic_Min_Disturbance ///< The Basic Min. Disturbance Full Legalizer, which tries to reconstruct a clustered placement as close to the incoming flat placement as it can.
};

