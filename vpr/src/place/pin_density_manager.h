#pragma once

#include "placer_state.h"
#include "vtr_ndmatrix.h"

class PinDensityManager {
  public:
    explicit PinDensityManager(const PlacerState& placer_state);

    void find_affected_channels_and_update_costs();

    double compute_cost();

    void update_move_channels();

    void reset_move_channels();

  private:
    const PlacerState& placer_state_;

    vtr::Matrix<int> chanx_input_pin_count_;
    vtr::Matrix<int> chany_input_pin_count_;
    vtr::Matrix<int> chanx_output_pin_count_;
    vtr::Matrix<int> chany_output_pin_count_;
};