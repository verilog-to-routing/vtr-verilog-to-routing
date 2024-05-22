#ifndef VPR_TIMING_INFO_H
#define VPR_TIMING_INFO_H
#include <memory>

#include "timing_info_fwd.h"
#include "tatum/analyzer_factory.hpp"
#include "tatum/timing_paths.hpp"
#include "timing_util.h"

//Generic inteface which provides functionality to update (but not
//access) timing information.
//
//This is useful for algorithms which know they need to update timing
//information (e.g. because they have made a change to the implementation)
//but do not care *what* timing information is updated (e.g. setup vs hold)
class TimingInfo {
  public:
    //Constructors
    virtual ~TimingInfo() = default;

  public:
    //Mutators

    //Invalidate a timing edgge (i.e. due to delay change)
    virtual void invalidate_delay(const tatum::EdgeId edge) = 0;

    //Update all timing information
    virtual void update() = 0;

    //Return the underlying timing analyzer
    virtual std::shared_ptr<const tatum::TimingAnalyzer> analyzer() const = 0;

    //Return the underlying delay calculator
    virtual std::shared_ptr<const tatum::DelayCalculator> delay_calculator() const = 0;

    //Return the underlying timing graph
    virtual std::shared_ptr<const tatum::TimingGraph> timing_graph() const = 0;

    //Return the underlying timing constraints
    virtual std::shared_ptr<const tatum::TimingConstraints> timing_constraints() const = 0;

    //Enable/disable warnings about unconstrained startpoints/endpoints during timing analysis
    virtual void set_warn_unconstrained(bool val) = 0;
};

//Generic interface which provides setup-related timing information
//
//This is useful for algorithms which need to access setup timing related
//information (e.g. to optimize critical path delay)
class SetupTimingInfo : public virtual TimingInfo {
  public:
    //Types
    typedef std::vector<AtomPinId>::const_iterator pin_iterator;
    typedef vtr::Range<pin_iterator> pin_range;

  public:
    //Accessors

    //Returns the underlying setup timing analyzer
    virtual std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer() const = 0;

    //Return the critical path with the least slack
    virtual tatum::TimingPathInfo least_slack_critical_path() const = 0;

    //Return the critical path the the longest absolute delay
    virtual tatum::TimingPathInfo longest_critical_path() const = 0;

    //Return the set of critical paths between all clock domain pairs
    virtual std::vector<tatum::TimingPathInfo> critical_paths() const = 0;

    //Return the total negative slack w.r.t. setup constraints
    virtual float setup_total_negative_slack() const = 0;

    //Return the worst negative slack w.r.t. setup constraints
    virtual float setup_worst_negative_slack() const = 0;

    //Return the worst setup slack of all paths passing through pin
    virtual float setup_pin_slack(AtomPinId pin) const = 0;

    //Return the setup criticality of the worst connection passing through pin
    virtual float setup_pin_criticality(AtomPinId pin) const = 0;

    //Return the range of pins with modified setup slack
    virtual pin_range pins_with_modified_setup_slack() const = 0;
    //
    //Return the range of pins with modified setup criticality
    virtual pin_range pins_with_modified_setup_criticality() const = 0;

  public:
    //Mutators
    virtual void update_setup() = 0;
};

//Generic interface which provides setup-related timing information
//
//This is useful for algorithms which need to access hold timing related
//information (e.g. to fix hold timing)
class HoldTimingInfo : public virtual TimingInfo {
  public:
    //Accessors

    //Returns the underlying hold timing analyzer
    virtual std::shared_ptr<const tatum::HoldTimingAnalyzer> hold_analyzer() const = 0;

    //Return the total negative slack w.r.t. hold constraints
    virtual float hold_total_negative_slack() const = 0;

    //Return the worst negative slack w.r.t. hold constraints
    virtual float hold_worst_negative_slack() const = 0;

    //Return the worst slack of all paths passing through pin
    virtual float hold_pin_slack(AtomPinId pin) const = 0;

    //Return the hold criticality of the worst connection passing through pin
    virtual float hold_pin_criticality(AtomPinId pin) const = 0;

  public:
    //Mutators
    virtual void update_hold() = 0;
};

//Generic interface which provides both setup and hold related timing information
//
//This is useful for algorithms which require access to both setup and hold timing
//information (e.g. simulatneously optimizing setup and hold)
//
//This class supports both the SetupTimingInfo and HoldTimingInfo interfaces and
//can be used in place of them in any algorithm requiring setup or hold related
//information.
//
//Implementation Note:
//  This class uses multiple inheritance, which is OK in this case for the following reasons:
//      * The inheritance is virtual avoiding the diamond problem (i.e. there is only
//        one base TimingInfo class instance)
//      * Both SetupTimingInfo and HoldTimingInfo are purely abstract classes so there
//        is no data to be duplicated
class SetupHoldTimingInfo : public SetupTimingInfo, public HoldTimingInfo {
  public:
    virtual std::shared_ptr<const tatum::SetupHoldTimingAnalyzer> setup_hold_analyzer() const = 0;
};

#endif
