#ifndef TATUM_TIMING_REPORTER_HPP
#define TATUM_TIMING_REPORTER_HPP
#include <iosfwd>
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/timing_analyzers.hpp"

#include "tatum/TimingGraphNameResolver.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/report/TimingPathCollector.hpp"
#include "tatum/report/TimingPathCategory.hpp"
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

/**
 * @brief Generates human readable timing reports (timing paths, clock skew and
 *        unconstrained nodes) from the results of a timing analysis.
 *
 * Each kind of report is available for setup and hold analysis, and can be written to a
 * file (given by its name) or to an output stream. For example:
 *
 *      TimingReporter reporter(name_resolver, timing_graph, timing_constraints);
 *      reporter.report_timing_setup("report_timing.setup.rpt", setup_analyzer);
 */
class TimingReporter {
    public:
        /**
         * @brief Creates a reporter for the given timing graph and constraints.
         *
         *   @param name_resolver       Provides the names of timing graph nodes used in the reports.
         *   @param timing_graph        The analyzed timing graph.
         *   @param timing_constraints  The constraints the timing graph was analyzed with.
         *   @param unit_scale          The time unit of the reported values, in seconds (e.g. 1e-9 for ns).
         *   @param precision           The number of digits reported after the decimal point.
         *
         * The reporter keeps references to the name resolver, timing graph and timing
         * constraints, so they must outlive it.
         */
        TimingReporter(const TimingGraphNameResolver& name_resolver,
                       const tatum::TimingGraph& timing_graph, 
                       const tatum::TimingConstraints& timing_constraints, 
                       float unit_scale=1e-9,
                       size_t precision=3);
    public:
        /**
         * @brief Reports the worst setup timing paths, in detail (i.e. every node along each path).
         *
         * Paths are reported in order of increasing slack (worst first). The report_timing_hold()
         * functions are the hold equivalents.
         *
         *   @param filename            The file to write the report to.
         *   @param setup_analyzer      The setup analysis whose results are reported.
         *   @param npaths              The maximum number of paths to report.
         *   @param category_filter     The categories of paths to report (e.g. only reg2reg).
         *                              Paths are filtered before keeping the npaths worst.
         *                              By default, all paths are reported.
         *
         * Note that only the worst path into each endpoint for each pair of launch/capture
         * clock domains is considered (see TimingPathCollector).
         */
        void report_timing_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but writes the report to the given stream.
        void report_timing_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but also returns the reported paths in paths (which is overwritten).
        void report_timing_setup(std::vector<tatum::TimingPath>& paths, std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;

        /// @brief Reports the worst hold timing paths. See report_timing_setup() for details.
        void report_timing_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but writes the report to the given stream.
        void report_timing_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but also returns the reported paths in paths (which is overwritten).
        void report_timing_hold(std::vector<tatum::TimingPath>& paths, std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;

        /**
         * @brief Reports the clock skew between the launch and capture registers of the timing
         *        paths with the worst setup skew.
         *
         * The report_skew_hold() functions are the hold equivalents.
         *
         *   @param filename            The file to write the report to.
         *   @param setup_analyzer      The setup analysis whose results are reported.
         *   @param nworst              The maximum number of paths to report.
         *   @param category_filter     The categories of paths to report. Since the skew of
         *                              in2reg/reg2out paths includes the full clock insertion
         *                              delay of their register, filtering to only reg2reg
         *                              paths gives the skew between registers. Paths are
         *                              filtered before keeping the nworst worst. By default,
         *                              all paths are reported.
         */
        void report_skew_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but writes the report to the given stream.
        void report_skew_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;

        /// @brief Reports the paths with the worst hold skew. See report_skew_setup() for details.
        void report_skew_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
        /// @brief Same as above, but writes the report to the given stream.
        void report_skew_hold(std::ostream& os, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t nworst=REPORT_TIMING_DEFAULT_NPATHS, const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;

        /**
         * @brief Reports the startpoints (SOURCEs) and endpoints (SINKs) which are not
         *        constrained by the setup analysis (e.g. because they have no clock or
         *        I/O constraint), one per line, along with their type and name.
         *
         * The report_unconstrained_hold() functions are the hold equivalents.
         *
         *   @param filename            The file to write the report to.
         *   @param setup_analyzer      The setup analysis whose results are reported.
         */
        void report_unconstrained_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer) const;
        /// @brief Same as above, but writes the report to the given stream.
        void report_unconstrained_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer) const;

        /// @brief Reports the nodes unconstrained by the hold analysis. See report_unconstrained_setup() for details.
        void report_unconstrained_hold(std::string filename, const tatum::HoldTimingAnalyzer& hold_analyzer) const;
        /// @brief Same as above, but writes the report to the given stream.
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
        void report_timing(std::ostream& os, const std::vector<TimingPath>& paths, const TimingPathCategoryFilter& category_filter) const;

        void report_timing_path(std::ostream& os, const TimingPath& path) const;

        void report_unconstrained(std::ostream& os, const NodeType type, const detail::TagRetriever& tag_retriever) const;

        void report_skew(std::ostream& os, const std::vector<SkewPath>& paths, TimingType timing_type, const TimingPathCategoryFilter& category_filter) const;

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
                                                 NodeId capture_node,
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
                                         Time path,
                                         Time offset) const;

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
