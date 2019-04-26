#pragma once
#include <string>

namespace tatum {

/**
 * TimingAnalyzer represents an abstract interface for all timing analyzers,
 * which can be:
 *   - updated (update_timing())
 *   - reset (reset_timing()).
 *
 * This is the most abstract interface provided (it does not allow access
 * to any calculated data).  As a result this interface is suitable for
 * code that needs to update timing analysis, but does not *use* the
 * analysis results itself.
 *
 * If you need the analysis results you should be using one of the dervied
 * classes.
 *
 * \see SetupTimingAnalyzer
 * \see HoldTimingAnalyzer
 * \see SetupHoldTimingAnalyzer
 */
class TimingAnalyzer {
    public:
        virtual ~TimingAnalyzer() {}

        ///Perform timing analysis to update timing information (i.e. arrival & required times)
        void update_timing() { update_timing_impl(); }

        double get_profiling_data(std::string key) const { return get_profiling_data_impl(key); }

        virtual size_t num_unconstrained_startpoints() const { return num_unconstrained_startpoints_impl(); }
        virtual size_t num_unconstrained_endpoints() const { return num_unconstrained_endpoints_impl(); }

    protected:
        virtual void update_timing_impl() = 0;

        virtual double get_profiling_data_impl(std::string key) const = 0;

        virtual size_t num_unconstrained_startpoints_impl() const = 0;
        virtual size_t num_unconstrained_endpoints_impl() const = 0;
};

} //namepsace
