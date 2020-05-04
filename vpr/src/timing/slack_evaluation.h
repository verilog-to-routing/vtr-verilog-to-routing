#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <map>
#include "atom_netlist_fwd.h"
#include "DomainPair.h"
#include "tatum/timing_analyzers.hpp"
#include "vtr_vector.h"

//Incrementally update slack and criticality?
#define INCR_UPDATE_SLACK
#define INCR_UPDATE_CRIT


class SetupSlackCrit {
  public: //Constructors
    SetupSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup);
    ~SetupSlackCrit();
  public: //Types
    typedef std::vector<AtomPinId>::const_iterator modified_pin_iterator;

    typedef vtr::Range<modified_pin_iterator> modified_pin_range;

  public: //Accessors
    //Returns the worst (least) slack of connections through the specified pin
    float setup_pin_slack(AtomPinId pin) const;

    //Returns the worst (maximum) criticality of connections through the specified pin.
    //  Criticality (in [0., 1.]) represents how timing-critical something is,
    //  0. is non-critical and 1. is most-critical.
    float setup_pin_criticality(AtomPinId pin) const;

    modified_pin_range pins_with_modified_slack() const;
    modified_pin_range pins_with_modified_criticality() const;

  public: //Mutators
    void update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

  private: //Implementation
    void update_slacks(const tatum::SetupTimingAnalyzer& analyzer);

    void update_pin_slack(const tatum::NodeId node, const tatum::SetupTimingAnalyzer& analyzer);

    void update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

    void update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                        const tatum::SetupTimingAnalyzer& analyzer,
                                        std::map<DomainPair, float>& max_req,
                                        std::map<DomainPair, float>& worst_slack);

    float calc_pin_criticality(const tatum::NodeId node,
                               const tatum::SetupTimingAnalyzer& analyzer,
                               const std::map<DomainPair, float>& max_req,
                               const std::map<DomainPair, float>& worst_slack);

  private: //Data
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;

    vtr::vector<AtomPinId, float> pin_slacks_;
    vtr::vector<AtomPinId, float> pin_criticalities_;

    std::vector<AtomPinId> pins_with_modified_slacks_;
    std::vector<AtomPinId> pins_with_modified_criticalities_;


    std::map<DomainPair, float> prev_max_req_;
    std::map<DomainPair, float> prev_worst_slack_;

#if !defined(INCR_SLACK_UPDATE) || !defined(INCR_UPDATE_CRIT)
    std::vector<tatum::NodeId> all_nodes_;
#endif

    size_t incr_slack_updates_ = 0;
    float incr_slack_update_time_sec_ = 0.;

    size_t max_req_worst_slack_updates_ = 0;
    float max_req_worst_slack_update_time_sec_ = 0.;

    size_t incr_criticality_updates_ = 0;
    float incr_criticality_update_time_sec_ = 0.;

    size_t full_criticality_updates_ = 0;
    float full_criticality_update_time_sec_ = 0.;
};

//TODO: implement a HoldSlackCrit class for hold analysis

class HoldSlackCrit {
  public: //Constructors
    HoldSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup);

  public: //Accessors
    //Returns the worst (least) slack of connections through the specified pin
    float hold_pin_slack(AtomPinId pin) const;

    //Returns the worst (maximum) criticality of connections through the specified pin.
    //  Criticality (in [0., 1.]) represents how timing-critical something is,
    //  0. is non-critical and 1. is most-critical.
    float hold_pin_criticality(AtomPinId pin) const;

  public: //Mutators
    void update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& analyzer);

  private: //Implementation
    void update_slacks(const tatum::HoldTimingAnalyzer& analyzer);

    void update_pin_slack(const AtomPinId pin, const tatum::HoldTimingAnalyzer& analyzer);

    void update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& analyzer);

    float calc_pin_criticality(AtomPinId pin,
                               const tatum::HoldTimingAnalyzer& analyzer,
                               const float scale,
                               const float shift);

  private: //Data
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;

    vtr::vector<AtomPinId, float> pin_slacks_;
    vtr::vector<AtomPinId, float> pin_criticalities_;
};

#endif
