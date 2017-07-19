#pragma once
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/graph_visitors/GraphVisitor.hpp"
#include <chrono>
#include <map>

namespace tatum {

/**
 * The abstract base class for all TimingGraphWalkers.
 *
 * TimingGraphWalker encapsulates the process of traversing the timing graph, exposing
 * only the do_*_traversal() methods, which can be called by TimingAnalyzers
 *
 * Internally the do_*_traversal() methods measure record performance related information 
 * and delegate to concrete sub-classes via the do_*_traversal_impl() virtual methods.
 *
 * \see GraphVisitor
 * \see TimingAnalyzer
 */
class TimingGraphWalker {
    public:
        virtual ~TimingGraphWalker() = default;

        ///Performs the arrival time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        void do_arrival_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_arrival_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["arrival_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the required time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        void do_required_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_required_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["required_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the arrival time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        void do_arrival_traversal(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_arrival_traversal_impl(tg, tc, dc, visitor);

            profiling_data_["arrival_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        void do_required_traversal(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_required_traversal_impl(tg, tc, dc, visitor);

            profiling_data_["required_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        void do_reset(const TimingGraph& tg, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_reset_impl(tg, visitor);

            profiling_data_["reset_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        void do_update_slack(const TimingGraph& tg, const DelayCalculator& dc, GraphVisitor& visitor) {
            auto start_time = Clock::now();

            do_update_slack_impl(tg, dc, visitor);

            profiling_data_["update_slack_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Retrieve profiling information
        ///\param key The profiling key
        ///\returns The profiling value for the given key, or NaN if the key is not found
        double get_profiling_data(std::string key) const { 
            auto iter = profiling_data_.find(key);
            if(iter != profiling_data_.end()) {
                return iter->second;
            } else {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }

        void set_profiling_data(std::string key, double val) { 
            profiling_data_[key] = val;
        }

        size_t num_unconstrained_startpoints() const { return num_unconstrained_startpoints_impl(); }
        size_t num_unconstrained_endpoints() const { return num_unconstrained_endpoints_impl(); }

    protected:
        ///Sub-class defined arrival time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        virtual void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) = 0;

        ///Sub-class defined required time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        virtual void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) = 0;

        ///Sub-class defined arrival time traversal
        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        virtual void do_arrival_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) = 0;

        ///Sub-class defined required time traversal
        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        virtual void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) = 0;

        ///Sub-class defined reset in preparation for a timing update
        virtual void do_reset_impl(const TimingGraph& tg, GraphVisitor& visitor) = 0;

        ///Sub-class defined slack calculation
        virtual void do_update_slack_impl(const TimingGraph& tg, const DelayCalculator& dc, GraphVisitor& visitor) = 0;

        virtual size_t num_unconstrained_startpoints_impl() const = 0;
        virtual size_t num_unconstrained_endpoints_impl() const = 0;

    private:
        std::map<std::string, double> profiling_data_;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

} //namepsace
