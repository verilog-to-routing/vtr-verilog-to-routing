
#pragma once

#include "placer.h"

#include <barrier>

class ParallelTemperer {
  public:   // Constructor
    ParallelTemperer(int num_parallel_annealers,
                     const Netlist<>& net_list,
                     const t_placer_opts& placer_opts,
                     const t_analysis_opts& analysis_opts,
                     const t_noc_opts& noc_opts,
                     const IntraLbPbPinLookup& pb_gpin_lookup,
                     const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                     const std::vector<t_direct_inf>& directs,
                     std::shared_ptr<PlaceDelayModel> place_delay_model,
                     bool cube_bb,
                     bool is_flat);

  public:   // Mutator

  private:

    void run_thread_(int worker_id);
    void try_annealer_swap();

    int num_annealers_;
    std::vector<t_placer_opts> placer_opts_;
    std::vector<std::unique_ptr<Placer>> placers_;
    vtr::RngContainer rng_;
};