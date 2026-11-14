#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2026
 * @brief   Declaration of the candidate gain attenuation functions supported
 *          by APPack.
 *
 * This is kept in its own header to keep header includes cheap.
 */

/**
 * @brief The candidate gain attenuation function used by APPack to scale a
 *        candidate molecule's gain based on its distance from the cluster
 *        being formed.
 */
enum class e_appack_gain_attenuation_fn_type {
    NONE,           ///< No attenuation is applied (multiplier is always 1.0).
    QUAD_SQRT_KNEE, ///< Piecewise quadratic decay near the cluster, transitioning to inverted sqrt decay farther away.
    GAUSSIAN,       ///< Smooth Gaussian decay.
};
