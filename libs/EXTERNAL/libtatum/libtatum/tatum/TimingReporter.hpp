#ifndef TATUM_TIMING_REPORTER_HPP
#define TATUM_TIMING_REPORTER_HPP
#include <iosfwd>
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/timing_analyzers.hpp"

#include "tatum/TimingGraphNameResolver.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/report/TimingPathCollector.hpp"
#include "tatum/report/TimingReportTagRetriever.hpp"

namespace tatum { namespace detail {

    float convert_to_printable_units(float value, float unit_scale);
    std::string to_printable_string(tatum::Time val, float unit_scale, size_t precision);

    //Helper class to track path state and formatting while writing timing path reports
    class ReportTimingPathHelper {
        public:
            ReportTimingPathHelper(float unit_scale, size_t precision, size_t point_width=60, size_t incr_width=10, size_t path_width=10)
                : unit_scale_(unit_scale)
                , precision_(precision)
                , point_width_(point_width)
                , incr_width_(incr_width)
                , path_width_(path_width) {}

            void update_print_path(std::ostream& os, std::string point, tatum::Time path);

            void update_print_path_no_incr(std::ostream& os, std::string point, tatum::Time path);

            void reset_path();

            void print_path_line_no_incr(std::ostream& os, std::string point, tatum::Time path) const;

            void print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const;

            void print_divider(std::ostream& os) const;

        private:
            float unit_scale_;
            size_t precision_;
            size_t point_width_;
            size_t incr_width_;
            size_t path_width_;

            tatum::Time prev_path_ = tatum::Time(0.);
    };

}} //namespace

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

        void report_skew_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS) const;
        void report_skew_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS) const;

        void report_skew_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS) const;
        void report_skew_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS) const;

        void report_unconstrained_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer) const;
        void report_unconstrained_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer) const;

        void report_unconstrained_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer) const;
        void report_unconstrained_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer) const;

    private:
        struct PathSkew {
            NodeId launch_node;
            NodeId capture_node;
            DomainId launch_domain;
            DomainId capture_domain;
            Time clock_launch;
            Time clock_capture;
            Time clock_capture_normalized;
            Time clock_constraint;
            Time clock_skew;
        };

    private:
        void report_timing(std::ostream& os, const std::vector<TimingPath>& paths) const;

        void report_timing_path(std::ostream& os, const TimingPath& path) const;

        void report_unconstrained(std::ostream& os, const NodeType type, const detail::TagRetriever& tag_retriever) const;

        void report_skew(std::ostream& os, const std::vector<SkewPath>& paths, TimingType timing_type) const;

        void report_skew_path(std::ostream& os, const SkewPath& skew_path, TimingType timing_type) const;

        Time report_timing_clock_launch_subpath(std::ostream& os,
                                                detail::ReportTimingPathHelper& path_helper,
                                                const TimingSubPath& subpath,
                                                DomainId domain,
                                                TimingType timing_type) const;

        Time report_timing_clock_capture_subpath(std::ostream& os,
                                                 detail::ReportTimingPathHelper& path_helper,
                                                 const TimingSubPath& subpath,
                                                 DomainId launch_domain,
                                                 DomainId capture_domain,
                                                 TimingType timing_type) const;

        Time report_timing_data_arrival_subpath(std::ostream& os,
                                                detail::ReportTimingPathHelper& path_helper,
                                                const TimingSubPath& subpath,
                                                DomainId domain,
                                                TimingType timing_type,
                                                Time path) const;

        Time report_timing_data_required_element(std::ostream& os,
                                                 detail::ReportTimingPathHelper& path_helper,
                                                 const TimingPathElem& data_required_elem,
                                                 DomainId capture_domain,
                                                 TimingType timing_type,
                                                 Time path) const;

        //Reports clock latency and path (caller should handle rising edge)
        Time report_timing_clock_subpath(std::ostream& os, 
                                         detail::ReportTimingPathHelper& path_helper,
                                         const TimingSubPath& subpath,
                                         DomainId domain,
                                         TimingType timing_type,
                                         Time path) const;

        bool nearly_equal(const tatum::Time& lhs, const tatum::Time& rhs) const;

        size_t estimate_point_print_width(const TimingPath& path) const;

    private:
        const TimingGraphNameResolver& name_resolver_;
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        float unit_scale_ = 1e-9;
        size_t precision_ = 3;

        float relative_error_tolerance_ = 1.e-5;
        float absolute_error_tolerance_ = 1e-13; //Sub pico-second

        TimingPathCollector path_collector_;
};

} //namespace
#endif
