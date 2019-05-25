#ifndef POST_CLUSTER_DELAY_CALCULATOR_H
#define POST_CLUSTER_DELAY_CALCULATOR_H
#include "vtr_linear_map.h"

#include "tatum/Time.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "vpr_utils.h"

#include "atom_delay_calc.h"
#include "clb_delay_calc.h"

class VprTimingGraphResolver; //Forward declaration

class PostClusterDelayCalculator : public tatum::DelayCalculator {
  public:
    PostClusterDelayCalculator(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, vtr::vector<ClusterNetId, float*>& net_delay);

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;
    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;
    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;

    void clear_cache();

    void set_tsu_margin_relative(float val);
    void set_tsu_margin_absolute(float val);

  private:
    friend VprTimingGraphResolver;

    //Returns the generic edge delay
    tatum::Time calc_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;

    tatum::Time atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;
    tatum::Time atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;
    tatum::Time atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;

    tatum::Time atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    float inter_cluster_delay(ClusterNetId net_id, const int driver_net_pin_index, const int sink_net_pin_index) const;

    tatum::Time get_cached_delay(tatum::EdgeId edge, DelayType delay_type) const;
    void set_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const;

    tatum::Time get_driver_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type) const;
    void set_driver_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const;

    tatum::Time get_sink_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type) const;
    void set_sink_clb_cached_delay(tatum::EdgeId edge, DelayType delay_type, tatum::Time delay) const;

    tatum::Time get_cached_setup_time(tatum::EdgeId edge) const;
    void set_cached_setup_time(tatum::EdgeId edge, tatum::Time setup) const;

    tatum::Time get_cached_hold_time(tatum::EdgeId edge) const;
    void set_cached_hold_time(tatum::EdgeId edge, tatum::Time hold) const;

    std::pair<ClusterPinId, ClusterPinId> get_cached_pins(tatum::EdgeId edge, DelayType delay_type) const;
    void set_cached_pins(tatum::EdgeId edge, DelayType delay_type, ClusterPinId src_pin, ClusterPinId sink_pin) const;

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
    vtr::vector<ClusterNetId, float*> net_delay_;

    ClbDelayCalc clb_delay_calc_;
    AtomDelayCalc atom_delay_calc_;

    float tsu_margin_rel_ = 1.0;
    float tsu_margin_abs_ = 0.0e-12;

    mutable vtr::vector<tatum::EdgeId, tatum::Time> edge_min_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, tatum::Time> edge_max_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, tatum::Time> driver_clb_min_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, tatum::Time> driver_clb_max_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, tatum::Time> sink_clb_min_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, tatum::Time> sink_clb_max_delay_cache_;
    mutable vtr::vector<tatum::EdgeId, std::pair<ClusterPinId, ClusterPinId>> pin_cache_min_;
    mutable vtr::vector<tatum::EdgeId, std::pair<ClusterPinId, ClusterPinId>> pin_cache_max_;
};

#include "PostClusterDelayCalculator.tpp"
#endif
