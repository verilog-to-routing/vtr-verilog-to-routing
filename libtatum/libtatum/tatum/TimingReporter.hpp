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

//A class for generating timing reports
class TimingReporter {
    public:
        TimingReporter(const TimingGraphNameResolver& name_resolver,
                       const tatum::TimingGraph& timing_graph, 
                       const tatum::TimingConstraints& timing_constraints, 
                       float unit_scale=1e-9,
                       size_t precision=3);
    public:
        void report_timing_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=100) const;
        void report_timing_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=100) const;

        void report_timing_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=100) const;
        void report_timing_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=100) const;
    private:
        void report_timing(std::ostream& os, const std::vector<TimingPath>& paths, size_t npaths) const;
        void report_path(std::ostream& os, const TimingPath& path) const;

        void update_print_path(std::ostream& os, std::string point, tatum::Time path) const;
        void update_print_path_no_incr(std::ostream& os, std::string point, tatum::Time path) const;
        void reset_path() const;

        void print_path_line_no_incr(std::ostream& os, std::string point, Time path) const;
        void print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const;

        std::vector<TimingPath> collect_worst_paths(const detail::TagRetriever& tag_retriever, size_t npaths) const;

        float convert_to_printable_units(float) const;

        std::string to_printable_string(tatum::Time val) const;

        bool nearly_equal(const tatum::Time& lhs, const tatum::Time& rhs) const;
    private:
    private:
        const TimingGraphNameResolver& name_resolver_;
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        float unit_scale_ = 1e-9;
        size_t precision_ = 3;

        float relative_error_tolerance_ = 1.e-5;
        float absolute_error_tolerance_ = 1e-13; //Sub pico-second

        mutable Time prev_path_ = Time(0.);
};

} //namespace
#endif
