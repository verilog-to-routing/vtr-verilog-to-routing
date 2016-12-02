#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <memory>
#include "atom_netlist.h"
#include "timing_analyzers.hpp"
#include "vtr_flat_map.h"

class SlackEvaluator {
    public:
        SlackEvaluator(const AtomNetlist& netlist, const AtomMap& atom_map)
            : netlist_(netlist)
            , atom_map_(atom_map) {}

        virtual ~SlackEvaluator() = default;

        //Update the slacks
        virtual void update()  = 0;

    protected:
        const AtomNetlist& netlist_;
        const AtomMap& atom_map_;
};

class SetupSlackEvaluator : public SlackEvaluator {
    public:
        SetupSlackEvaluator(const AtomNetlist& netlist, const AtomMap& atom_map, std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : SlackEvaluator(netlist, atom_map)
            , setup_analyzer_(analyzer) {}

        float setup_slack(AtomPinId src_pin, AtomPinId sink_pin) { 
            auto iter = setup_slacks_.find({src_pin, sink_pin});
            if(iter == setup_slacks_.end()) {
                VPR_THROW(VPR_ERROR_TIMING, "Invalid source/sink pin during slack look-up (pins must be connected)");
            }

            return iter->second;
        }

        float setup_criticality(AtomPinId src_pin, AtomPinId sink_pin) { 
            auto iter = setup_criticalities_.find({src_pin, sink_pin});
            if(iter == setup_criticalities_.end()) {
                VPR_THROW(VPR_ERROR_TIMING, "Invalid source/sink pin during criticality look-up (pins must be connected)");
            }

            return iter->second;
        }

        void update() override {
            setup_analyzer_->update_timing();

            update_setup_slacks();
            update_setup_criticalities();
        }

    protected:
        virtual void update_setup_slacks() = 0;
        virtual void update_setup_criticalities() = 0;
    protected:
        //vtr::flat_map<AtomPinId,vtr::flat_map<AtomPinId,float>> setup_slacks_;
        //vtr::flat_map<AtomPinId,vtr::flat_map<AtomPinId,float>> setup_criticalities_;
        vtr::flat_map<std::pair<AtomPinId,AtomPinId>,float> setup_slacks_;
        vtr::flat_map<std::pair<AtomPinId,AtomPinId>,float> setup_criticalities_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> setup_analyzer_;
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
class OptimizerSlack : public SetupSlackEvaluator {
    public:
        OptimizerSlack(const AtomNetlist& netlist, const AtomMap& atom_map, 
                       std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer, 
                       const tatum::TimingGraph& tg,
                       const tatum::FixedDelayCalculator& dc)
            : SetupSlackEvaluator(netlist, atom_map, analyzer)
            , tg_(tg)
            , dc_(dc)
            , raw_edge_slacks_(tg_.edges().size())
            {}

    private:
        void update_setup_slacks() override {
            update_raw_edge_slacks();

            update_pin_slacks();
        }

        void update_setup_criticalities() override {
            update_edge_shifts();

            update_pin_criticalities();
        }

    private:

        struct DomainPair {
            tatum::DomainId launch;
            tatum::DomainId capture;

            friend bool operator<(const DomainPair& lhs, const DomainPair& rhs) {
                if(lhs.launch < rhs.launch) return true;
                return lhs.capture < rhs.capture;
            }
        };

        struct SlackOrigin {
            float slack;
            tatum::NodeId origin_node;
        };

        void update_raw_edge_slacks() {
            for(tatum::EdgeId edge : tg_.edges()) {
                raw_edge_slacks_[edge].clear(); //Reset if updating

                tatum::NodeId src_node = tg_.edge_src_node(edge); 
                tatum::NodeId sink_node = tg_.edge_sink_node(edge); 

                std::vector<std::pair<DomainPair,SlackOrigin>> slacks;

                for(const tatum::TimingTag& src_tag : setup_analyzer_->setup_tags(src_node, tatum::TagType::DATA_ARRIVAL)) {
                    float T_arr_src = src_tag.time().value();

                    for(const tatum::TimingTag& sink_tag : setup_analyzer_->setup_tags(sink_node, tatum::TagType::DATA_REQUIRED)) {
                        if(sink_tag.launch_clock_domain() != sink_tag.launch_clock_domain()) continue;

                        float T_req_sink = sink_tag.time().value();

                        float delay = dc_.max_edge_delay(tg_, edge).value();
                        VTR_ASSERT(!std::isnan(delay));

                        float slack = T_req_sink - T_arr_src - delay;

                        DomainPair domains = {src_tag.launch_clock_domain(), sink_tag.launch_clock_domain()};
                        SlackOrigin slack_origin = {slack, sink_tag.origin_node()};
                        slacks.emplace_back(domains, slack_origin);
                    }
                }

                raw_edge_slacks_[edge] = vtr::make_flat_map(std::move(slacks));
            }
        }

        void update_edge_shifts() {

            //Record the worst arrival times per launch domain
            std::map<tatum::DomainId,float> max_arr;
            for(tatum::NodeId node : tg_.primary_outputs()) {
                for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_ARRIVAL)) {
                    float arr_time = tag.time().value();

                    tatum::DomainId launch = tag.launch_clock_domain();

                    if(max_arr.count(launch)) {
                        max_arr[launch] = std::max(max_arr[launch], arr_time);
                    } else {
                        max_arr[launch] = arr_time;
                    }
                }
            }

            //Determine the shift requried to relax required times and the maximum required time per domain
            for(tatum::NodeId node : tg_.primary_outputs()) {
                for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                    DomainPair domains = {tag.launch_clock_domain(), tag.capture_clock_domain()};

                    float req_time = tag.time().value();

                    float shift = std::max(0.f, max_arr[tag.launch_clock_domain()] - req_time);

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

        void update_pin_criticalities() {
            setup_criticalities_.clear();

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
            
            setup_criticalities_ = vtr::make_flat_map(std::move(criticalities));
        }

        void update_pin_slacks() {
            std::vector<std::pair<std::pair<AtomPinId,AtomPinId>,float>> slacks;

            for(tatum::EdgeId edge : tg_.edges()) {
                tatum::NodeId src_node = tg_.edge_src_node(edge);
                tatum::NodeId sink_node = tg_.edge_sink_node(edge);

                AtomPinId src_pin = atom_map_.pin_tnode[src_node];
                AtomPinId sink_pin = atom_map_.pin_tnode[sink_node];

                float min_slack = NAN;

                for(const auto& kv : raw_edge_slacks_[edge]) {
                    float slack = kv.second.slack;
                    if(std::isnan(min_slack) || slack < min_slack) {
                        min_slack = slack; 
                    }
                }

                auto key = std::make_pair(src_pin, sink_pin);
                slacks.push_back({key,min_slack});
            }
            setup_slacks_ = vtr::make_flat_map(slacks);
        }

        float criticality(const tatum::EdgeId edge) {
            float max_criticality = NAN;

            //Maximum criticality over all constraints using per-constraint
            //relaxed required time
            for(const auto& kv : raw_edge_slacks_[edge]) {
                const auto& domains = kv.first;
                const auto& slack_origin = kv.second;

                float slack = shifted_slack(domains, slack_origin);
                float max_req = shifted_max_req(domains);

                float criticality = 1. - (slack / max_req);

                if(std::isnan(max_criticality) || max_criticality < criticality) {
                    max_criticality = criticality;
                }
            }

            VTR_ASSERT(raw_edge_slacks_[edge].empty() || !std::isnan(max_criticality));

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
        const tatum::FixedDelayCalculator& dc_;

        vtr::linear_map<tatum::EdgeId,vtr::flat_map<DomainPair,SlackOrigin>> raw_edge_slacks_;
        std::map<tatum::NodeId,std::map<DomainPair,float>> shifts_;
        std::map<DomainPair,float> max_req_relaxed_;
};


#endif
