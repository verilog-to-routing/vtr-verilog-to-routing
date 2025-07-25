#pragma once

#include "placer_state.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"

#include <variant>

struct t_pl_blocks_to_be_move;

class PinDensityManager {
  public:
    explicit PinDensityManager(const PlacerState& placer_state,
                               const t_placer_opts& placer_opts);


    double find_affected_channels_and_update_costs(const t_pl_blocks_to_be_moved& blocks_affected);

    double compute_cost();

    void update_move_channels();

    void reset_move_channels();

    void print() const;

  private:
    const t_placer_opts& placer_opts_;
    const PlacerState& placer_state_;
    bool pin_density_manager_started_;

    float chanx_unique_signals_per_chan_;
    float chany_unique_signals_per_chan_;

    vtr::Matrix<int> chanx_input_pin_count_;
    vtr::Matrix<int> chany_input_pin_count_;
    vtr::Matrix<int> chan_output_pin_count_;

    vtr::Matrix<int> ts_chanx_input_pin_count_;
    vtr::Matrix<int> ts_chany_input_pin_count_;
    vtr::Matrix<int> ts_chan_output_pin_count_;

    struct t_output_pin_sb_locs {
        t_physical_tile_loc sb0;
        t_physical_tile_loc sb1;
    };

    struct t_input_pin_adjacent_chan {
        t_physical_tile_loc loc;
        e_rr_type chan_type;
    };

    vtr::vector<ClusterPinId, std::variant<t_output_pin_sb_locs, t_input_pin_adjacent_chan>> pin_info_;
    vtr::vector<ClusterPinId, std::variant<t_output_pin_sb_locs, t_input_pin_adjacent_chan>> ts_pin_info_;

    struct PhysicalLocHasher {
        std::size_t operator()(const t_physical_tile_loc& loc) const {
            std::size_t seed = 0;
            vtr::hash_combine(seed, loc.x);
            vtr::hash_combine(seed, loc.y);
            vtr::hash_combine(seed, loc.layer_num);
            return seed;
        }

        std::size_t operator()(const std::pair<t_physical_tile_loc, e_rr_type>& chan_loc) const {
            std::size_t seed = operator()(chan_loc.first);
            vtr::hash_combine(seed, chan_loc.second);
            return seed;
        }
    };

    std::vector<std::tuple<ClusterPinId, std::variant<t_output_pin_sb_locs, t_input_pin_adjacent_chan> >> moved_pins_;
    std::unordered_set<t_physical_tile_loc, PhysicalLocHasher> affected_sb_locs_;

    std::unordered_set<std::pair<t_physical_tile_loc, e_rr_type>, PhysicalLocHasher> affected_chans_;

  private:
    t_input_pin_adjacent_chan input_pin_loc_chan_type_(const t_physical_tile_loc& loc, e_side side) const;
    t_output_pin_sb_locs output_pin_sb_locs_(const t_physical_tile_loc& loc, e_side side) const;

    void translate_affected_sb_locs_to_chans_();
};