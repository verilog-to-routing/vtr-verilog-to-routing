#pragma once

#include "tatum/Time.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

#include "atom_netlist.h"
#include "routing_netlist.h"
#include "vpr_net_pins_matrix.h"

#include "atom_delay_calc.h"
#include "clb_delay_calc.h"

class VprTimingGraphResolver; //Forward declaration

class PostClusterDelayCalculator : public tatum::DelayCalculator {
  public:
    /**
     * @brief Builds a delay calculator which uses the routing delays of the connections
     *        of the clustered netlist (used during placement).
     *
     * @param net_delay_min Min routing delay of each net connection, used for hold analysis.
     * @param net_delay_max Max routing delay of each net connection, used for setup analysis.
     *                      Callers without separate min delays (e.g. placement) pass the
     *                      same matrix for both.
     */
    PostClusterDelayCalculator(const AtomNetlist& netlist,
                               const AtomLookup& netlist_lookup,
                               const ClbNetPinsMatrix<float>& net_delay_min,
                               const ClbNetPinsMatrix<float>& net_delay_max);

    /**
     * @brief Builds a delay calculator which uses the routing delays of the connections
     *        of a routing netlist (used during and after routing).
     *
     * @param net_list      The netlist used by routing. It must outlive the delay calculator.
     * @param net_delay_min Min routing delay of each net connection, used for hold analysis.
     * @param net_delay_max Max routing delay of each net connection, used for setup analysis.
     */
    PostClusterDelayCalculator(const AtomNetlist& netlist,
                               const AtomLookup& netlist_lookup,
                               const RoutingNetlist& net_list,
                               const NetPinsMatrix<float>& net_delay_min,
                               const NetPinsMatrix<float>& net_delay_max);

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;
    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;
    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override;

    void clear_cache();

    void set_tsu_margin_relative(float val);
    void set_tsu_margin_absolute(float val);

    /// @brief Returns true if the routing delays are those of a flat routing (the connections of the atom nets are routed)
    bool is_flat() const { return is_flat_; }

  private:
    friend VprTimingGraphResolver;

    /// @brief Sets up the parts of the delay calculator which do not depend on where the routing delays come from
    PostClusterDelayCalculator(const AtomNetlist& netlist,
                               const AtomLookup& netlist_lookup,
                               bool is_flat);

    //Returns the generic edge delay
    tatum::Time calc_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;

    tatum::Time atom_combinational_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;
    tatum::Time atom_clock_to_q_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;
    tatum::Time atom_net_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id, DelayType delay_type) const;

    tatum::Time atom_setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;
    tatum::Time atom_hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const;

    /// @brief Returns the routing delay of a connection of a clustered net (the routing is not flat)
    float inter_cluster_delay(ClusterNetId clb_net_id, const int driver_net_pin_index, const int sink_net_pin_index, DelayType delay_type) const;

    /// @brief Returns the routing delay of a connection of an atom net (the routing is flat)
    float inter_cluster_delay(AtomNetId atom_net_id, const int driver_net_pin_index, const int sink_net_pin_index, DelayType delay_type) const;

    //Returns the delay type of the net delay used for an edge of the given delay type
    DelayType routing_delay_type(DelayType delay_type) const;

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

    // The cached driver and sink pins of the clustered net which carries an edge between two
    // clusters (invalid if the edge is inside a cluster or has not been cached yet). The pins
    // are not cached with flat routing, where the edges are the connections of the atom nets.
    std::pair<ClusterPinId, ClusterPinId> get_cached_pins(tatum::EdgeId edge, DelayType delay_type) const;
    void set_cached_pins(tatum::EdgeId edge, DelayType delay_type, ClusterPinId src_pin, ClusterPinId sink_pin) const;

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;

    // The routing netlist whose connection delays are used (nullptr if the delays of the
    // connections of the clustered netlist are used directly)
    const RoutingNetlist* net_list_ = nullptr;

    // The routing delays of the connections of the clustered netlist (used if net_list_ is nullptr)
    const ClbNetPinsMatrix<float>* clb_net_delay_min_ = nullptr;
    const ClbNetPinsMatrix<float>* clb_net_delay_max_ = nullptr;

    // The routing delays of the connections of the routing netlist (used if net_list_ is not nullptr)
    const NetPinsMatrix<float>* net_delay_min_ = nullptr;
    const NetPinsMatrix<float>* net_delay_max_ = nullptr;

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
    bool is_flat_;
};

#include "PostClusterDelayCalculator.tpp"
