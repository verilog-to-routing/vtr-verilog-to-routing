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
#    include <tbb/combinable.h>
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

//Incrementally update slack and criticality?
constexpr bool INCR_UPDATE_ATOM_SLACK = true;
constexpr bool INCR_UPDATE_ATOM_MAX_REQ_WORST_SLACK = true;
constexpr bool INCR_UPDATE_ATOM_CRIT = true;

SetupSlackCrit::SetupSlackCrit(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , pin_slacks_(netlist_.pins().size(), NAN)
    , pin_criticalities_(netlist_.pins().size(), NAN) {
    //We are doing some kind of full (i.e. non-incremental updates), record
    //all the relevant timing graph nodes
    all_nodes_.reserve(netlist.pins().size());
    for (AtomPinId pin : netlist.pins()) {
        tatum::NodeId node = netlist_lookup_.atom_pin_tnode(pin);

        if (node) {
            all_nodes_.push_back(node);
        }
    }
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

    auto nodes = nodes_to_update(INCR_UPDATE_ATOM_SLACK);

    pins_with_modified_slacks_.clear();

#if defined(VPR_USE_TBB)
    tbb::combinable<std::vector<AtomPinId>> modified_pins; //Per-thread vectors

    tbb::parallel_for_each(nodes.begin(), nodes.end(), [&, this](tatum::NodeId node) {
        AtomPinId modified_pin = this->update_pin_slack(node, analyzer);

        if (modified_pin) {
            modified_pins.local().push_back(modified_pin); //Insert into per-thread vector
        }
    });

    //Merge per-thread modified pins vectors
    modified_pins.combine_each([&](const std::vector<AtomPinId>& pins) {
        pins_with_modified_slacks_.insert(pins_with_modified_slacks_.end(),
                                          pins.begin(), pins.end());
    });

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

    //Multiple timing graph nodes may map to the same sequential primitive pin.
    //When determining the criticality to use for such pins we always choose the
    //external facing node. This ensures the pin criticality reflects the criticality
    //of the external timing path connected to this pin.
    if (is_internal_tnode(pin, node)) {
        return AtomPinId::INVALID();
    }

    VTR_ASSERT_SAFE(is_external_tnode(pin, node));

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

    {
        vtr::Timer timer;

        bool previously_updated = (!prev_max_req_.empty() && !prev_worst_slack_.empty());

        bool max_req_unchanged = (max_req_ == prev_max_req_);
        bool worst_slack_unchanged = (worst_slack_ == prev_worst_slack_);

        //If max required times and worst slacks are unchanged, we could incrementally
        //update the criticalities of each pin. (An incremental update is done lazily,
        //only on the nodes modified by the analyzer.)
        //
        //Otherwise (if the max required and/or worst slacks changed), we fully
        //recalculate all criticalities.
        //
        //  TODO: consider if incremental criticality update is feasible based only
        //        on changed domain pairs....
        bool could_do_incremental_update = (previously_updated
                                            && max_req_unchanged
                                            && worst_slack_unchanged);

        //For debugability, only do incremental updates if INCR_UPDATE_ATOM_CRIT is true
        bool do_incremental_update = (INCR_UPDATE_ATOM_CRIT && could_do_incremental_update);

        const auto& nodes = nodes_to_update(do_incremental_update);

        update_pin_criticalities_from_nodes(nodes, analyzer);

        VTR_ASSERT_DEBUG_MSG(verify_pin_criticalities(timing_graph, analyzer), "Updated pin criticalities should match those computed from scratch");

        //Save the max required times and worst slacks so we can determine when next
        //updated whether the update can be done incrementally
        prev_max_req_ = max_req_;
        prev_worst_slack_ = worst_slack_;

        if (do_incremental_update) {
            ++incr_criticality_updates_;
            incr_criticality_update_time_sec_ += timer.elapsed_sec();
        } else {
            ++full_criticality_updates_;
            full_criticality_update_time_sec_ += timer.elapsed_sec();
        }
    }
}

void SetupSlackCrit::update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                    const tatum::SetupTimingAnalyzer& analyzer) {
    bool incr_update_successful = false;
    if (INCR_UPDATE_ATOM_MAX_REQ_WORST_SLACK) {
        //Try to incrementally update
        incr_update_successful = incr_update_max_req_and_worst_slack(timing_graph, analyzer);
    }

    if (!incr_update_successful) {
        //Recompute  from scratch if incremental update wasn't successful
        recompute_max_req_and_worst_slack(timing_graph, analyzer);
    }
}

bool SetupSlackCrit::incr_update_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                         const tatum::SetupTimingAnalyzer& analyzer) {
    /*
     * This function attempts to incrementally update the maximum required times and worst slacks
     * for all clock domain pairs.
     *
     * This is accomplished by iterating through the all *modified* SINK timing nodes (i.e. nodes
     * which correspond to primary outputs and FF inputs) and comparing their values with those
     * previously calculated. Since the number of *modified* SINKs can be much less than the total
     * number the incremental update can be more efficient.
     *
     * This is essentially the same calculation as recompute_max_req_and_worst_slack() (which iterates
     * over *all* primary). By interating only over the modified nodes, this will produce the correct 
     * result *except* when the previous max-required/worst-slack have changed.
     *
     * In such as case, if the previously dominant node has had it's required time decreased (or worst 
     * slack increased) it may no longer be dominant, and we abort the incremental update (a full update
     * must be performed).
     *
     * Generally, the max required time and worst slack change relatively infrequently, so this speculative
     * update approach tends to be much more efficient.
     */
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

    VTR_ASSERT_DEBUG_MSG(verify_max_req_worst_slack(timing_graph, analyzer), "Calculated max required times and worst slacks should match those computed from scratch");

    ++incr_max_req_worst_slack_updates_;
    incr_max_req_worst_slack_update_time_sec_ += timer.elapsed_sec();

    return true; //Successfully incrementally updated
}

void SetupSlackCrit::recompute_max_req_and_worst_slack(const tatum::TimingGraph& timing_graph,
                                                       const tatum::SetupTimingAnalyzer& analyzer) {
    //Recomputes from scratch, the maximum required time and worst slack per clock domain pair
    vtr::Timer timer;

    //Clear any previous values
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
                max_req_node_[domain_pair] = node; //Record dominant node to help later  incremental updates
            }
        }

        for (auto& tag : analyzer.setup_slacks(node)) {
            auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

            float slack = tag.time().value();

            VTR_ASSERT_SAFE_MSG(!std::isnan(slack), "Slack should not be nan");
            VTR_ASSERT_SAFE_MSG(std::isfinite(slack), "Slack should not be infinite");

            if (!worst_slack_.count(domain_pair) || slack < worst_slack_[domain_pair]) {
                worst_slack_[domain_pair] = slack;
                worst_slack_node_[domain_pair] = node; //Record dominant node to help later  incremental updates
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
    tbb::combinable<std::vector<AtomPinId>> modified_pins; //Per-thread vectors

    tbb::parallel_for_each(nodes.begin(), nodes.end(), [&, this](tatum::NodeId node) {
        AtomPinId modified_pin = update_pin_criticality(node, analyzer);

        if (modified_pin) {
            modified_pins.local().push_back(modified_pin); //Insert into per-thread vector
        }
    });

    //Merge per-thread modified pins vectors
    modified_pins.combine_each([&](const std::vector<AtomPinId>& pins) {
        pins_with_modified_criticalities_.insert(pins_with_modified_criticalities_.end(),
                                                 pins.begin(), pins.end());
    });

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
    if (is_internal_tnode(pin, node)) {
        return AtomPinId::INVALID();
    }

    VTR_ASSERT_SAFE(is_external_tnode(pin, node));

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

bool SetupSlackCrit::is_internal_tnode(const AtomPinId pin, const tatum::NodeId node) const {
    return !is_external_tnode(pin, node);
}

bool SetupSlackCrit::is_external_tnode(const AtomPinId pin, const tatum::NodeId node) const {
    return netlist_lookup_.atom_pin_tnode(pin, BlockTnode::EXTERNAL) == node;
}

bool SetupSlackCrit::verify_pin_criticalities(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) const {
    auto modified_nodes = nodes_to_update(/*incremental=*/true);
    auto check_nodes = timing_graph.nodes();
    for (tatum::NodeId node : check_nodes) {
        AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
        VTR_ASSERT_SAFE(pin);

        //Multiple timing graph nodes may map to the same sequential primitive pin.
        //When determining the criticality to use for such pins we always choose the
        //external facing node. This ensures the pin criticality reflects the criticality
        //of the external timing path connected to this pin.
        if (is_internal_tnode(pin, node)) {
            continue;
        }

        VTR_ASSERT_SAFE(is_external_tnode(pin, node));

        float new_crit = calc_relaxed_criticality(max_req_, worst_slack_, analyzer.setup_slacks(node));
        if (new_crit != pin_criticalities_[pin]) {
            auto itr = std::find(modified_nodes.begin(), modified_nodes.end(), node);

            bool in_modified = (itr != modified_nodes.end());

            VPR_ERROR(VPR_ERROR_TIMING,
                      "Mismatched pin criticality was %g, but expected %g, in modified %d: pin '%s' (%zu) tnode %zu\n",
                      pin_criticalities_[pin],
                      new_crit,
                      in_modified,
                      netlist_.pin_name(pin).c_str(),
                      size_t(pin),
                      size_t(node));
            return false;
        }
    }

    return true;
}

bool SetupSlackCrit::verify_max_req_worst_slack(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& analyzer) {
    auto calc_max_req = max_req_;
    auto calc_max_req_node = max_req_node_;

    auto calc_worst_slack = worst_slack_;
    auto calc_worst_slack_node = worst_slack_node_;

    recompute_max_req_and_worst_slack(timing_graph, analyzer);

    if (calc_max_req != max_req_) {
        VPR_ERROR(VPR_ERROR_TIMING,
                  "Calculated max required times does not match value calculated from scratch");
        return false;
    }
    if (calc_max_req_node != max_req_node_) {
        VPR_ERROR(VPR_ERROR_TIMING,
                  "Calculated max required nodes does not match value calculated from scratch");
        return false;
    }
    if (calc_worst_slack != worst_slack_) {
        VPR_ERROR(VPR_ERROR_TIMING,
                  "Calculated worst slack does not match value calculated from scratch");
        return false;
    }
    if (calc_worst_slack_node != worst_slack_node_) {
        VPR_ERROR(VPR_ERROR_TIMING,
                  "Calculated worst slack nodes does not match value calculated from scratch");
        return false;
    }

    return true;
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
