#pragma once
/*Defines the route_budgets class that contains the minimum, maximum,
 * target, upper bound, and lower bound budgets. These information are
 * used by the router to optimize for hold time. */

#include <vector>
#include <queue>
#include "RoutingDelayCalculator.h"
#include "clustered_netlist_utils.h"
#include "timing_info.h"

enum analysis_type {
    SETUP,
    HOLD
};

enum slack_allocated_type {
    POSITIVE,
    NEGATIVE,
    BOTH
};

/*Scope over which set_low_skew_clock_budgets() computes its target delay.
 * PER_CLOCK_DOMAIN (default) minimizes skew within each clock domain independently.
 * GLOBAL minimizes skew across all clock domains combined, using a single delay target
 * shared by every clock connection in the design.
 * TODO: expose this as a command-line option once there's demand for GLOBAL.*/
enum class e_low_skew_clock_target_scope {
    PER_CLOCK_DOMAIN,
    GLOBAL
};

#define UNINITIALIZED_PATH_DELAY (-2)
class route_budgets {
  public:
    route_budgets(const Netlist<>& net_list, bool is_flat);

    route_budgets(std::vector<std::vector<float>> net_delay);

    virtual ~route_budgets();

    /*getter functions*/
    float get_delay_target(ParentNetId net_id, int ipin);
    float get_min_delay_budget(ParentNetId net_id, int ipin);
    float get_max_delay_budget(ParentNetId net_id, int ipin);
    float get_crit_short_path(ParentNetId net_id, int ipin);
    bool if_set() const;

    /*main loader function*/
    void load_route_budgets(NetPinsMatrix<float>& net_delay,
                            std::shared_ptr<SetupTimingInfo> timing_info,
                            const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                            const t_router_opts& router_opts);

    /*debugging tools*/
    void print_route_budget(std::string filename, NetPinsMatrix<float>& net_delay);

    /*lower budgets during congestion*/
    void update_congestion_times(ParentNetId net_id);
    bool increase_min_budgets_if_struggling(float delay_decrement, std::shared_ptr<SetupHoldTimingInfo> timing_info, float worst_neg_slack, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
    void increase_short_crit(ParentNetId net_id, float delay_decs);
    void not_congested_this_iteration(ParentNetId net_id);

    bool get_should_reroute(ParentNetId net_id);
    void set_should_reroute(ParentNetId net_id, bool value);
    int get_hold_fac(ParentNetId net_id);
    void set_hold_fac(ParentNetId net_id, int value);

    /*One-shot reroute request, independent of get/set_should_reroute's hold-slack gating.
     * Used to force clock connections through the router once their skew budgets are (re)computed,
     * since otherwise an already-legally-routed, non-critical net is never revisited. */
    bool get_should_reroute_for_skew(ParentNetId net_id);
    void set_should_reroute_for_skew(ParentNetId net_id, bool value);

  private:
    /*For allocating and freeing memory*/
    void free_budgets();
    void alloc_budget_memory();
    void load_initial_budgets();

    /*different ways to set route budgets*/
    void allocate_slack_using_delays_and_criticalities(NetPinsMatrix<float>& net_delay,
                                                       std::shared_ptr<SetupTimingInfo> timing_info,
                                                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                       const t_router_opts& router_opts);
    void allocate_slack_using_weights(NetPinsMatrix<float>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, bool negative_hold_slack);
    /*Sets the target (and min/max) delay of every clock connection to the maximum observed
     * clock delay, to encourage the router to equalize clock delays and reduce skew. All
     * other connections are left at their initial (unconstrained) budgets.*/
    void set_low_skew_clock_budgets(NetPinsMatrix<float>& net_delay);
    /*Sometimes want to allocate only positive or negative slack.
     * By default, allocate both*/
    float minimax_PERT(std::shared_ptr<SetupHoldTimingInfo> orig_timing_info,
                       std::shared_ptr<SetupHoldTimingInfo> timing_info,
                       NetPinsMatrix<float>& temp_budgets,
                       NetPinsMatrix<float>& net_delay,
                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                       analysis_type analysis_type,
                       bool keep_in_bounds,
                       slack_allocated_type slack_type = BOTH);

    void process_negative_slack_using_minimax(NetPinsMatrix<float>& net_delay, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);

    /*Perform static timing analysis*/
    std::shared_ptr<SetupHoldTimingInfo> perform_sta(NetPinsMatrix<float>& temp_budgets);

    /*checks*/
    void keep_budget_in_bounds(NetPinsMatrix<float>& temp_budgets);
    void keep_budget_in_bounds(NetPinsMatrix<float>& temp_budgets, ParentNetId net_id, ParentPinId ipin);
    void keep_min_below_max_budget();
    void check_if_budgets_in_bounds();
    void check_if_budgets_in_bounds(ParentNetId net_id, ParentPinId pin_id);
    void keep_budget_above_value(NetPinsMatrix<float>& temp_budgets, float bottom_range);

    /*helper functions*/
    float calculate_clb_pin_slack(ParentNetId net_id,
                                  int ipin,
                                  std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                  const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                  analysis_type type,
                                  AtomPinId& atom_pin);

    float get_total_path_delay(std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> timing_analyzer,
                               analysis_type analysis_type,
                               ParentNetId net_id,
                               int ipin,
                               AtomPinId& atom_pin);
    void set_min_max_budgets_equal();
    std::shared_ptr<RoutingDelayCalculator> get_routing_calc(NetPinsMatrix<float>& net_delay);
    void calculate_delay_targets();
    void calculate_delay_targets(ParentNetId net_id, ParentPinId pin_id);
    tatum::EdgeId get_edge_from_nets(ParentNetId net_id, int ipin);

    /*debugging tools*/
    void print_temporary_budgets_to_file(NetPinsMatrix<float>& temp_budgets) const;

    /*Budget variables*/
    NetPinsMatrix<float> delay_min_budget;  //[0..num_nets][0..clb_net[inet].pins]
    NetPinsMatrix<float> delay_max_budget;  //[0..num_nets][0..clb_net[inet].pins]
    NetPinsMatrix<float> delay_target;      //[0..num_nets][0..clb_net[inet].pins]
    NetPinsMatrix<float> delay_lower_bound; //[0..num_nets][0..clb_net[inet].pins]
    NetPinsMatrix<float> delay_upper_bound; //[0..num_nets][0..clb_net[inet].pins]
    NetPinsMatrix<float> short_path_crit;   //[0..num_nets][0..clb_net[inet].pins]

    NetPinsMatrix<float> total_path_delays_hold;
    NetPinsMatrix<float> total_path_delays_setup;

    /*used to keep count the number of continuous time this node was congested*/
    vtr::vector<ParentNetId, int> num_times_congested; //[0..num_nets]
    std::queue<float> negative_hold_slacks;

    const Netlist<>& net_list_;
    bool is_flat_;

    /*budgets only valid when loaded*/
    bool set;

    /*flag to reroute each net for hold violation*/
    std::map<ParentNetId, bool> should_reroute_for_hold;
    std::map<ParentNetId, int> hold_fac;

    /*flag to force a one-shot reroute of a net after its skew budgets are (re)computed*/
    std::map<ParentNetId, bool> should_reroute_for_skew;

    /*see e_low_skew_clock_target_scope*/
    e_low_skew_clock_target_scope low_skew_clock_target_scope_ = e_low_skew_clock_target_scope::PER_CLOCK_DOMAIN;
};
