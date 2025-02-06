
#pragma once

#include "place_delay_model.h"

/**
 * @class SimpleDelayModel
 * @brief A simple delay model based on the information stored in router lookahead
 * This is in contrast to other placement delay models that get the cost of getting from one location to another by running the router
 */
class SimpleDelayModel : public PlaceDelayModel {
  public:
    SimpleDelayModel() {}

    /// @brief Use the information in the router lookahead to fill the delay matrix instead of running the router
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
     * @brief The matrix to store the minimum delay between different points on different layers.
     *
     *The matrix used to store delay information is a 5D matrix. This data structure stores the minimum delay for each tile type on each layer to other layers
     *for each dx and dy. We decided to separate the delay for each physical type on each die to accommodate cases where the connectivity of a physical type differs
     *on each layer. Additionally, instead of using d_layer, we distinguish between the destination layer to handle scenarios where connectivity between layers
     *is not uniform. For example, if the number of inter-layer connections between layer 1 and 2 differs from the number of connections between layer 0 and 1.
     *One might argue that this variability could also occur for dx and dy. However, we are operating under the assumption that the FPGA fabric architecture is regular.
     */
    vtr::NdMatrix<float, 5> delays_; // [0..num_physical_type-1][0..num_layers-1][0..num_layers-1][0..max_dx][0..max_dy]
};