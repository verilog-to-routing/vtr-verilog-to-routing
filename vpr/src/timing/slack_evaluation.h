#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <map>
#include "atom_netlist_fwd.h"
#include "DomainPair.h"
#include "tatum/timing_analyzers.hpp"
#include "vtr_vector.h"

/*
 * SetupSlackCrit converts raw timing analysis results (i.e. timing tags associated with
 * tatum::NodeIds calculated by the timign analyzer), to the shifted slacks and relaxed
 * criticalities associated with atom netlist connections (i.e. associated withAtomPinIds).
 *
 * For efficiency, when update_slacks_and_criticalities() is called it attempts to incrementally
 * update the shifted slacks and relaxed criticalities based on the set of timing graph nodes
 * which are reported as having been modified by the previous timing analysis.
 */
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

    //Returns the set of pins which have respectively had their slack or criticality modified
    //by the last call to update_slacks_and_criticalities()
    modified_pin_range pins_with_modified_slack() const;
    modified_pin_range pins_with_modified_criticality() const;

  public: //Mutators
    //Incrementally updates all atom netlist connection (i.e. AtomPinId) slacks and criticalities,
    //based on the last timing analysis performed by analyzer.
    void update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

  private: //Implementation
    //Updates slacks of all pins based on the last timing analyzer
    void update_slacks(const tatum::SetupTimingAnalyzer& analyzer);

    //Updates slacks of the pin associated with 'node' based on the last timing analysis
    //Returns the pin if it's slack was modified, or AtomPinId::INVALID() if unchanged
    AtomPinId update_pin_slack(const tatum::NodeId node, const tatum::SetupTimingAnalyzer& analyzer);

    //Updates the criticalities of all pins based on the last timing analysis
    void update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

    //Updates maximum required times and worst slacks. Attempts to do so incrementally
    //if possible, but falls back to from-scratch method if needed.
    void update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                        const tatum::SetupTimingAnalyzer& analyzer);

    //Attempts to incrementally update maximum required times and worst slacks from scratch,
    //returns true if successful, false otherwise.
    bool incr_update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                             const tatum::SetupTimingAnalyzer& analyzer);

    //Recomputes maximum required times and worst slacks from scratch, updating the
    //relevant class data members
    void recompute_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                           const tatum::SetupTimingAnalyzer& analyzer);

    //Updates criticalities of pins associated with the specified set of timing graph nodes
    template<typename NodeRange>
    void update_pin_criticalities_from_nodes(const NodeRange& nodes, const tatum::SetupTimingAnalyzer& analyzer);

    //Updates the criticality of the pin associated with 'node' based on the last timing analysis
    //Returns the pin if it's criticality was modified, or AtomPinId::INVALID() if unchanged
    AtomPinId update_pin_criticality(const tatum::NodeId node,
                                     const tatum::SetupTimingAnalyzer& analyzer);

    //Records the timing graph nodes modified during the last timing analysis.
    //Updates modified_nodes_ and modified_sink_nodes
    void record_modified_nodes(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

    bool is_internal_tnode(const AtomPinId pin, const tatum::NodeId node) const;
    bool is_external_tnode(const AtomPinId pin, const tatum::NodeId node) const;

    //Returns the timing graph nodes which need to be updated depending on whether an
    //incremental or non-incremental update is requested.
    auto nodes_to_update(bool incremental) const {
        if (incremental) {
            //Note that this is done lazily only on the nodes modified by the analyzer
            return vtr::make_range(modified_nodes_.begin(), modified_nodes_.end());
        } else {
            return vtr::make_range(all_nodes_.begin(), all_nodes_.end());
        }
    }

    //Sanity check that calculated pin criticalites match those calculated from scratch
    bool verify_pin_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) const;

    //Sanity check that calculated max required and worst slacks match those calculated from scratch
    //Note modifies max_req_* and worst_slack_* to from-scratch values
    bool verify_max_req_worst_slack(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer);

  private: //Data
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;

    vtr::vector<AtomPinId, float> pin_slacks_;        //Calculated adjusted slacks of all pins
    vtr::vector<AtomPinId, float> pin_criticalities_; //Calculated criticality of all pins

    std::vector<AtomPinId> pins_with_modified_slacks_;        //Set of pins with modified slacks
    std::vector<AtomPinId> pins_with_modified_criticalities_; //Set of pins with modified criticalities

    std::map<DomainPair, float> max_req_;                  //Maximum required times for all clock domains
    std::map<DomainPair, float> worst_slack_;              //Worst slacks for all clock domains
    std::map<DomainPair, tatum::NodeId> max_req_node_;     //Timing graph nodes with maximum required times (for all clock domains)
    std::map<DomainPair, tatum::NodeId> worst_slack_node_; //Timing graph nodes with worst slacks (for all clock domains)

    std::map<DomainPair, float> prev_max_req_;     //Maximum required times from previous call to update_max_req_worst_slack()
    std::map<DomainPair, float> prev_worst_slack_; //Worst slacks from previous call to update_max_req_worst_slack()

    std::vector<tatum::NodeId> all_nodes_; //Set of all timing graph nodes (used only for non-incremental updates)

    std::vector<tatum::NodeId> modified_nodes_;      //Modified timing graph nodes
    std::vector<tatum::NodeId> modified_sink_nodes_; //Modified timing graph nodes of tatum::NodeType::SINK type

    //Run-time metrics
    size_t incr_slack_updates_ = 0;
    float incr_slack_update_time_sec_ = 0.;
    size_t full_max_req_worst_slack_updates_ = 0;
    float full_max_req_worst_slack_update_time_sec_ = 0.;
    size_t incr_max_req_worst_slack_updates_ = 0;
    float incr_max_req_worst_slack_update_time_sec_ = 0.;
    size_t incr_criticality_updates_ = 0;
    float incr_criticality_update_time_sec_ = 0.;
    size_t full_criticality_updates_ = 0;
    float full_criticality_update_time_sec_ = 0.;
};

//TODO: fully implement a HoldSlackCrit class for hold analysis
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
