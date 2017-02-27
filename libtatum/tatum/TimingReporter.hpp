#ifndef TATUM_TIMING_REPORTER_HPP
#define TATUM_TIMING_REPORTER_HPP
#include <iosfwd>
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/timing_analyzers.hpp"

#include "tatum/TimingGraphNameResolver.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/report/TimingReportTagRetriever.hpp"

namespace tatum {

constexpr size_t REPORT_TIMING_DEFAULT_NPATHS=100;

//A class for generating timing reports
class TimingReporter {
    public:
        TimingReporter(const TimingGraphNameResolver& name_resolver,
                       const tatum::TimingGraph& timing_graph, 
                       const tatum::TimingConstraints& timing_constraints, 
                       float unit_scale=1e-9,
                       size_t precision=3);
    public:
        void report_timing_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS) const;
        void report_timing_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS) const;

        void report_timing_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS) const;
        void report_timing_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS) const;

        void report_unconstrained_endpoints_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer) const;
        void report_unconstrained_endpoints_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer) const;

        void report_unconstrained_endpoints_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer) const;
        void report_unconstrained_endpoints_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer) const;
    private:
        void report_timing(std::ostream& os, const std::vector<TimingPath>& paths, size_t npaths) const;
        void report_path(std::ostream& os, const TimingPath& path) const;
        std::vector<TimingPath> collect_worst_paths(const detail::TagRetriever& tag_retriever, size_t npaths) const;

        void report_unconstrained_endpoints(std::ostream& os, const detail::TagRetriever& tag_retriever) const;

        bool nearly_equal(const tatum::Time& lhs, const tatum::Time& rhs) const;

        size_t estimate_point_print_width(const TimingPath& path) const;
    private:
    private:
        const TimingGraphNameResolver& name_resolver_;
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        float unit_scale_ = 1e-9;
        size_t precision_ = 3;

        float relative_error_tolerance_ = 1.e-5;
        float absolute_error_tolerance_ = 1e-13; //Sub pico-second

};

} //namespace
#endif
