#pragma once

#include "placer_state.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"

struct t_pl_blocks_to_be_move;

class PinDensityManager {
  public:
    explicit PinDensityManager(const PlacerState& placer_state);

    void find_affected_channels_and_update_costs(const t_pl_blocks_to_be_moved& blocks_affected);

    double compute_cost();

    void update_move_channels();

    void reset_move_channels();

  private:
    const PlacerState& placer_state_;

    vtr::Matrix<int> chanx_input_pin_count_;
    vtr::Matrix<int> chany_input_pin_count_;
    vtr::Matrix<int> chan_output_pin_count_;

    vtr::vector<ClusterPinId, t_physical_tile_loc> pin_locs_;
    vtr::vector<ClusterPinId, e_rr_type> pin_chan_type_;

    std::vector<std::tuple<ClusterPinId, t_physical_tile_loc, e_rr_type>> moved_pins_;

    vtr::vector<ClusterPinId, t_physical_tile_loc> ts_pin_locs_;
    vtr::vector<ClusterPinId, e_rr_type> ts_pin_chan_type_;

  private:
    std::pair<t_physical_tile_loc, e_rr_type> input_pin_loc_chan_type_(const t_physical_tile_loc& loc, e_side side) const;
};