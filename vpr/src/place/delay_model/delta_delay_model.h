
#pragma once

#include "place_delay_model.h"

/**
 * @class DeltaDelayModel
 *
 * @brief A simple delay model based on the distance (delta) between block locations.
 */
class DeltaDelayModel : public PlaceDelayModel {
  public:
    DeltaDelayModel(float min_cross_layer_delay,
                    bool is_flat)
        : cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}

    DeltaDelayModel(float min_cross_layer_delay,
                    vtr::NdMatrix<float, 4> delta_delays,
                    bool is_flat)
        : delays_(std::move(delta_delays))
        , cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}

    void compute(RouterDelayProfiler& router,
                 const t_placer_opts& placer_opts,
                 const t_router_opts& router_opts,
                 int longest_length) override;

    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override;

    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;

    const vtr::NdMatrix<float, 4>& delays() const {
        return delays_;
    }

  private:
    vtr::NdMatrix<float, 4> delays_; // [0..num_layers-1][0..max_dx][0..max_dy]
    float cross_layer_delay_;

    /// Indicates whether the router is a two-stage or run-flat
    bool is_flat_;
};