#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2026
 * @brief   Declaration of a class that computes the candidate gain
 *          attenuation multiplier used within APPack.
 */

#include <cmath>
#include "appack_gain_attenuation_fn_type.h"
#include "vtr_assert.h"

/**
 * @brief Manager class which computes the gain attenuation multipliers used
 *        by APPack to update the gain of a candidate molecule based on its
 *        distance (d) relative to the location of the cluster being
 *        constructed:
 *              gain_new = gain * get_gain_attenuation(d)
 *
 * The active distance-based attenuation function is selected by the
 * constructor argument.
 */
class APPackGainAttenuationManager {
  public:
    /**
     * @brief Constructor for the gain attenuation manager.
     *
     *  @param fn_type
     *      The distance-based gain attenuation function to use.
     *  @param inter_die_gain_multiplier
     *      Gain multiplier used when a candidate's flat placement location
     *      is on a different die than the cluster location.
     */
    APPackGainAttenuationManager(e_appack_gain_attenuation_fn_type fn_type,
                                 float inter_die_gain_multiplier)
        : fn_type_(fn_type)
        , inter_die_gain_multiplier_(inter_die_gain_multiplier) {}

    /**
     * @brief Computes the gain attenuation multiplier for a candidate at the
     *        given distance from the cluster being formed, using the
     *        currently selected attenuation function.
     *
     * The result is always within [0, 1].
     */
    inline float get_gain_attenuation(float dist) const {
        float gain_mult;
        switch (fn_type_) {
            case e_appack_gain_attenuation_fn_type::NONE:
                gain_mult = 1.0f;
                break;
            case e_appack_gain_attenuation_fn_type::QUAD_SQRT_KNEE:
                gain_mult = quad_sqrt_knee_attenuation_(dist);
                break;
            case e_appack_gain_attenuation_fn_type::GAUSSIAN:
                gain_mult = gaussian_attenuation_(dist);
                break;
            default:
                VTR_ASSERT_SAFE_MSG(false, "Unrecognized APPack gain attenuation function");
                gain_mult = 1.0f;
                break;
        }

        VTR_ASSERT_SAFE(gain_mult >= 0.0f && gain_mult <= 1.0f);
        return gain_mult;
    }

    /**
     * @brief Gets the gain multiplier to apply when a candidate's flat
     *        placement location is on a different die than the cluster
     *        location.
     */
    inline float get_inter_die_gain_multiplier() const {
        return inter_die_gain_multiplier_;
    }

  private:
    // =========== Quadratic / inverted sqrt knee =========================//
    // We use the following gain attenuation function:
    //      attenuation = { 1 - (quad_fac * d)^2        if d < dist_th
    //                    { 1 / sqrt(d - sqrt_offset)   if d >= dist_th
    // The numbers below were empirically found to work well.

    // Distance threshold which decides when to use quadratic decay or
    // inverted sqrt decay. If the distance is less than this threshold,
    // quadratic decay is used. Inverted sqrt is used otherwise.
    static constexpr float quad_sqrt_knee_dist_th_ = 2.0f;
    // Attenuation value at the threshold.
    static constexpr float quad_sqrt_knee_attenuation_th_ = 0.25f;

    // Using the distance threshold and the attenuation value at that point,
    // we can compute the other two terms. This is to keep the attenuation
    // function smooth.
    // Horizontal offset to the inverted sqrt decay.
    static constexpr float quad_sqrt_knee_sqrt_offset_ = quad_sqrt_knee_dist_th_ - ((1.0f / quad_sqrt_knee_attenuation_th_) * (1.0f / quad_sqrt_knee_attenuation_th_));
    // Squared scaling factor for the quadratic decay term.
    static constexpr float quad_sqrt_knee_quad_fac_sqr_ = (1.0f - quad_sqrt_knee_attenuation_th_) / (quad_sqrt_knee_dist_th_ * quad_sqrt_knee_dist_th_);

    /// @brief Computes the quadratic / inverted sqrt knee attenuation for the given distance.
    inline float quad_sqrt_knee_attenuation_(float dist) const {
        if (dist < quad_sqrt_knee_dist_th_) {
            return 1.0f - (quad_sqrt_knee_quad_fac_sqr_ * dist * dist);
        }
        return 1.0f / std::sqrt(dist - quad_sqrt_knee_sqrt_offset_);
    }

    // =========== Gaussian decay ==========================================//
    // Smooth Gaussian decay, proposed as an alternative to the piecewise
    // quad/sqrt knee above in:
    //   Qihang Wu, Taizun Jafri, Aman Arora, and Vidya A. Chhabria,
    //   "VPR-Evolve: Multi-Agent-Driven Algorithm Evolution for FPGA Place
    //   and Route," arXiv preprint (not yet published).
    //      attenuation = e^{-dist^2 / (2 * sigma^2)}

    // Standard deviation of the Gaussian decay curve.
    static constexpr float gaussian_sigma_ = 2.0f;

    /// @brief Computes the Gaussian attenuation for the given distance.
    inline float gaussian_attenuation_(float dist) const {
        return std::exp(-(dist * dist) / (2.0f * gaussian_sigma_ * gaussian_sigma_));
    }

    /// @brief The currently selected distance-based gain attenuation function.
    e_appack_gain_attenuation_fn_type fn_type_;

    /// @brief Gain multiplier used when a candidate's flat placement location
    ///        is on a different die than the cluster location.
    float inter_die_gain_multiplier_;
};
