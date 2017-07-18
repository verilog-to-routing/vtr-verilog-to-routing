#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "tatum/util/tatum_math.hpp"
#include "tatum/util/OsFormatGuard.hpp"
#include "tatum/error.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"

namespace { //File-scope utilities

float convert_to_printable_units(float value, float unit_scale) {
    return value / unit_scale;
}

std::string to_printable_string(tatum::Time val, float unit_scale, size_t precision) {
    std::stringstream ss;
    if(!std::signbit(val.value())) ss << " "; //Pad possitive values so they align with negative prefixed with -
    ss << std::fixed << std::setprecision(precision) << convert_to_printable_units(val.value(), unit_scale);

    return ss.str();
}

//Helper class to track path state and formatting while writing timing path reports
class ReportTimingPathHelper {
    public:
        ReportTimingPathHelper(float unit_scale, size_t precision, size_t point_width=60, size_t incr_width=10, size_t path_width=10)
            : unit_scale_(unit_scale)
            , precision_(precision)
            , point_width_(point_width)
            , incr_width_(incr_width)
            , path_width_(path_width) {}


        void update_print_path(std::ostream& os, std::string point, tatum::Time path) {

            tatum::Time incr = path - prev_path_;

            print_path_line(os, point, to_printable_string(incr, unit_scale_, precision_), to_printable_string(path, unit_scale_, precision_));

            prev_path_ = path;
        }

        void update_print_path_no_incr(std::ostream& os, std::string point, tatum::Time path) {
            print_path_line(os, point, "", to_printable_string(path, unit_scale_, precision_));

            prev_path_ = path;
        }

        void reset_path() {
            prev_path_ = tatum::Time(0.);
        }

        void print_path_line_no_incr(std::ostream& os, std::string point, tatum::Time path) const {
            print_path_line(os, point, "", to_printable_string(path, unit_scale_, precision_));
        }

        void print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const {
            os << std::setw(point_width_) << std::left << point;
            os << std::setw(incr_width_) << std::right << incr;
            os << std::setw(path_width_) << std::right << path;
            os << "\n";
        }

        void print_divider(std::ostream& os) const {
            size_t cnt = point_width_ + incr_width_ + path_width_;
            for(size_t i = 0; i < cnt; ++i) {
                os << "-";
            }
            os << "\n";
        }


    private:
        float unit_scale_;
        size_t precision_;
        size_t point_width_;
        size_t incr_width_;
        size_t path_width_;

        tatum::Time prev_path_ = tatum::Time(0.);
};

} //namespace

namespace tatum {

/*
 * Public member functions
 */

TimingReporter::TimingReporter(const TimingGraphNameResolver& name_resolver,
                               const TimingGraph& timing_graph, 
                               const TimingConstraints& timing_constraints, 
                               float unit_scale,
                               size_t precision)
    : name_resolver_(name_resolver)
    , timing_graph_(timing_graph)
    , timing_constraints_(timing_constraints)
    , unit_scale_(unit_scale)
    , precision_(precision) {
    //pass
}

void TimingReporter::report_timing_setup(std::string filename, 
                                         const SetupTimingAnalyzer& setup_analyzer,
                                         size_t npaths) const {
    std::ofstream os(filename);
    report_timing_setup(os, setup_analyzer, npaths);
}

void TimingReporter::report_timing_setup(std::ostream& os, 
                                         const SetupTimingAnalyzer& setup_analyzer,
                                         size_t npaths) const {
    auto paths = path_collector_.collect_worst_setup_paths(timing_graph_, setup_analyzer, npaths);

    report_timing(os, paths, npaths);
}

void TimingReporter::report_timing_hold(std::string filename, 
                                         const HoldTimingAnalyzer& hold_analyzer,
                                         size_t npaths) const {
    std::ofstream os(filename);
    report_timing_hold(os, hold_analyzer, npaths);
}

void TimingReporter::report_timing_hold(std::ostream& os, 
                                         const HoldTimingAnalyzer& hold_analyzer,
                                         size_t npaths) const {
    auto paths = path_collector_.collect_worst_hold_paths(timing_graph_, hold_analyzer, npaths);

    report_timing(os, paths, npaths);
}

void TimingReporter::report_unconstrained_endpoints_setup(std::string filename, 
                                                          const tatum::SetupTimingAnalyzer& setup_analyzer) const {
    std::ofstream os(filename);
    report_unconstrained_endpoints_setup(os, setup_analyzer);
}

void TimingReporter::report_unconstrained_endpoints_setup(std::ostream& os, 
                                                          const tatum::SetupTimingAnalyzer& setup_analyzer) const {
    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    report_unconstrained_endpoints(os, tag_retriever);
}

void TimingReporter::report_unconstrained_endpoints_hold(std::string filename, 
                                                         const tatum::HoldTimingAnalyzer& hold_analyzer) const {
    std::ofstream os(filename);
    report_unconstrained_endpoints_hold(os, hold_analyzer);
}

void TimingReporter::report_unconstrained_endpoints_hold(std::ostream& os, 
                                                         const tatum::HoldTimingAnalyzer& hold_analyzer) const {
    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    report_unconstrained_endpoints(os, tag_retriever);
}

/*
 * Private member functions
 */

void TimingReporter::report_timing(std::ostream& os,
                                   const std::vector<TimingPath>& paths, 
                                   size_t npaths) const {
    tatum::OsFormatGuard flag_guard(os);

    os << "#Timing report of worst " << npaths << " path(s)\n";
    os << "# Unit scale: " << std::setprecision(0) << std::scientific << unit_scale_ << " seconds\n";
    os << "# Output precision: " << precision_ << "\n";
    os << "\n";

    size_t i = 0;
    for(const auto& path : paths) {
        os << "#Path " << ++i << "\n";
        report_path(os, path);
        os << "\n";
    }

    os << "#End of timing report\n";
}

void TimingReporter::report_path(std::ostream& os, const TimingPath& timing_path) const {
    std::string divider = "--------------------------------------------------------------------------------";

    TimingPathInfo path_info = timing_path.path_info();

    os << "Startpoint: " << name_resolver_.node_name(path_info.startpoint()) 
       << " (" << name_resolver_.node_block_type_name(path_info.startpoint())
        << " clocked by " << timing_constraints_.clock_domain_name(path_info.launch_domain())
        << ")\n";
    os << "Endpoint  : " << name_resolver_.node_name(path_info.endpoint()) 
        << " (" << name_resolver_.node_block_type_name(path_info.endpoint()) 
        << " clocked by " << timing_constraints_.clock_domain_name(path_info.capture_domain())
        << ")\n";

    if(path_info.type() == TimingType::SETUP) {
        os << "Path Type : setup" << "\n";
    } else {
        TATUM_ASSERT_MSG(path_info.type() == TimingType::HOLD, "Expected path type SETUP or HOLD");
        os << "Path Type : hold" << "\n";
    }

    os << "\n";

    size_t point_print_width = estimate_point_print_width(timing_path);

    //Helper to track path state and output formatting
    ReportTimingPathHelper path_helper(unit_scale_, precision_, point_print_width);
    path_helper.print_path_line(os, "Point", " Incr", " Path");
    path_helper.print_divider(os);


    //Launch path
    Time arr_time;
    Time arr_path;
    {
        path_helper.reset_path();

        {
            //Launch clock origin
            arr_path = Time(0.);
            std::string point = "clock " + timing_constraints_.clock_domain_name(path_info.launch_domain()) + " (rise edge)";
            path_helper.update_print_path(os, point, arr_path);
        }

        {
            //Launch clock latency
            Time latency = Time(timing_constraints_.source_latency(path_info.launch_domain()));
            arr_path += latency;
            std::string point = "clock source latency";
            path_helper.update_print_path(os, point, arr_path);
        }

        //Launch clock path
        for(const TimingPathElem& path_elem : timing_path.clock_launch_elements()) {
            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";
            arr_path = path_elem.tag().time();

            path_helper.update_print_path(os, point, arr_path);
        }

        {
            //Input constraint
            TATUM_ASSERT(timing_path.data_arrival_elements().size() > 0);
            const TimingPathElem& path_elem = *(timing_path.data_arrival_elements().begin());

            Time input_constraint = timing_constraints_.input_constraint(path_elem.node(), path_info.launch_domain());
            if(input_constraint.valid()) {
                arr_path += Time(input_constraint);

                path_helper.update_print_path(os, "input external delay", arr_path);
            }
        }

        //Launch data
        for(const TimingPathElem& path_elem : timing_path.data_arrival_elements()) {

            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";

            EdgeId in_edge = path_elem.incomming_edge();
            if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    point += " [clock-to-output]";
            }

            arr_path = path_elem.tag().time();

            path_helper.update_print_path(os, point, arr_path);
        }

        {
            //Final arrival time
            const TimingPathElem& path_elem = *(--timing_path.data_arrival_elements().end());

            TATUM_ASSERT(timing_graph_.node_type(path_elem.node()) == NodeType::SINK);

            TATUM_ASSERT(path_elem.tag().type() == TagType::DATA_ARRIVAL);
            arr_time = path_elem.tag().time();
            path_helper.update_print_path_no_incr(os, "data arrival time", arr_time);
            os << "\n";
        }

        //Sanity check the arrival time calculated by this timing report (i.e. path) and the one calculated by
        //the analyzer (i.e. arr_time) agree
        if(!nearly_equal(arr_time, arr_path)) {
            os.flush();
            std::stringstream ss;
            ss << "Internal Error: analyzer arrival time (" << arr_time.value() << ")"
               << " differs from timing report path arrival time (" << arr_path.value() << ")"
               << " beyond tolerance";
            throw tatum::Error(ss.str());
        }
    }


    //Capture path (required time)
    Time req_time;
    Time req_path;
    {
        path_helper.reset_path();

        {
            //Capture clock origin
            req_path = Time(0.);

            Time constraint;
            if(path_info.type() == TimingType::SETUP) {
                constraint = Time(timing_constraints_.setup_constraint(path_info.launch_domain(), path_info.capture_domain()));
            } else {
                constraint = Time(timing_constraints_.hold_constraint(path_info.launch_domain(), path_info.capture_domain()));
            }

            req_path += constraint;
            std::string point = "clock " + timing_constraints_.clock_domain_name(path_info.capture_domain()) + " (rise edge)";
            path_helper.update_print_path(os, point, req_path);
        }

        {
            //Capture clock source latency
            Time latency = Time(timing_constraints_.source_latency(path_info.capture_domain()));
            req_path += latency;
            std::string point = "clock source latency";
            path_helper.update_print_path(os, point, req_path);
        }

        //Clock capture path
        for(const TimingPathElem& path_elem : timing_path.clock_capture_elements()) {
            TATUM_ASSERT(path_elem.tag().type() == TagType::CLOCK_CAPTURE);

            req_path = path_elem.tag().time();
            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";
            path_helper.update_print_path(os, point, req_path);
        }

        //Data required
        {
            const TimingPathElem& path_elem = timing_path.data_required_element();

            TATUM_ASSERT(timing_graph_.node_type(path_elem.node()) == NodeType::SINK);

            //Uncertainty
            Time uncertainty;
            if(path_info.type() == TimingType::SETUP) {
                uncertainty = -Time(timing_constraints_.setup_clock_uncertainty(path_info.launch_domain(), path_info.capture_domain()));
            } else {
                uncertainty = Time(timing_constraints_.hold_clock_uncertainty(path_info.launch_domain(), path_info.capture_domain()));
            }
            req_path += uncertainty;
            path_helper.update_print_path(os, "clock uncertainty", req_path);

            //Setup/hold time
            EdgeId in_edge = path_elem.incomming_edge();
            if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                std::string point;
                if(path_info.type() == TimingType::SETUP) {
                    point = "cell setup time";
                } else {
                    TATUM_ASSERT_MSG(path_info.type() == TimingType::HOLD, "Expected path type SETUP or HOLD");
                    point = "cell hold time";
                }
                req_path = path_elem.tag().time();
                path_helper.update_print_path(os, point, req_path);
            }

            //Output constraint
            Time output_constraint = timing_constraints_.output_constraint(path_elem.node(), path_info.capture_domain());
            if(output_constraint.valid()) {
                req_path += -Time(output_constraint);
                path_helper.update_print_path(os, "output external delay", req_path);
            }

            //Final arrival time
            req_time = path_elem.tag().time();
            path_helper.update_print_path_no_incr(os, "data required time", req_time);
        }


        //Sanity check required time
        if(!nearly_equal(req_time, req_path)) {
            os.flush();
            std::stringstream ss;
            ss << "Internal Error: analyzer required time (" << req_time.value() << ")"
               << " differs from report_timing path required time (" << req_path.value() << ")"
               << " beyond tolerance";
            throw tatum::Error(ss.str());
        }
    }

    //Summary and slack
    path_helper.print_divider(os);
    path_helper.print_path_line_no_incr(os, "data required time", req_time);
    path_helper.print_path_line_no_incr(os, "data arrival time", -arr_time);
    path_helper.print_divider(os);
    Time slack = timing_path.slack_tag().time();
    if(slack.value() < 0. || std::signbit(slack.value())) {
        path_helper.print_path_line_no_incr(os, "slack (VIOLATED)", slack);
    } else {
        path_helper.print_path_line_no_incr(os, "slack (MET)", slack);
    }
    os << "\n";
    os.flush();

    //Sanity check slack
    Time path_slack = req_path - arr_path;
    if(!nearly_equal(slack, path_slack)) {
        os.flush();
        std::stringstream ss;
        ss << "Internal Error: analyzer slack (" << slack << ")"
           << " differs from report_timing path slack (" << path_slack << ")"
           << " beyond tolerance";
        throw tatum::Error(ss.str());
    }
}

void TimingReporter::report_unconstrained_endpoints(std::ostream& os, const detail::TagRetriever& tag_retriever) const {
    os << "#Unconstrained timing endpoints\n";
    os << "\n";

    os << "timing_node_id node_type node_name\n";
    os << "-------------- --------- ---------\n";

    for(NodeId node : timing_graph_.nodes()) {
        NodeType node_type = timing_graph_.node_type(node);
        if(node_type == NodeType::SINK) {
            //An endpoint
            auto tags = tag_retriever.tags(node);
            if(!is_constrained(node_type, tags)) {
                os << size_t(node)
                   << " " << name_resolver_.node_block_type_name(node)
                   << " " << name_resolver_.node_name(node)
                   << "\n";
            }
        }
    }

    os << "\n";
    os << "#End of unconstrained endpoints report\n";
}

bool TimingReporter::nearly_equal(const Time& lhs, const Time& rhs) const {
    return tatum::util::nearly_equal(lhs.value(), rhs.value(), absolute_error_tolerance_, relative_error_tolerance_);
}

size_t TimingReporter::estimate_point_print_width(const TimingPath& path) const {
    size_t width = 60; //default
    for(auto elems : {path.clock_launch_elements(), path.data_arrival_elements(), path.clock_capture_elements()}) {
        for(auto elem : elems) {
            //Take the longest typical point name
            std::string point = name_resolver_.node_name(elem.node()) + " (" + name_resolver_.node_block_type_name(elem.node()) + ")";
            point += " [clock-to-output]";

            //Keep the max over all points
            width = std::max(width, point.size());
        }
    }
    return width;
}

} //namespace tatum
