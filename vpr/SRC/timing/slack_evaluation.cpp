#include "slack_evaluation.h"

#include "tatum/TimingGraph.hpp"
#include "timing_util.h"
#include "vpr_error.h"
#include "atom_netlist.h"

/*
 * SetupSlackCrit
 */

SetupSlackCrit::SetupSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , pin_slacks_(netlist_.pins().size(), NAN)
    , pin_criticalities_(netlist_.pins().size(), NAN) {
    //pass
}

//Returns the worst (least) slack of connections through the specified pin
float SetupSlackCrit::setup_pin_slack(AtomPinId pin) const { return pin_slacks_[pin]; }

//Returns the worst (maximum) criticality of connections through the specified pin.
//  Criticality (in [0., 1.]) represents how timing-critical something is, 
//  0. is non-critical and 1. is most-critical.
float SetupSlackCrit::setup_pin_criticality(AtomPinId pin) const { return pin_criticalities_[pin]; }

void SetupSlackCrit::update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
    update_slacks(analyzer);
    update_criticalities(timing_graph, analyzer);
}

void SetupSlackCrit::update_slacks(const tatum::SetupTimingAnalyzer& analyzer) {
    for(AtomPinId pin : netlist_.pins()) {
        update_pin_slack(pin, analyzer);
    }
}

void SetupSlackCrit::update_pin_slack(const AtomPinId pin, const tatum::SetupTimingAnalyzer& analyzer) {
    //Find the timing node associated with the pin
    tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);
    VTR_ASSERT(node);

    //Find the worst (least) slack at this node
    auto tags = analyzer.setup_slacks(node);
    auto min_tag_iter = find_minimum_tag(tags);
    if(min_tag_iter != tags.end()) {
        pin_slacks_[pin] = min_tag_iter->time().value();
    } else {
        //No tags (e.g. driven by constant generator)
        pin_slacks_[pin] = std::numeric_limits<float>::infinity();
    }
}

void SetupSlackCrit::update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
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

void SetupSlackCrit::update_pin_criticality(AtomPinId pin, 
                                            const tatum::SetupTimingAnalyzer& analyzer, 
                                            const std::map<DomainPair,float>& max_req, 
                                            const std::map<DomainPair,float>& worst_slack) {
    tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);
    VTR_ASSERT(node);

    //Calculate maximum criticality over all domains
    pin_criticalities_[pin] = calc_relaxed_criticality(max_req, worst_slack, analyzer.setup_slacks(node));
}

/*
 * HoldSlackCrit
 */

HoldSlackCrit::HoldSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , pin_slacks_(netlist_.pins().size(), NAN)
    , pin_criticalities_(netlist_.pins().size(), NAN) {
    //pass
}

//Returns the worst (least) slack of connections through the specified pin
float HoldSlackCrit::hold_pin_slack(AtomPinId pin) const { return pin_slacks_[pin]; }

//Returns the worst (maximum) criticality of connections through the specified pin.
//  Criticality (in [0., 1.]) represents how timing-critical something is, 
//  0. is non-critical and 1. is most-critical.
float HoldSlackCrit::hold_pin_criticality(AtomPinId pin) const { return pin_criticalities_[pin]; }

void HoldSlackCrit::update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& analyzer) {
    update_slacks(analyzer);
    update_criticalities(timing_graph, analyzer);
}

void HoldSlackCrit::update_slacks(const tatum::HoldTimingAnalyzer& analyzer) {
    for(AtomPinId pin : netlist_.pins()) {
        update_pin_slack(pin, analyzer);
    }
}

void HoldSlackCrit::update_pin_slack(const AtomPinId pin, const tatum::HoldTimingAnalyzer& analyzer) {
    //Find the timing node associated with the pin
    tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);
    VTR_ASSERT(node);

    //Find the worst (least) slack at this node
    auto tags = analyzer.hold_slacks(node);
    auto min_tag_iter = find_minimum_tag(tags);
    if(min_tag_iter != tags.end()) {
        pin_slacks_[pin] = min_tag_iter->time().value();
    } else {
        //No tags (e.g. driven by constant generator)
        pin_slacks_[pin] = std::numeric_limits<float>::infinity();
    }
}

void HoldSlackCrit::update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& analyzer) {
    //Record the maximum required time, and wost slack per domain pair
    std::map<DomainPair,float> max_req;
    std::map<DomainPair,float> worst_slack;
    for(tatum::NodeId node : timing_graph.logical_outputs()) {

        for(auto& tag : analyzer.hold_tags(node, tatum::TagType::DATA_REQUIRED)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float req = tag.time().value();
            if(!max_req.count(domain_pair) || max_req[domain_pair] < req) {
                max_req[domain_pair] = req;
            }
        }

        for(auto& tag : analyzer.hold_slacks(node)) {
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

void HoldSlackCrit::update_pin_criticality(AtomPinId pin, 
                                            const tatum::HoldTimingAnalyzer& analyzer, 
                                            const std::map<DomainPair,float>& max_req, 
                                            const std::map<DomainPair,float>& worst_slack) {
    tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);
    VTR_ASSERT(node);

    //Calculate maximum criticality over all domains
    pin_criticalities_[pin] = calc_relaxed_criticality(max_req, worst_slack, analyzer.hold_slacks(node));
}
