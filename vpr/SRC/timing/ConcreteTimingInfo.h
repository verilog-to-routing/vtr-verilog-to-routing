#ifndef VPR_CONCRETE_TIMING_INFO_H
#define VPR_CONCRETE_TIMING_INFO_H

#include "timing_util.h"
#include "vpr_error.h"
#include "SlackEvaluator.h"

//NOTE: These classes should not be used directly but created with the 
//      make_*_timing_info() functions in TimingInfo.h, and used through
//      their abstract interfaces (SetupTimingInfo, HoldTimingInfo etc.)

template<class DelayCalc>
class ConcreteSetupTimingInfo : public SetupTimingInfo {
    public:
        //Constructors
        ConcreteSetupTimingInfo(const tatum::TimingGraph& timing_graph, 
                                const tatum::TimingConstraints& timing_constraints,
                                std::shared_ptr<DelayCalc> delay_calc,
                                std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calc_(delay_calc)
            , setup_analyzer_(analyzer)
            , slack_crit_(g_atom_nl, g_atom_map) {
            //pass
        }

    public:
        //Accessors
        PathInfo least_slack_critical_path() const override {
            return find_least_slack_critical_path_delay(timing_constraints_, *setup_analyzer_);
        }

        PathInfo longest_critical_path() const override {
            return find_longest_critical_path_delay(timing_constraints_, *setup_analyzer_);
        }

        std::vector<PathInfo> critical_paths() const override {
            return find_critical_path_delays(timing_constraints_, *setup_analyzer_);
        }

        float setup_total_negative_slack() const override {
            return find_setup_total_negative_slack(*setup_analyzer_);
        }

        float setup_worst_negative_slack() const override {
            return find_setup_worst_negative_slack(*setup_analyzer_);
        }

        float setup_pin_slack(AtomPinId pin) const override {
            return slack_crit_.setup_pin_slack(pin);
        }

        float setup_pin_criticality(AtomPinId pin) const override {
            return slack_crit_.setup_pin_criticality(pin);
        }

        std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override {
            return setup_analyzer();
        }

        std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer() const override {
            return setup_analyzer_;
        }

    public:
        //Mutators
        void update() override {
            update_setup();
        }

        void update_setup() override { 
            setup_analyzer_->update_timing(); 
            update_setup_slacks(); 
        }

        void update_setup_slacks() {
            slack_crit_.update_slacks_and_criticalities(timing_graph_, *setup_analyzer_);
        }

    private:

    private:
        //Data
        const tatum::TimingGraph& timing_graph_;
        const tatum::TimingConstraints& timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calc_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> setup_analyzer_;

        SetupSlackCrit slack_crit_;
};

template<class DelayCalc>
class ConcreteHoldTimingInfo : public HoldTimingInfo {
    public:
        //Constructors
        ConcreteHoldTimingInfo(const tatum::TimingGraph& timing_graph, 
                               const tatum::TimingConstraints& timing_constraints,
                               std::shared_ptr<DelayCalc> delay_calc,
                               std::shared_ptr<tatum::HoldTimingAnalyzer> analyzer)
            : timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calc_(delay_calc)
            , hold_analyzer_(analyzer) {
            //pass
        }

    public:
        //Accessors
        float hold_total_negative_slack() const override { VPR_THROW(VPR_ERROR_TIMING, "Unimplemented"); return NAN; }
        float hold_worst_negative_slack() const override { VPR_THROW(VPR_ERROR_TIMING, "Unimpemented"); return NAN; }

        float hold_pin_slack(AtomPinId pin) const override { return hold_pin_slacks_[pin]; }
        float hold_pin_criticality(AtomPinId pin) const override { return hold_pin_criticalities_[pin]; }

        std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override { return hold_analyzer(); }
        std::shared_ptr<const tatum::HoldTimingAnalyzer> hold_analyzer() const override { return hold_analyzer_; }

    public:
        //Mutators
        void update() override {
            update_hold();
        }

        void update_hold() override { 
            hold_analyzer_->update_timing(); 
            update_hold_slacks(); 
        }

        void update_hold_slacks() {
            VPR_THROW(VPR_ERROR_TIMING, "Unimplemented");
        }

    private:
        //Data
        const tatum::TimingGraph& timing_graph_;
        const tatum::TimingConstraints& timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calc_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> hold_analyzer_;

        vtr::vector_map<AtomPinId,float> hold_pin_slacks_;
        vtr::vector_map<AtomPinId,float> hold_pin_criticalities_;
};

template<class DelayCalc>
class ConcreteSetupHoldTimingInfo : public SetupHoldTimingInfo {
    public:
        //Constructors
        ConcreteSetupHoldTimingInfo(const tatum::TimingGraph& timing_graph, 
                                    const tatum::TimingConstraints& timing_constraints,
                                    std::shared_ptr<DelayCalc> delay_calc,
                                    std::shared_ptr<tatum::SetupHoldTimingAnalyzer> analyzer)
            : setup_timing_(timing_graph, timing_constraints, setup_hold_analyzer)
            , hold_timing_(timing_graph, timing_constraints, setup_hold_analyzer)
            , setup_hold_analyzer_(analyzer) {}
    private:
        //Accessors

        //Setup related
        std::vector<PathInfo> critical_paths() const override { return setup_timing_.critical_paths(); }
        PathInfo least_slack_critical_path() const override { return setup_timing_.least_slack_critical_path(); }
        PathInfo longest_critical_path() const override { return setup_timing_.longest_critical_path(); }

        float setup_total_negative_slack() const override { return setup_timing_.setup_total_negative_slack(); }
        float setup_worst_negative_slack() const override { return setup_timing_.setup_worst_negative_slack(); }

        float setup_pin_slack(AtomPinId pin) const override { return setup_timing_.setup_pin_slack(pin); }
        float setup_pin_criticality(AtomPinId pin) const override { return setup_timing_.setup_pin_criticality(pin); }

        std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer() const override { return setup_timing_.setup_analyzer(); }

        //Hold related
        float hold_total_negative_slack() const override { return hold_timing_.hold_total_negative_slack(); }
        float hold_worst_negative_slack() const override { return hold_timing_.hold_worst_negative_slack(); }

        float hold_pin_slack(AtomPinId pin) const override { return hold_timing_.hold_pin_slack(pin); }
        float hold_pin_criticality(AtomPinId pin) const override { return hold_timing_.hold_pin_criticality(pin); }

        std::shared_ptr<const tatum::HoldTimingAnalyzer> hold_analyzer() const override { return hold_timing_.hold_analyzer(); }

        //Combined setup-hold related
        std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> setup_hold_analyzer() const override { return setup_hold_analyzer_; }

        std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override { return setup_hold_analyzer(); }

    public:
        //Mutators

        //Update both setup and hold simultaneously
        //  This is more efficient since it performs a single combined STA to update both
        //  setup and hold
        void update() override {
            setup_hold_analyzer_->update_timing(); 
            setup_timing_.update_setup(); 
            hold_timing_.update_hold();
        }

        //Update hold only
        void update_hold() override { hold_timing_.update_hold(); }

        //Update setup only
        void update_setup() override { setup_timing_.update_setup(); }

    private:
        ConcreteSetupTimingInfo<DelayCalc> setup_timing_;
        ConcreteSetupTimingInfo<DelayCalc> hold_timing_;
        std::shared_ptr<DelayCalc> delay_calc_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> setup_hold_analyzer_;
};

#endif
