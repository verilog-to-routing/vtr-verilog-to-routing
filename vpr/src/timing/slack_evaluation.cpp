#include "slack_evaluation.h"

#include "tatum/TimingGraph.hpp"
#include "timing_util.h"
#include "vpr_error.h"
#include "atom_netlist.h"
#include "vtr_log.h"
#include "vtr_time.h"

#if defined(VPR_USE_TBB)
#    include <tbb/task_group.h>
#    include <tbb/parallel_for_each.h>
#endif

template<typename T>
void nodes_to_pins(T nodes, const AtomLookup& atom_lookup, std::vector<AtomPinId>& pins) {
    pins.reserve(nodes.size());
    for (tatum::NodeId node : nodes) {
        AtomPinId pin = atom_lookup.tnode_atom_pin(node);
        pins.push_back(pin);
    }
}

/*
 * SetupSlackCrit
 */

SetupSlackCrit::SetupSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , pin_slacks_(netlist_.pins().size(), NAN)
    , pin_criticalities_(netlist_.pins().size(), NAN) {

#if !defined(INCR_SLACK_UPDATE) || !defined(INCR_UPDATE_CRIT)
    all_nodes_.reserve(netlist.pins().size());
    for (AtomPinId pin : netlist.pins()) {
        tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);

        if (node) {
            all_nodes_.push_back(node);
        }
    }
#endif
}

SetupSlackCrit::~SetupSlackCrit() {
    VTR_LOG("Incr Slack updates %zu in %g sec\n", incr_slack_updates_, incr_slack_update_time_sec_);
    VTR_LOG("Full Max Req/Worst Slack updates %zu in %g sec\n", full_max_req_worst_slack_updates_, full_max_req_worst_slack_update_time_sec_);
    VTR_LOG("Incr Max Req/Worst Slack updates %zu in %g sec\n", incr_max_req_worst_slack_updates_, incr_max_req_worst_slack_update_time_sec_);
    VTR_LOG("Incr Criticality updates %zu in %g sec\n", incr_criticality_updates_, incr_criticality_update_time_sec_);
    VTR_LOG("Full Criticality updates %zu in %g sec\n", full_criticality_updates_, full_criticality_update_time_sec_);
}

//Returns the worst (least) slack of connections through the specified pin
float SetupSlackCrit::setup_pin_slack(AtomPinId pin) const { return pin_slacks_[pin]; }

//Returns the worst (maximum) criticality of connections through the specified pin.
//  Criticality (in [0., 1.]) represents how timing-critical something is,
//  0. is non-critical and 1. is most-critical.
float SetupSlackCrit::setup_pin_criticality(AtomPinId pin) const { return pin_criticalities_[pin]; }

SetupSlackCrit::modified_pin_range SetupSlackCrit::pins_with_modified_slack() const {
    return vtr::make_range(pins_with_modified_slacks_);
}

SetupSlackCrit::modified_pin_range SetupSlackCrit::pins_with_modified_criticality() const {
    return vtr::make_range(pins_with_modified_criticalities_);
}

void SetupSlackCrit::update_slacks_and_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
    record_modified_nodes(timing_graph, analyzer);

#if defined(VPR_USE_TBB)
    tbb::task_group g;
    g.run([&] { update_slacks(analyzer); });
    g.run([&] { update_criticalities(timing_graph, analyzer); });
    g.wait();
#else
    update_slacks(analyzer);
    update_criticalities(timing_graph, analyzer);
#endif
}

void SetupSlackCrit::update_slacks(const tatum::SetupTimingAnalyzer& analyzer) {
    vtr::Timer timer;

#ifdef INCR_UPDATE_SLACK
    //Note that this is done lazily only on the nodes modified by the analyzer
    const auto& nodes = modified_nodes_;
#else
    const auto& nodes = all_nodes_;
#endif

    pins_with_modified_slacks_.clear();

#if defined(VPR_USE_TBB)
    tbb::parallel_for_each(nodes.begin(), nodes.end(), [&, this](tatum::NodeId node) {
        this->update_pin_slack(node, analyzer);
    });

    //TODO: this is pessimistically assuming all nodes are modified (i.e. ignoring 
    //      return value of update_pin_slack()
    //TODO: parallelize
    nodes_to_pins(nodes, netlist_lookup_, pins_with_modified_slacks_);
#else
    for (tatum::NodeId node : nodes) {
        AtomPinId modified_pin = update_pin_slack(node, analyzer);
        if (modified_pin) {
            pins_with_modified_slacks_.push_back(modified_pin);
        }
    }
#endif


    ++incr_slack_updates_;
    incr_slack_update_time_sec_ += timer.elapsed_sec();
}

AtomPinId SetupSlackCrit::update_pin_slack(const tatum::NodeId node, const tatum::SetupTimingAnalyzer& analyzer) {
    //Find the timing node associated with the pin
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    VTR_ASSERT_SAFE(pin);

    //Find the worst (least) slack at this node
    auto tags = analyzer.setup_slacks(node);
    auto min_tag_iter = find_minimum_tag(tags);

    float new_slack = std::numeric_limits<float>::quiet_NaN();
    if (min_tag_iter != tags.end()) {
        new_slack = min_tag_iter->time().value();
    } else {
        //No tags (e.g. driven by constant generator)
        new_slack = std::numeric_limits<float>::infinity();
    }

    if (pin_slacks_[pin] != new_slack) {
        pin_slacks_[pin] = new_slack;
        return pin; //Modified
    }
    return AtomPinId::INVALID(); //Unchanged
}

void SetupSlackCrit::update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
    //Record the maximum required time, and wost slack per domain pair
    update_max_req_and_worst_slack(timing_graph, analyzer);

    if (max_req_ == prev_max_req_ && worst_slack_ == prev_worst_slack_) {
        //Max required times and worst slacks unchanged, incrementally update
        //the criticalities of each pin
        //
        // Note that this is done lazily only on the nodes modified by the analyzer
        vtr::Timer timer;
#ifdef INCR_UPDATE_CRIT
        const auto& nodes = modified_nodes_;
#else
        const auto& nodes = all_nodes_;
#endif

        update_pin_criticalities_from_nodes(nodes, analyzer);

#if 0
        //Sanity check
        auto check_nodes = timing_graph.nodes();
        for (tatum::NodeId node : check_nodes) {
            AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
            VTR_ASSERT_SAFE(pin);
            float new_crit = calc_pin_criticality(node, analyzer);

            //Multiple timing graph nodes may map to the same sequential primitive pin.
            //When determining the criticality to use for such pins we always choose the
            //external facing node. This ensures the pin criticality reflects the criticality
            //of the external timing path connected to this pin.
            if (netlist_lookup_.atom_pin_tnode(pin, BlockTnode::EXTERNAL) != node) {
                continue;
            }

            if (new_crit != pin_criticalities_[pin]) {
                auto itr = std::find(nodes.begin(), nodes.end(), node);

                VPR_ERROR(VPR_ERROR_TIMING,
                          "Mismatched pin criticality was %g, but expected %g %g, in modified %d: pin '%s' (%zu) tnode %zu\n", pin_criticalities_[pin], new_crit,  calc_pin_criticality(node, analyzer), itr != nodes.end(), netlist_.pin_name(pin).c_str(), size_t(pin), size_t(node));
            }
        }
#endif


        ++incr_criticality_updates_;
        incr_criticality_update_time_sec_ += timer.elapsed_sec();
    } else {
        //Max required and/or worst slacks changed, fully recalculate criticalities
        //
        //  TODO: consider if incremental criticality update is feasible based only 
        //        on changed domain pairs....
        vtr::Timer timer;

        auto nodes = timing_graph.nodes();

        update_pin_criticalities_from_nodes(nodes, analyzer);

        ++full_criticality_updates_;
        full_criticality_update_time_sec_ += timer.elapsed_sec();
    }
    prev_max_req_ = max_req_;
    prev_worst_slack_ = worst_slack_;
}

void SetupSlackCrit::update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                    const tatum::SetupTimingAnalyzer& analyzer) {
#ifdef INCR_UPDATE_MAX_REQ_WORST_SLACK
    //Try to incrementally update
    bool incr_update_successful = incr_update_max_req_and_worst_slack(timing_graph, analyzer);
#else
    bool incr_update_successful = false;
#endif

    if (!incr_update_successful) {
        //Recompute  from scratch if incremental update wasn't successful
        recompute_max_req_and_worst_slack(timing_graph, analyzer);
    }

}

bool SetupSlackCrit::incr_update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                         const tatum::SetupTimingAnalyzer& analyzer) {
    vtr::Timer timer;

    if (max_req_node_.empty() || worst_slack_node_.empty()) {
        //Must have been previously updated
        return false;
    }
    
    for (tatum::NodeId node : modified_sink_nodes_) {
        for (auto& tag : analyzer.setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float req = tag.time().value();

            VTR_ASSERT_SAFE(max_req_.count(domain_pair));
            float prev_req = max_req_[domain_pair];

            VTR_ASSERT_SAFE(max_req_node_.count(domain_pair));
            tatum::NodeId prev_node = max_req_node_[domain_pair];

            if (prev_node == node && prev_req > req) {
                //This node was the dominant node, but is no longer dominant we
                //must re-evaluate all other nodes to figure out which is now dominant
                //so abort incremental update
                return false; //Unsuccessful incremental update
            }

            if (max_req_[domain_pair] < req) {
                max_req_[domain_pair] = req;
                max_req_node_[domain_pair] = node;
            }
        }

        for (auto& tag : analyzer.setup_slacks(node)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float slack = tag.time().value();

            VTR_ASSERT_SAFE_MSG(!std::isnan(slack), "Slack should not be nan");
            VTR_ASSERT_SAFE_MSG(std::isfinite(slack), "Slack should not be infinite");

            VTR_ASSERT_SAFE(worst_slack_.count(domain_pair));
            float prev_slack = worst_slack_[domain_pair];

            VTR_ASSERT_SAFE(worst_slack_node_.count(domain_pair));
            tatum::NodeId prev_node = worst_slack_node_[domain_pair];

            if (prev_node == node && slack > prev_slack) {
                //This node was the dominant node, but is no longer dominant we
                //must re-evaluate all other nodes to figure out which is now dominant
                //so abort incremental update
                return false; //Unsuccessful incremental update
            }

            if (slack < worst_slack_[domain_pair]) {
                worst_slack_[domain_pair] = slack;
                worst_slack_node_[domain_pair] = node;
            }
        }
    }

#ifdef VTR_ASSERT_DEBUG_ENABLED
    auto incr_max_req = max_req_;
    auto incr_max_req_node = max_req_node_;

    auto incr_worst_slack = worst_slack_;
    auto incr_worst_slack_node = worst_slack_node_;

    recompute_max_req_and_worst_slack(timing_graph, analyzer);

    VTR_ASSERT_DEBUG(incr_max_req == max_req_);
    VTR_ASSERT_DEBUG(incr_max_req_node == max_req_node_);
    VTR_ASSERT_DEBUG(incr_worst_slack == worst_slack_);
    VTR_ASSERT_DEBUG(incr_worst_slack_node == worst_slack_node_);
#else
    static_cast<void>(timing_graph);
#endif

    ++incr_max_req_worst_slack_updates_;
    incr_max_req_worst_slack_update_time_sec_ += timer.elapsed_sec();

    return true; //Successfully incrementally updated
}

void SetupSlackCrit::recompute_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                       const tatum::SetupTimingAnalyzer& analyzer) {
    vtr::Timer timer;

    max_req_.clear();
    max_req_node_.clear();

    worst_slack_.clear();
    worst_slack_node_.clear();

    for (tatum::NodeId node : timing_graph.logical_outputs()) {
        for (auto& tag : analyzer.setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float req = tag.time().value();
            if (!max_req_.count(domain_pair) || max_req_[domain_pair] < req) {
                max_req_[domain_pair] = req;
                max_req_node_[domain_pair] = node;
            }
        }

        for (auto& tag : analyzer.setup_slacks(node)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float slack = tag.time().value();

            VTR_ASSERT_SAFE_MSG(!std::isnan(slack), "Slack should not be nan");
            VTR_ASSERT_SAFE_MSG(std::isfinite(slack), "Slack should not be infinite");

            if (!worst_slack_.count(domain_pair) || slack < worst_slack_[domain_pair]) {
                worst_slack_[domain_pair] = slack;
                worst_slack_node_[domain_pair] = node;
            }
        }
    }

    ++full_max_req_worst_slack_updates_;
    full_max_req_worst_slack_update_time_sec_ += timer.elapsed_sec();
}

template<typename NodeRange>
void SetupSlackCrit::update_pin_criticalities_from_nodes(const NodeRange& nodes, const tatum::SetupTimingAnalyzer& analyzer) {

        pins_with_modified_criticalities_.clear();
#if defined(VPR_USE_TBB)
        tbb::parallel_for_each(nodes.begin(), nodes.end(), [&, this](tatum::NodeId node) {
            update_pin_criticality(node, analyzer);
        });

        //TODO: this is pessimistically assuming all nodes are modified (i.e. ignoring 
        //      return value of update_pin_slack()
        //TODO: parallelize
        nodes_to_pins(nodes, netlist_lookup_, pins_with_modified_criticalities_);
#else
        for (tatum::NodeId node : nodes) {
            AtomPinId modified_pin = update_pin_criticality(node, analyzer);
            if (modified_pin) {
                pins_with_modified_criticalities_.push_back(modified_pin);
            }
        }
#endif
}

AtomPinId SetupSlackCrit::update_pin_criticality(const tatum::NodeId node,
                                const tatum::SetupTimingAnalyzer& analyzer) {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    VTR_ASSERT_SAFE(pin);

    //Multiple timing graph nodes may map to the same sequential primitive pin.
    //When determining the criticality to use for such pins we always choose the
    //external facing node. This ensures the pin criticality reflects the criticality
    //of the external timing path connected to this pin.
    if (is_internal_tnode(node)) {
        return AtomPinId::INVALID();
    }

    float new_crit = calc_relaxed_criticality(max_req_, worst_slack_, analyzer.setup_slacks(node));

    if (pin_criticalities_[pin] != new_crit) {
        pin_criticalities_[pin] = new_crit;
        return pin; //Modified
    }
    return AtomPinId::INVALID(); //Unmodified
}

void SetupSlackCrit::record_modified_nodes(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
    const auto& nodes = analyzer.modified_nodes();

    modified_nodes_.clear();
    modified_sink_nodes_.clear();

    modified_nodes_.reserve(nodes.size());

    for (tatum::NodeId node : nodes) {
        modified_nodes_.push_back(node);

        if (timing_graph.node_type(node) == tatum::NodeType::SINK) {
            modified_sink_nodes_.push_back(node);
        }
    }
}

bool SetupSlackCrit::is_internal_tnode(const tatum::NodeId node) const {
    return !is_external_tnode(node);
}

bool SetupSlackCrit::is_external_tnode(const tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    VTR_ASSERT_SAFE(pin);

    return netlist_lookup_.atom_pin_tnode(pin, BlockTnode::EXTERNAL) == node;
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
#if defined(VPR_USE_TBB)
    tbb::task_group g;
    g.run([&] { update_slacks(analyzer); });
    g.run([&] { update_criticalities(timing_graph, analyzer); });
    g.wait();
#else
    update_slacks(analyzer);
    update_criticalities(timing_graph, analyzer);
#endif
}

void HoldSlackCrit::update_slacks(const tatum::HoldTimingAnalyzer& analyzer) {
    for (AtomPinId pin : netlist_.pins()) {
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
    if (min_tag_iter != tags.end()) {
        pin_slacks_[pin] = min_tag_iter->time().value();
    } else {
        //No tags (e.g. driven by constant generator)
        pin_slacks_[pin] = std::numeric_limits<float>::infinity();
    }
}

void HoldSlackCrit::update_criticalities(const tatum::TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& analyzer) {
    //TODO: this calculates a simple shifted and scaled criticality, not clear if this is the
    //right approach (e.g. should we use a more intellegent method like the one used by setup slack?)
    float worst_slack = std::numeric_limits<float>::infinity();
    float best_slack = -std::numeric_limits<float>::infinity();
    for (tatum::NodeId node : timing_graph.nodes()) {
        for (auto& tag : analyzer.hold_slacks(node)) {
            float slack = tag.time().value();
            worst_slack = std::min(worst_slack, slack);
            best_slack = std::max(best_slack, slack);
        }
    }

    //Calculate the transformation from slack to criticality,
    //we scale and shift the result so the worst_slack takes on criticalty 1.0
    //while the best slack takes on criticality 0.0
    float scale = 1. / std::abs(best_slack - worst_slack);
    float shift = -worst_slack;

    //Update the criticalities of each pin
    auto pins = netlist_.pins();
#if defined(VPR_USE_TBB)
    tbb::parallel_for_each(pins.begin(), pins.end(), [&, this](auto pin) {
        this->pin_criticalities_[pin] = this->calc_pin_criticality(pin, analyzer, scale, shift);
    });
#else
    for (auto pin : pins) {
        pin_criticalities_[pin] = calc_pin_criticality(pin, analyzer, scale, shift);
    }
#endif
}

float HoldSlackCrit::calc_pin_criticality(AtomPinId pin,
                                          const tatum::HoldTimingAnalyzer& analyzer,
                                          const float scale,
                                          const float shift) {
    tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);
    VTR_ASSERT(node);

    float criticality = 0.;

    for (auto tag : analyzer.hold_slacks(node)) {
        float slack = tag.time().value();

        float tag_criticality = 1. - scale * (slack + shift);

        criticality = std::max(criticality, tag_criticality);
    }

    VTR_ASSERT(criticality >= 0.);
    VTR_ASSERT(criticality <= 1.);

    return criticality;
}
