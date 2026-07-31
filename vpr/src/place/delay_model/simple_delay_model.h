#pragma once

#include <optional>
#include "place_delay_model.h"
#include "router_lookahead.h"
#include "router_lookahead_interposer.h"

/**
 * @class SimpleDelayModel
 * @brief A simple delay model based on the information stored in router lookahead
 * This is in contrast to other placement delay models that get the cost of getting from one location to another by running the router
 */
class SimpleDelayModel final : public PlaceDelayModel {
  public:
    explicit SimpleDelayModel(const RouterLookahead& router_lookahead)
        : router_lookahead_(router_lookahead) {}

    /// @brief Set up any auxiliary data needed to query the router lookahead (e.g. interposer delay information)
    void compute(RouterDelayProfiler& router,
                 const t_placer_opts& placer_opts,
                 const t_router_opts& router_opts,
                 int longest_length) override;

    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override;

    void dump_echo(std::string /*filepath*/) const override {}

    void read(const std::string& /*file*/) override;
    void write(const std::string& /*file*/) const override;

  private:
    /// @brief The router lookahead queried for the minimum delay between locations.
    const RouterLookahead& router_lookahead_;

    /// @brief Contains delay information of crossing a die, used in 2.5D architectures.
    std::optional<InterposerLookahead> interposer_lookahead_;
};
