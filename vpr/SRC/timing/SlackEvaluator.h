#ifndef VPR_SLACK_EVALUATOR
#define VPR_SLACK_EVALUATOR
#include <memory>
#include "atom_netlist.h"
#include "timing_analyzers.hpp"

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
            , setup_slack_(netlist_.pins().size(), NAN)
            , setup_criticality_(netlist_.pins().size(), NAN)
            , setup_analyzer_(analyzer) {}

        float setup_slack(AtomPinId pin) { return setup_slack_[pin]; }
        float setup_criticality(AtomPinId pin) { return setup_criticality_[pin]; }

        void update() override {
            setup_analyzer_->update_timing();

            update_setup_slacks();
            update_setup_criticalities();
        }

    protected:
        virtual void update_setup_slacks() = 0;
        virtual void update_setup_criticalities() = 0;
    protected:
        vtr::linear_map<AtomPinId,float> setup_slack_;
        vtr::linear_map<AtomPinId,float> setup_criticality_;
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
        OptimizerSlack(const AtomNetlist& netlist, const AtomMap& atom_map, std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : SetupSlackEvaluator(netlist, atom_map, analyzer)
            , per_constraint_slacks_(netlist_.pins().size()) {}

    private:
        void update_setup_slacks() override {
            for(AtomPinId pin : netlist_.pins()) {

                initialize_per_constraint_info(pin);

                setup_slack_[pin] = pin_slack(pin);
            }
        }

        void update_setup_criticalities() override {
            for(AtomPinId pin : netlist_.pins()) {
                setup_criticality_[pin] = pin_criticality(pin);
            }
        }

    private:

        //Initializes the per-constraint slacks at the specified node
        void initialize_per_constraint_info(const AtomPinId pin) {
            tatum::NodeId node = atom_map_.pin_tnode[pin];

            //Collect the per-domain arrival times
            std::map<tatum::DomainId,float> arr_times;
            for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_ARRIVAL)) {
                arr_times[tag.launch_clock_domain()] = tag.time().value();
            }


            //Calculate the slacks and relaxed required time
            for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                float req_time = tag.time().value();
                float arr_time = arr_times[tag.launch_clock_domain()];
                auto domains = std::make_pair(tag.launch_clock_domain(), tag.capture_clock_domain());


                //Calculate the slack
                float slack = req_time - arr_time;

                //Save it
                per_constraint_slacks_[pin].insert(std::make_pair(domains, slack));


                //Calculate the relaxed required time (Equation 16 of ROMTC)
                float req_relaxed = std::max(req_time, arr_time);

                //Record the maximum relaxed required time per constraint
                if(per_constraint_req_relaxed_.count(domains)) {
                    per_constraint_req_relaxed_[domains] = std::max(req_relaxed, per_constraint_req_relaxed_[domains]);
                } else {
                    per_constraint_req_relaxed_[domains] = req_relaxed;
                }

            }
        }

        //Return the worst-case slack for the specified pin
        float pin_slack(const AtomPinId pin) const {
            //Minimum slack over all constraints (Equation 11 of ROMTC)
            float min_slack = NAN;

            for(const auto& kv : per_constraint_slacks_[pin]) {
                float slack = kv.second;

                if(isnan(min_slack) || slack < min_slack) {
                    min_slack = slack;
                }
            }

            VTR_ASSERT(per_constraint_slacks_[pin].empty() || !isnan(min_slack));

            return min_slack;
        }

        //Return the criticality for the specified pin
        float pin_criticality(const AtomPinId pin) {
            float max_criticality = NAN;

            //Maximum criticality over all constraints using per-constraint
            //relaxed required time (Equation 13 of ROMTC)
            for(const auto& kv : per_constraint_slacks_[pin]) {
                auto domains = kv.first;
                float slack = kv.second;

                float criticality = 1. - (slack / per_constraint_req_relaxed_[domains]);

                if(isnan(max_criticality) || max_criticality < criticality) {
                    max_criticality = criticality;
                }
            }

            VTR_ASSERT(per_constraint_slacks_[pin].empty() || !isnan(max_criticality));

            return max_criticality;
        }

    private:
        typedef std::map<std::pair<tatum::DomainId,tatum::DomainId>,float> PerConstraintValues;

        vtr::linear_map<AtomPinId,PerConstraintValues> per_constraint_slacks_;
        PerConstraintValues per_constraint_req_relaxed_;
};

//AnalysisSlack produces slack and criticality which are appropriate for displaying to an end user.
//If using slack/criticality in an optimizer use OptimizerSlack
#if 0
class AnalysisSlack : public SetupSlackEvaluator {
    public:
        AnalysisSlack(const AtomNelist& netlist, const AtomMap& atom_map, std::unique_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : SetupSlackEvaluator(netlist, atom_map, analyzer) {}

    private:

        void update_slacks() override {
            //Re-calculate slacks
            for(AtomPinId pin : netlist_.pins()) {
                tatum::NodeId node = atom_map_.pin_tnode[pin];

                slack_[pin] = min_node_slack(node);
            }
        }

        void update_criticality() override {
            //According to Equation (12) of "Robust Optimization of Multiple Timing Constriants"
            //

            //We first determine the maximum required time over all constraints
            float max_req = max_req_time();

            //Calculate the criticality
            //  slack_ should have been initialized to the minimum slack 
            //  (accross all constraints) for each pin
            for(AtomPinId pin : netlist_.pins()) {
                criticality_[pin] = 1. - (slack_[pin] / max_req);
            }
        }

    private:
        float min_node_slack(tatum::NodeId node) {
            float node_slack = NAN;


            //Collect the arrival times for each domain
            std::map<tatum::DomainId,float> arr_times;
            for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(tatum::TagType::DATA_ARRIVAL)) {
                arr_times[tag.launch_clock_domain()] = tag.time().value();
            }

            //Calculate the slack for each tag and record the minimum
            for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(tatum::TagType::DATA_REQUIRED)) {
                float req_time = tag.time().value();
                float arr_time = arr_times[tag.launch_clock_domain()];

                float tag_slack = req_time - arr_time;

                if(isnan(node_slack)) {
                    node_slack = tag_slack;
                } else {
                    node_slack = std::min(min_slack, tag_slack);
                }
            }

            return node_slack;
        }

        float max_req_time() {
            float max_req = NAN;
            for(tatum::NodeId node : tg.nodes()) { //TODO: should be able to loop only over output nodes...
                for(const tatum::TimingTag& tag : setup_analyzer_->setup_tags(tatum::TagType::DATA_REQUIRED)) {
                    if(isnan(max_req)) {
                        max_req = tag.time();
                    } else {
                        max_req = std::max(max_req, tag.time());
                    }
                }
            }
            return max_req;
        }
};
#endif

#endif
