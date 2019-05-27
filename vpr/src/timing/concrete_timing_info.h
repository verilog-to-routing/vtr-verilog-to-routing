#ifndef VPR_CONCRETE_TIMING_INFO_H
#define VPR_CONCRETE_TIMING_INFO_H

#include "vtr_log.h"
#include "timing_util.h"
#include "vpr_error.h"
#include "slack_evaluation.h"
#include "globals.h"

#include "tatum/report/graphviz_dot_writer.hpp"

void warn_unconstrained(std::shared_ptr<const tatum::TimingAnalyzer> analyzer);

//NOTE: These classes should not be used directly but created with the
//      make_*_timing_info() functions in timing_info.h, and used through
//      their abstract interfaces (SetupTimingInfo, HoldTimingInfo etc.)

template<class DelayCalc>
class ConcreteSetupTimingInfo : public SetupTimingInfo {
  public:
    //Constructors
    ConcreteSetupTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph_v,
                            std::shared_ptr<const tatum::TimingConstraints> timing_constraints_v,
                            std::shared_ptr<DelayCalc> delay_calc,
                            std::shared_ptr<tatum::SetupTimingAnalyzer> analyzer_v)
        : timing_graph_(timing_graph_v)
        , timing_constraints_(timing_constraints_v)
        , delay_calc_(delay_calc)
        , setup_analyzer_(analyzer_v)
        , slack_crit_(g_vpr_ctx.atom().nlist, g_vpr_ctx.atom().lookup) {
        //pass
    }

  public:
    //Accessors
    tatum::TimingPathInfo least_slack_critical_path() const override {
        return find_least_slack_critical_path_delay(*timing_constraints_, *setup_analyzer_);
    }

    tatum::TimingPathInfo longest_critical_path() const override {
        return find_longest_critical_path_delay(*timing_constraints_, *setup_analyzer_);
    }

    std::vector<tatum::TimingPathInfo> critical_paths() const override {
        return tatum::find_critical_paths(*timing_graph_, *timing_constraints_, *setup_analyzer_);
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

    std::shared_ptr<const tatum::TimingGraph> timing_graph() const override {
        return timing_graph_;
    }

    std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const override {
        return timing_constraints_;
    }

    std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer() const override {
        return setup_analyzer_;
    }

  public:
    //Mutators
    void update() override {
        update_setup();

        if (warn_unconstrained_) {
            warn_unconstrained(analyzer());
        }
    }

    void update_setup() override {
        //Update the arrival and required times and re-calculate slacks
        double sta_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            setup_analyzer_->update_setup_timing();

            sta_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        double slack_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            update_setup_slacks();

            slack_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        //Update global timing analysis stats
        auto& timing_ctx = g_vpr_ctx.mutable_timing();
        timing_ctx.stats.sta_wallclock_time += sta_wallclock_time;
        timing_ctx.stats.slack_wallclock_time += slack_wallclock_time;
        timing_ctx.stats.num_full_setup_updates += 1;
    }

    void update_setup_slacks() {
        slack_crit_.update_slacks_and_criticalities(*timing_graph_, *setup_analyzer_);
    }

    void set_warn_unconstrained(bool val) override { warn_unconstrained_ = val; }

  private:
  private:
    //Data
    std::shared_ptr<const tatum::TimingGraph> timing_graph_;
    std::shared_ptr<const tatum::TimingConstraints> timing_constraints_;
    std::shared_ptr<DelayCalc> delay_calc_;
    std::shared_ptr<tatum::SetupTimingAnalyzer> setup_analyzer_;

    SetupSlackCrit slack_crit_;

    bool warn_unconstrained_ = true;

    typedef std::chrono::duration<double> dsec;
    typedef std::chrono::high_resolution_clock Clock;
};

template<class DelayCalc>
class ConcreteHoldTimingInfo : public HoldTimingInfo {
  public:
    //Constructors
    ConcreteHoldTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph_v,
                           std::shared_ptr<const tatum::TimingConstraints> timing_constraints_v,
                           std::shared_ptr<DelayCalc> delay_calc,
                           std::shared_ptr<tatum::HoldTimingAnalyzer> analyzer_v)
        : timing_graph_(timing_graph_v)
        , timing_constraints_(timing_constraints_v)
        , delay_calc_(delay_calc)
        , hold_analyzer_(analyzer_v)
        , slack_crit_(g_vpr_ctx.atom().nlist, g_vpr_ctx.atom().lookup) {
        //pass
    }

  public:
    //Accessors
    float hold_total_negative_slack() const override {
        return find_hold_total_negative_slack(*hold_analyzer_);
    }

    float hold_worst_negative_slack() const override {
        return find_hold_worst_negative_slack(*hold_analyzer_);
    }

    float hold_pin_slack(AtomPinId pin) const override {
        return slack_crit_.hold_pin_slack(pin);
    }

    float hold_pin_criticality(AtomPinId pin) const override {
        return slack_crit_.hold_pin_criticality(pin);
    }

    std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const override { return hold_analyzer(); }
    std::shared_ptr<const tatum::HoldTimingAnalyzer> hold_analyzer() const override { return hold_analyzer_; }
    std::shared_ptr<const tatum::TimingGraph> timing_graph() const override { return timing_graph_; }
    std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const override { return timing_constraints_; }

  public:
    //Mutators
    void update() override {
        update_hold();

        if (warn_unconstrained_) {
            warn_unconstrained(analyzer());
        }
    }

    void update_hold() override {
        double sta_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            hold_analyzer_->update_hold_timing();

            sta_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        double slack_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            update_hold_slacks();

            slack_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        //Update global timing analysis stats
        auto& timing_ctx = g_vpr_ctx.mutable_timing();
        timing_ctx.stats.sta_wallclock_time += sta_wallclock_time;
        timing_ctx.stats.slack_wallclock_time += slack_wallclock_time;
        timing_ctx.stats.num_full_hold_updates += 1;
    }

    void update_hold_slacks() {
        slack_crit_.update_slacks_and_criticalities(*timing_graph_, *hold_analyzer_);
    }

    void set_warn_unconstrained(bool val) override { warn_unconstrained_ = val; }

  private:
    //Data
    std::shared_ptr<const tatum::TimingGraph> timing_graph_;
    std::shared_ptr<const tatum::TimingConstraints> timing_constraints_;
    std::shared_ptr<DelayCalc> delay_calc_;
    std::shared_ptr<tatum::HoldTimingAnalyzer> hold_analyzer_;

    HoldSlackCrit slack_crit_;

    bool warn_unconstrained_ = true;

    typedef std::chrono::duration<double> dsec;
    typedef std::chrono::high_resolution_clock Clock;
};

template<class DelayCalc>
class ConcreteSetupHoldTimingInfo : public SetupHoldTimingInfo {
  public:
    //Constructors
    ConcreteSetupHoldTimingInfo(std::shared_ptr<const tatum::TimingGraph> timing_graph_v,
                                std::shared_ptr<const tatum::TimingConstraints> timing_constraints_v,
                                std::shared_ptr<DelayCalc> delay_calc,
                                std::shared_ptr<tatum::SetupHoldTimingAnalyzer> analyzer_v)
        : setup_timing_(timing_graph_v, timing_constraints_v, delay_calc, analyzer_v)
        , hold_timing_(timing_graph_v, timing_constraints_v, delay_calc, analyzer_v)
        , setup_hold_analyzer_(analyzer_v) {
        //pass
    }

  private:
    //Accessors

    //Setup related
    std::vector<tatum::TimingPathInfo> critical_paths() const override { return setup_timing_.critical_paths(); }
    tatum::TimingPathInfo least_slack_critical_path() const override { return setup_timing_.least_slack_critical_path(); }
    tatum::TimingPathInfo longest_critical_path() const override { return setup_timing_.longest_critical_path(); }

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
    std::shared_ptr<const tatum::TimingGraph> timing_graph() const override {
        return setup_timing_.timing_graph();
        ;
    }
    std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const override { return setup_timing_.timing_constraints(); }

  public:
    //Mutators

    //Update both setup and hold simultaneously
    //  This is more efficient than calling update_hold() and update_setup() separately, since
    //  it performs a single combined STA to update both setup and hold (instead of calling it
    //  twice).
    void update() override {
        double sta_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            setup_hold_analyzer_->update_timing();

            sta_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        double slack_wallclock_time = 0.;
        {
            auto start_time = Clock::now();

            setup_timing_.update_setup_slacks();
            hold_timing_.update_hold_slacks();

            slack_wallclock_time = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        if (warn_unconstrained_) {
            warn_unconstrained(analyzer());
        }

        //Update global timing analysis stats
        auto& timing_ctx = g_vpr_ctx.mutable_timing();
        timing_ctx.stats.sta_wallclock_time += sta_wallclock_time;
        timing_ctx.stats.slack_wallclock_time += slack_wallclock_time;
        timing_ctx.stats.num_full_setup_hold_updates += 1;
    }

    //Update hold only
    void update_hold() override { hold_timing_.update_hold(); }

    //Update setup only
    void update_setup() override { setup_timing_.update_setup(); }

    void set_warn_unconstrained(bool val) override { warn_unconstrained_ = val; }

  private:
    ConcreteSetupTimingInfo<DelayCalc> setup_timing_;
    ConcreteHoldTimingInfo<DelayCalc> hold_timing_;
    std::shared_ptr<tatum::SetupHoldTimingAnalyzer> setup_hold_analyzer_;

    bool warn_unconstrained_ = true;

    typedef std::chrono::duration<double> dsec;
    typedef std::chrono::high_resolution_clock Clock;
};

//No-op version of timing info, useful for algorithms that query timing info when run with timing disabled
class ConstantTimingInfo : public SetupHoldTimingInfo {
  public:
    ConstantTimingInfo(const float criticality)
        : criticality_(criticality) {}

  public: //Accessors
    //Setup related
    std::vector<tatum::TimingPathInfo> critical_paths() const override { return std::vector<tatum::TimingPathInfo>(); }
    tatum::TimingPathInfo least_slack_critical_path() const override { return tatum::TimingPathInfo(); }
    tatum::TimingPathInfo longest_critical_path() const override { return tatum::TimingPathInfo(); }

    float setup_total_negative_slack() const override { return 0.; }
    float setup_worst_negative_slack() const override { return 0.; }

    float setup_pin_slack(AtomPinId /*pin*/) const override { return 0.; }

    float setup_pin_criticality(AtomPinId /*pin*/) const override {
        /*
         * Note: There is a big difference between setting pin criticality to 0 compared to 1.
         * If pin criticality is set to 0, then the current path delay is completely ignored
         * during optimization. By setting pin criticality to 1, the current path delay will
         * always be considered and optimized for.
         */
        return criticality_;
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
    std::shared_ptr<const tatum::TimingGraph> timing_graph() const override {
        return nullptr;
        ;
    }
    std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const override { return nullptr; }

    void set_warn_unconstrained(bool /*val*/) override {}

  public: //Mutators
    void update() override {}
    void update_hold() override {}
    void update_setup() override {}

  private:
    float criticality_;

    typedef std::chrono::duration<double> dsec;
    typedef std::chrono::high_resolution_clock Clock;
};
#endif
