#pragma once

#include "place_delay_model.h"

/**
 * @class SeparableDelayModel
 * @brief A delay model that treats the x and y components of a connection's delay as independent.
 *
 * Like SimpleDelayModel, the delay information comes from the router lookahead rather than from
 * running the router. Unlike SimpleDelayModel, which stores a single delay indexed by (dx, dy),
 * this model keeps one table per axis and sums their contributions. Each table is indexed by the
 * absolute source and sink coordinate along its axis rather than by a delta, so architectures whose
 * routing is non-uniform along a row or column (e.g. a column of hard blocks that interrupts the
 * channels) are still represented.
 */
class SeparableDelayModel : public PlaceDelayModel {
  public:
    SeparableDelayModel() {}

    /// @brief Use the information in the router lookahead to fill the delay matrices instead of running the router
    void compute(RouterDelayProfiler& router,
                 const t_placer_opts& placer_opts,
                 const t_router_opts& router_opts,
                 int longest_length) override;

    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override;

    void dump_echo(std::string /*filepath*/) const override {}

    void read(const std::string& /*file*/) override;
    void write(const std::string& /*file*/) const override;

  private:
    /**
     * @brief The x component of the minimum delay between two points, per tile type and layer pair.
     *
     * As in SimpleDelayModel, the delay is stored per physical tile type and per (source layer, sink
     * layer) pair, since the connectivity of a physical type may differ between layers and the number
     * of inter-layer connections need not be uniform across layer pairs.
     */
    vtr::NdMatrix<float, 5> x_delays_; // [0..num_physical_type-1][0..num_layers-1][0..num_layers-1][0..max_x][0..max_x]

    /// @brief The y component of the minimum delay. See x_delays_.
    vtr::NdMatrix<float, 5> y_delays_; // [0..num_physical_type-1][0..num_layers-1][0..num_layers-1][0..max_y][0..max_y]
};
