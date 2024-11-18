

#ifndef VTR_PLACER_H
#define VTR_PLACER_H

#include <memory>
#include <optional>

#include "timing_place.h"
#include "place_checkpoint.h"
#include "PlacementDelayCalculator.h"
#include "placer_state.h"
#include "noc_place_utils.h"
#include "net_cost_handler.h"
#include "place_log_util.h"

class PlacementAnnealer;
namespace vtr{
class ScopedStartFinishTimer;
}

class Placer {
  public:
    Placer(const Netlist<>& net_list,
           const t_placer_opts& placer_opts,
           const t_analysis_opts& analysis_opts,
           const t_noc_opts& noc_opts,
           const std::vector<t_direct_inf>& directs,
           std::shared_ptr<PlaceDelayModel> place_delay_model,
           bool cube_bb);

    void place();

    /**
     * @brief Copies the placement location variables into the global placement context.
     */
    void copy_locs_to_global_state();

    /*
     * Getters
     */
    const PlacementAnnealer& annealer() const;

    const t_placer_opts& placer_opts() const;

    const t_noc_opts& noc_opts() const;

    const t_placer_costs& costs() const;

    const tatum::TimingPathInfo& critical_path() const;

    std::shared_ptr<const SetupTimingInfo> timing_info() const;

    const PlacerState& placer_state() const;

    const std::optional<NocCostHandler>& noc_cost_handler() const;

    //TODO: make this private
  private:
    const t_placer_opts& placer_opts_;
    const t_analysis_opts& analysis_opts_;
    const t_noc_opts& noc_opts_;
    t_placer_costs costs_;
    PlacerState placer_state_;
    vtr::RngContainer rng_;
    NetCostHandler net_cost_handler_;
    std::optional<NocCostHandler> noc_cost_handler_;
    std::shared_ptr<PlaceDelayModel> place_delay_model_;

    t_placement_checkpoint placement_checkpoint_;

    std::shared_ptr<SetupTimingInfo> timing_info_;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc_;
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks_;
    std::unique_ptr<PlacerCriticalities> placer_criticalities_;
    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator_;
    tatum::TimingPathInfo critical_path_;


    std::unique_ptr<vtr::ScopedStartFinishTimer> timer_;

    IntraLbPbPinLookup pb_gpin_lookup_;
    ClusteredPinAtomPinsLookup netlist_pin_lookup_;

    std::unique_ptr<PlacementAnnealer> annealer_;

    PlacementLogPrinter log_printer_;

  private:
    void alloc_and_init_timing_objects_(const Netlist<>& net_list,
                                        const t_analysis_opts& analysis_opts);

    /**
     * Checks that the placement has not confused our data structures.
     * i.e. the clb and block structures agree about the locations of
     * every block, blocks are in legal spots, etc.  Also recomputes
     * the final placement cost from scratch and makes sure it is
     * within round-off of what we think the cost is.
     */
    void check_place_();

    int check_placement_costs_();

    void print_initial_placement_stats_();

    void print_post_placement_stats_();
};

#endif //VTR_PLACER_H
