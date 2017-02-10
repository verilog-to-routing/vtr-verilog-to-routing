#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <memory>
#include "atom_netlist.h"
#include "timing_analyzers.hpp"
#include "vtr_flat_map.h"
#include "vpr_error.h"
#include "timing_util.h"

class SlackEvaluator {
    public:
        virtual ~SlackEvaluator() = default;

        //Update the slacks
        virtual void update()  = 0;
};

class SetupSlackEvaluator : public SlackEvaluator {
    public:
        virtual float setup_slack(AtomPinId src_pin, AtomPinId sink_pin) const = 0;
        virtual float setup_criticality(AtomPinId src_pin, AtomPinId sink_pin) const = 0;

        virtual float setup_slack(AtomPinId pin) const = 0;
        virtual float setup_criticality(AtomPinId pin) const = 0;

        virtual float critical_path_delay() const = 0;
};

class ConstantSetupSlackEvaluator : public SetupSlackEvaluator {
    public:
        ConstantSetupSlackEvaluator(float slack=0., float criticality=1., float cpd=NAN)
            : slack_(slack)
            , criticality_(criticality)
            , cpd_(cpd) {}

        float setup_slack(AtomPinId /*src_pin*/, AtomPinId /*sink_pin*/) const override { return slack_; }
        float setup_criticality(AtomPinId /*src_pin*/, AtomPinId /*sink_pin*/) const override { return criticality_; }

        float setup_slack(AtomPinId /*pin*/) const override { return slack_; }
        float setup_criticality(AtomPinId /*pin*/) const override { return criticality_; }

        float critical_path_delay() const override { return cpd_; }

        void update() override { /*NoOp*/ }

    private:
        const float slack_;
        const float criticality_;
        const float cpd_;
};

class DynamicSetupSlackEvaluator : public SetupSlackEvaluator {
    public:
        DynamicSetupSlackEvaluator(const AtomNetlist& netlist, const AtomMap& atom_map, std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : netlist_(netlist)
            , atom_map_(atom_map)
            , setup_analyzer_(analyzer) {}

        //Returns the slack of the connection from src_pin to sink_pin
        float setup_slack(AtomPinId src_pin, AtomPinId sink_pin) const override { 
            auto iter = setup_edge_slacks_.find({src_pin, sink_pin});

            if(iter == setup_edge_slacks_.end()) {
                VPR_THROW(VPR_ERROR_TIMING, "Invalid source/sink pin during slack look-up (pins must be connected)");
            }

            return iter->second;
        }

        //Returns the criticality of the connection from src_pin to sink_pin
        float setup_criticality(AtomPinId src_pin, AtomPinId sink_pin) const override { 
            auto iter = setup_edge_criticalities_.find({src_pin, sink_pin});

            if(iter == setup_edge_criticalities_.end()) {
                VPR_THROW(VPR_ERROR_TIMING, "Invalid source/sink pin during criticality look-up (pins must be connected)");
            }

            return iter->second;
        }

        //Returns the slack of the most critical connection (lowest slack) through the specified pin
        float setup_slack(AtomPinId pin) const override {
            return setup_pin_slacks_[pin];
        }

        //Returns the criticality of the most critical connection through the specified pin
        float setup_criticality(AtomPinId pin) const override {
            return setup_pin_criticalities_[pin];
        }

        float critical_path_delay() const override { return find_least_slack_critical_path_delay(g_timing_constraints, *setup_analyzer_).path_delay; }

        void update() override {
            setup_analyzer_->update_timing();

            update_setup_edge_slacks();
            update_setup_edge_criticalities();

            update_setup_pin_slacks();
            update_setup_pin_criticalities();
        }

    private:
        void update_setup_pin_slacks() {
            setup_pin_slacks_ = vtr::vector_map<AtomPinId,float>(netlist_.pins().size(), NAN);

            //Record the lowest slack for every sink pin
            for(auto kv : setup_edge_slacks_) {
                AtomPinId sink_pin = kv.first.second;
                float slack = kv.second;

                if (std::isnan(setup_pin_slacks_[sink_pin])) {
                    setup_pin_slacks_[sink_pin] = slack;
                } else {
                    setup_pin_slacks_[sink_pin] = std::min(setup_pin_slacks_[sink_pin], slack);
                }
            }
        }

        void update_setup_pin_criticalities() {
            setup_pin_criticalities_ = vtr::vector_map<AtomPinId,float>(netlist_.pins().size(), NAN);

            //Record the highest criticality for every sink pin
            for(auto kv : setup_edge_criticalities_) {
                AtomPinId sink_pin = kv.first.second;
                float criticality = kv.second;

                if (std::isnan(setup_pin_criticalities_[sink_pin])) {
                    setup_pin_criticalities_[sink_pin] = criticality;
                } else {
                    setup_pin_criticalities_[sink_pin] = std::min(setup_pin_criticalities_[sink_pin], criticality);
                }
            }

        }

    protected:
        virtual void update_setup_edge_slacks() = 0;
        virtual void update_setup_edge_criticalities() = 0;
    protected:
        const AtomNetlist& netlist_;
        const AtomMap& atom_map_;
        vtr::flat_map<std::pair<AtomPinId,AtomPinId>,float> setup_edge_slacks_;
        vtr::flat_map<std::pair<AtomPinId,AtomPinId>,float> setup_edge_criticalities_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> setup_analyzer_;
    private:
        vtr::vector_map<AtomPinId,float> setup_pin_slacks_;
        vtr::vector_map<AtomPinId,float> setup_pin_criticalities_;
};

/*
 * The slack formulations used in OptimizerSlack and AnalysisSlack
 * are derived from:
 *
 *      M. Wainberg and V. Betz, "Robust Optimization of Multiple Timing Constraints," 
 *          IEEE CAD, vol. 34, no. 12, pp. 1942-1953, Dec. 2015. doi: 10.1109/TCAD.2015.2440316
 * 
 * However unlike the implementation described above, we perform slack and criticality calculations
 * as a post-processing step after timing analysis (i.e. we do not modify the required times during
 * the timing analysis traversals).
 */

//OptimizerSlack produces slack and criticality values that are appropriate for use in an optimizer.
//If report slack to a user (rather than an optimizer) use AnalysisSlack
//
//  Slacks and criticality are calculated according to the per-constraint 'Relaxed Slacks' method 
//  from "Robust Optimization of Mulitple Timing Constraints" (ROMTC)
template<class DelayCalc>
class OptimizerSlacks : public DynamicSetupSlackEvaluator {
    public:
        OptimizerSlacks(const AtomNetlist& netlist, const AtomMap& atom_map, 
                       std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer, 
                       const tatum::TimingGraph& tg,
                       const DelayCalc& dc)
            : DynamicSetupSlackEvaluator(netlist, atom_map, analyzer)
            , tg_(tg)
            , dc_(dc)
            {}

    private:
        void update_setup_edge_slacks() override {
            update_edge_slacks();
        }

        void update_setup_edge_criticalities() override {
            update_edge_shifts();

            update_edge_criticalities();
        }

    private:

        struct DomainPair {
            tatum::DomainId launch;
            tatum::DomainId capture;

            friend bool operator<(const DomainPair& lhs, const DomainPair& rhs) {
                return std::tie(lhs.launch, lhs.capture) < std::tie(rhs.launch, rhs.capture);
            }
        };

        struct SlackOrigin {
            float slack;
            tatum::NodeId origin_node;
        };

        void update_edge_shifts() {

            //Record the worst arrival times per domain pair
            std::map<DomainPair,float> max_arr;
            for(tatum::NodeId node : tg_.logical_outputs()) {
                for(const tatum::TimingTag& arr_tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_ARRIVAL)) {
                    float arr_time = arr_tag.time().value();
                    tatum::DomainId launch = arr_tag.launch_clock_domain();

                    for(const tatum::TimingTag& req_tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                        tatum::DomainId capture = req_tag.capture_clock_domain();

                        DomainPair domains = {launch, capture};
                        if(max_arr.count(domains)) {
                            max_arr[domains] = std::max(max_arr[domains], arr_time);
                        } else {
                            max_arr[domains] = arr_time;
                        }
                    }
                }
            }

            //Determine the shift requried to relax required times and the maximum required time per domain
            for(tatum::NodeId node : tg_.logical_outputs()) {

                for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                    DomainPair domains = {tag.launch_clock_domain(), tag.capture_clock_domain()};

                    float req_time = tag.time().value();

                    VTR_ASSERT(max_arr.count(domains));
                    float shift = std::max(0.f, max_arr[domains] - req_time);
                    VTR_ASSERT(shift >= 0.);

                    //Record the max shift, per sink node and constraint
                    if(shifts_[node].count(domains)) {
                        //Keep the largest shift (i.e. minimal slack)
                        shifts_[node][domains] = std::max(shifts_[node][domains], shift);
                    } else {
                        shifts_[node][domains] = shift;
                    }

                    //Record the max req time per constraint
                    float max_req_relaxed = req_time + shift;
                    if(max_req_relaxed_.count(domains)) {
                        max_req_relaxed_[domains] = std::max(max_req_relaxed_[domains], max_req_relaxed);
                    } else {
                        max_req_relaxed_[domains] = max_req_relaxed;
                    }
                }
            }
        }

        void update_edge_criticalities() {
            setup_edge_criticalities_.clear();

            std::vector<std::pair<std::pair<AtomPinId,AtomPinId>,float>> criticalities;

            for(tatum::EdgeId edge : tg_.edges()) {
                tatum::NodeId src_node = tg_.edge_src_node(edge);
                tatum::NodeId sink_node = tg_.edge_sink_node(edge);

                AtomPinId src_pin = atom_map_.pin_tnode[src_node];
                AtomPinId sink_pin = atom_map_.pin_tnode[sink_node];

                float crit = criticality(edge);

                auto key = std::make_pair(src_pin, sink_pin);
                criticalities.push_back({key,crit});
            }
            
            setup_edge_criticalities_ = vtr::make_flat_map(std::move(criticalities));
        }

        void update_edge_slacks() {
            std::vector<std::pair<std::pair<AtomPinId,AtomPinId>,float>> slacks;

            for(tatum::EdgeId edge : tg_.edges()) {
                tatum::NodeId src_node = tg_.edge_src_node(edge);
                tatum::NodeId sink_node = tg_.edge_sink_node(edge);

                AtomPinId src_pin = atom_map_.pin_tnode[src_node];
                AtomPinId sink_pin = atom_map_.pin_tnode[sink_node];

                float min_slack = NAN;

                for(const auto& slack_tag : setup_analyzer_->setup_slacks(edge)) {
                    float slack = slack_tag.time().value();
                    if(std::isnan(min_slack) || slack < min_slack) {
                        min_slack = slack; 
                    }
                }

                auto key = std::make_pair(src_pin, sink_pin);
                slacks.push_back({key,min_slack});
            }
            setup_edge_slacks_ = vtr::make_flat_map(slacks);
        }

        float criticality(const tatum::EdgeId edge) {
            float max_criticality = NAN;

            //Maximum criticality over all constraints using per-constraint
            //relaxed required time
            auto slack_tags = setup_analyzer_->setup_slacks(edge);
            for(const auto& slack_tag : slack_tags) {
                DomainPair domains = {slack_tag.launch_clock_domain(), slack_tag.capture_clock_domain()};
                SlackOrigin slack_origin = {slack_tag.time().value(), slack_tag.origin_node()};

                float slack = shifted_slack(domains, slack_origin);
                float max_req = shifted_max_req(domains);

                float criticality = 1. - (slack / max_req);

                if(std::isnan(max_criticality) || max_criticality < criticality) {
                    max_criticality = criticality;
                }
            }

            VTR_ASSERT(slack_tags.empty() || !std::isnan(max_criticality));

            return max_criticality;
        }

        float shifted_slack(DomainPair constr, SlackOrigin slack_origin) {
            VTR_ASSERT(shifts_.count(slack_origin.origin_node));
            VTR_ASSERT(shifts_[slack_origin.origin_node].count(constr));

            float shift = shifts_[slack_origin.origin_node][constr];

            float raw_slack = slack_origin.slack;

            float shifted_slack = raw_slack + shift;

            return shifted_slack;
        }

        float shifted_max_req(DomainPair constr) {
            VTR_ASSERT(max_req_relaxed_.count(constr));

            float max_req_relaxed = max_req_relaxed_[constr];

            return max_req_relaxed;
        }

    private:
        const tatum::TimingGraph& tg_;
        const DelayCalc& dc_;

        std::map<tatum::NodeId,std::map<DomainPair,float>> shifts_;
        std::map<DomainPair,float> max_req_relaxed_;
};

//Convenience wrapper for automatic template deduction
template<class DelayCalc>
OptimizerSlacks<DelayCalc> make_optimizer_slacks(const AtomNetlist& netlist, 
                        const AtomMap& atom_map, 
                        std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer, 
                        const tatum::TimingGraph& tg,
                        const DelayCalc& dc) {
    return OptimizerSlacks<DelayCalc>(netlist, atom_map, analyzer, tg, dc);
}


#endif
