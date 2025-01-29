
#pragma once


#pragma once

#include "placer.h"

#include <memory>
#include <barrier>
#include <functional>
#include <atomic>

class FlatPlacementInfo;

class MultiPlacer {
  public:   // Constructor
    MultiPlacer(const Netlist<>& net_list,
                const t_placer_opts& placer_opts,
                const t_analysis_opts& analysis_opts,
                const t_noc_opts& noc_opts,
                const IntraLbPbPinLookup& pb_gpin_lookup,
                const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                const std::vector<t_direct_inf>& directs,
                const FlatPlacementInfo& flat_placement_info,
                std::shared_ptr<PlaceDelayModel> place_delay_model,
                bool cube_bb,
                bool is_flat);

  public:   // Mutator
    void place();
    void copy_locs_to_global_state(PlacementContext& place_ctx);

  private:

    int num_annealers_;
    std::vector<t_placer_opts> placer_opts_;
    std::vector<std::unique_ptr<Placer>> placers_;
    std::barrier<> stop_condition_barrier_;
    std::atomic<bool> stop_flag_;
};