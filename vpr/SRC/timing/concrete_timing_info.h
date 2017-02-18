#ifndef VPR_CONCRETE_TIMING_INFO_H
#define VPR_CONCRETE_TIMING_INFO_H

#include "timing_util.h"
#include "vpr_error.h"
#include "slack_evaluation.h"
#include "globals.h"

//NOTE: These classes should not be used directly but created with the 
//      make_*_timing_info() functions in timing_info.h, and used through
//      their abstract interfaces (SetupTimingInfo, HoldTimingInfo etc.)

template<class DelayCalc>
class ConcreteSetupTimingInfo : public SetupTimingInfo {
    public:
        //Constructors
        ConcreteSetupTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                                std::shared_ptr<const tatum::TimingConstraints> timing_constraints,
                                std::shared_ptr<DelayCalc> delay_calc,
                                std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer)
            : timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calc_(delay_calc)
            , setup_analyzer_(analyzer)
            , slack_crit_(g_atom_nl, g_atom_lookup) {
            //pass
        }

    public:
        //Accessors
        PathInfo least_slack_critical_path() const override {
            return find_least_slack_critical_path_delay(*timing_constraints_, *setup_analyzer_);
        }

        PathInfo longest_critical_path() const override {
            return find_longest_critical_path_delay(*timing_constraints_, *setup_analyzer_);
        }

        std::vector<PathInfo> critical_paths() const override {
            return find_critical_path_delays(*timing_constraints_, *setup_analyzer_);
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

        std::shared_ptr<const tatum::TimingGraph> timing_graph() const  override {
            return timing_graph_;
        }

        std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const  override {
            return timing_constraints_;
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
            //Update the arrival and required times and re-calculate slacks
            double sta_wallclock_time = 0.;
            {
                auto start_time = Clock::now();

                setup_analyzer_->update_timing(); 

                sta_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
            }

            double slack_wallclock_time = 0.;
            {
                auto start_time = Clock::now();

                update_setup_slacks(); 

                slack_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
            }

            //Update global timing analysis stats
            g_timing_analysis_profile_stats.sta_wallclock_time += sta_wallclock_time;
            g_timing_analysis_profile_stats.slack_wallclock_time += slack_wallclock_time;
            g_timing_analysis_profile_stats.num_full_updates += 1;
        }

        void update_setup_slacks() {
            slack_crit_.update_slacks_and_criticalities(*timing_graph_, *setup_analyzer_);
        }

    private:

    private:
        //Data
        std::shared_ptr<const tatum::TimingGraph> timing_graph_;
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calc_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> setup_analyzer_;

        SetupSlackCrit slack_crit_;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

template<class DelayCalc>
class ConcreteHoldTimingInfo : public HoldTimingInfo {
    public:
        //Constructors
        ConcreteHoldTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                               std::shared_ptr<const tatum::TimingConstraints> timing_constraints,
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
        std::shared_ptr<const tatum::TimingGraph> timing_graph() const  override { return timing_graph_; }
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const  override { return timing_constraints_; }

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
        std::shared_ptr<const tatum::TimingGraph> timing_graph_;
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calc_;
        std::shared_ptr<tatum::SetupTimingAnalyzer> hold_analyzer_;

        vtr::vector_map<AtomPinId,float> hold_pin_slacks_;
        vtr::vector_map<AtomPinId,float> hold_pin_criticalities_;
};

template<class DelayCalc>
class ConcreteSetupHoldTimingInfo : public SetupHoldTimingInfo {
    public:
        //Constructors
        ConcreteSetupHoldTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                                    std::shared_ptr<const tatum::TimingConstraints> timing_constraints,
                                    std::shared_ptr<DelayCalc> delay_calc,
                                    std::shared_ptr<tatum::SetupHoldTimingAnalyzer> analyzer)
            : setup_timing_(timing_graph, timing_constraints, setup_hold_analyzer)
            , hold_timing_(timing_graph, timing_constraints, setup_hold_analyzer)
            , setup_hold_analyzer_(analyzer) {
            //pass
        }
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

        //TimingInfo related
        std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override { return setup_hold_analyzer(); }
        std::shared_ptr<const tatum::TimingGraph> timing_graph() const  override { return setup_timing_.timing_graph();; }
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const  override { return setup_timing_.timing_constraints(); }

    public:
        //Mutators

        //Update both setup and hold simultaneously
        //  This is more efficient than calling update_hold() and update_setup() separately, since 
        //  it performs a single combined STA to update both setup and hold (instead of calling it
        //  twice).
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

//No-op version of timing info, useful for algorithms that query timing info when run with timing disabled
class NoOpTimingInfo : public SetupHoldTimingInfo {
    public: //Accessors
        //Setup related
        std::vector<PathInfo> critical_paths() const override { return std::vector<PathInfo>(); }
        PathInfo least_slack_critical_path() const override { return PathInfo(); }
        PathInfo longest_critical_path() const override { return PathInfo(); }

        float setup_total_negative_slack() const override { return 0.; }
        float setup_worst_negative_slack() const override { return 0.; }

        float setup_pin_slack(AtomPinId /*pin*/) const override { return 0.; }

        float setup_pin_criticality(AtomPinId /*pin*/) const override { 
            /*
             * Use criticality of 1. This makes all nets critical.  
             *
             * Note: There is a big difference between setting pin criticality to 0 compared to 1.
             * If pin criticality is set to 0, then the current path delay is completely ignored 
             * during optimization. By setting pin criticality to 1, the current path delay will 
             * always be considered and optimized for.
             */
            return 1.; 
        }

        std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer() const override { return nullptr; }

        //Hold related
        float hold_total_negative_slack() const override { return 0.; }
        float hold_worst_negative_slack() const override { return 0.; }

        float hold_pin_slack(AtomPinId /*pin*/) const override { return 0.; }
        float hold_pin_criticality(AtomPinId /*pin*/) const override { return 1.; }

        std::shared_ptr<const tatum::HoldTimingAnalyzer> hold_analyzer() const override { return nullptr; }

        //Combined setup-hold related
        std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> setup_hold_analyzer() const override { return nullptr; }

        //TimingInfo related
        std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override { return nullptr; }
        std::shared_ptr<const tatum::TimingGraph> timing_graph() const  override { return nullptr;; }
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const  override { return nullptr; }

    public: //Mutators

        void update() override { }
        void update_hold() override { }
        void update_setup() override { }
};

#endif
