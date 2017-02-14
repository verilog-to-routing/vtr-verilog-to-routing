#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <memory>
#include "atom_netlist.h"
#include "timing_analyzers.hpp"
#include "vtr_flat_map.h"
#include "vpr_error.h"
#include "timing_util.h"

class SetupSlackCrit {
    public: //Constructors
        SetupSlackCrit(const AtomNetlist& netlist, const AtomMap& netlist_map)
            : netlist_(netlist)
            , netlist_map_(netlist_map)
            , pin_slacks_(netlist_.pins().size(), NAN)
            , pin_criticalities_(netlist_.pins().size(), NAN) {
            //pass
        }

    public: //Accessors

        //Returns the worst (least) slack of connections through the specified pin
        float setup_pin_slack(AtomPinId pin) const { return pin_slacks_[pin]; }

        //Returns the worst (maximum) criticality of connections through the specified pin.
        //  Criticality (in [0., 1.]) represents how timing-critical something is, 
        //  0. is non-critical and 1. is most-critical.
        float setup_pin_criticality(AtomPinId pin) const { return pin_criticalities_[pin]; }

    public: //Mutators
        void update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
            update_slacks(analyzer);
            update_criticalities(timing_graph, analyzer);
        }

    private: //Implementation

        void update_slacks(const tatum::SetupTimingAnalyzer& analyzer) {
            for(AtomPinId pin : netlist_.pins()) {
                update_pin_slack(pin, analyzer);
            }
        }

        void update_pin_slack(const AtomPinId pin, const tatum::SetupTimingAnalyzer& analyzer) {
            //Find the timing node associated with the pin
            tatum::NodeId node = g_atom_map.pin_tnode[pin];
            VTR_ASSERT(node);

            //Find the worst (least) slack at this node
            auto min_tag = find_minimum_tag(analyzer.setup_slacks(node));
            pin_slacks_[pin] = min_tag.time().value();
        }

        void update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
            //Record the maximum required time, and wost slack per domain pair
            std::map<DomainPair,float> max_req;
            std::map<DomainPair,float> worst_slack;
            for(tatum::NodeId node : timing_graph.logical_outputs()) {

                for(auto& tag : analyzer.setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                    auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

                    float req = tag.time().value();
                    if(!max_req.count(domain_pair) || max_req[domain_pair] < req) {
                        max_req[domain_pair] = req;
                    }
                }

                for(auto& tag : analyzer.setup_slacks(node)) {
                    auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

                    float slack = tag.time().value();
                    if(!worst_slack.count(domain_pair) || slack < worst_slack[domain_pair]) {
                        worst_slack[domain_pair] = slack;
                    }
                }
            }

            //Update the criticalities of each pin
            for(AtomPinId pin : netlist_.pins()) {
                update_pin_criticality(pin, analyzer, max_req, worst_slack);
            }
        }

        void update_pin_criticality(AtomPinId pin, 
                                    const tatum::SetupTimingAnalyzer& analyzer, 
                                    const std::map<DomainPair,float>& max_req, 
                                    const std::map<DomainPair,float>& worst_slack) {
            tatum::NodeId node = g_atom_map.pin_tnode[pin];
            VTR_ASSERT(node);

            //Calculate maximum criticality over all domains
            pin_criticalities_[pin] = calc_relaxed_criticality(max_req, worst_slack, analyzer.setup_slacks(node));
        }

    private: //Data
        const AtomNetlist& netlist_;
        const AtomMap& netlist_map_;
        
        vtr::vector_map<AtomPinId, float> pin_slacks_;
        vtr::vector_map<AtomPinId, float> pin_criticalities_;
};

//TODO: implement a HoldSlackCrit class for hold analysis


#endif
