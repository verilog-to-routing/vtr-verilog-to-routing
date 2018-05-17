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

namespace tatum { namespace detail {

float convert_to_printable_units(float value, float unit_scale) {
    return value / unit_scale;
}

std::string to_printable_string(tatum::Time val, float unit_scale, size_t precision) {
    std::stringstream ss;
    if(!std::signbit(val.value())) ss << " "; //Pad possitive values so they align with negative prefixed with -
    ss << std::fixed << std::setprecision(precision) << convert_to_printable_units(val.value(), unit_scale);

    return ss.str();
}

void ReportTimingPathHelper::update_print_path(std::ostream& os, std::string point, tatum::Time path) {

    tatum::Time incr = path - prev_path_;

    print_path_line(os, point, to_printable_string(incr, unit_scale_, precision_), to_printable_string(path, unit_scale_, precision_));

    prev_path_ = path;
}

void ReportTimingPathHelper::update_print_path_no_incr(std::ostream& os, std::string point, tatum::Time path) {
    print_path_line(os, point, "", to_printable_string(path, unit_scale_, precision_));

    prev_path_ = path;
}

void ReportTimingPathHelper::reset_path() {
    prev_path_ = tatum::Time(0.);
}

void ReportTimingPathHelper::print_path_line_no_incr(std::ostream& os, std::string point, tatum::Time path) const {
    print_path_line(os, point, "", to_printable_string(path, unit_scale_, precision_));
}

void ReportTimingPathHelper::print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const {
    os << std::setw(point_width_) << std::left << point;
    os << std::setw(incr_width_) << std::right << incr;
    os << std::setw(path_width_) << std::right << path;
    os << "\n";
}

void ReportTimingPathHelper::print_divider(std::ostream& os) const {
    size_t cnt = point_width_ + incr_width_ + path_width_;
    for(size_t i = 0; i < cnt; ++i) {
        os << "-";
    }
    os << "\n";
}

}} //namespace

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
    auto paths = path_collector_.collect_worst_setup_timing_paths(timing_graph_, setup_analyzer, npaths);

    report_timing(os, paths);
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
    auto paths = path_collector_.collect_worst_hold_timing_paths(timing_graph_, hold_analyzer, npaths);

    report_timing(os, paths);
}

void TimingReporter::report_skew_setup(std::string filename, 
                                         const SetupTimingAnalyzer& setup_analyzer,
                                         size_t nworst) const {
    std::ofstream os(filename);
    report_skew_setup(os, setup_analyzer, nworst);
}

void TimingReporter::report_skew_setup(std::ostream& os, 
                                         const SetupTimingAnalyzer& setup_analyzer,
                                         size_t nworst) const {
    auto paths = path_collector_.collect_worst_setup_skew_paths(timing_graph_, timing_constraints_, setup_analyzer, nworst);

    os << "#Clock skew for setup timing startpoint/endpoint\n";
    os << "\n";
    report_skew(os, paths, TimingType::SETUP);
    os << "#End of clock skew for setup timing startpoint/endpoint report\n";
}

void TimingReporter::report_skew_hold(std::string filename, 
                                         const HoldTimingAnalyzer& hold_analyzer,
                                         size_t nworst) const {
    std::ofstream os(filename);
    report_skew_hold(os, hold_analyzer, nworst);
}

void TimingReporter::report_skew_hold(std::ostream& os, 
                                         const HoldTimingAnalyzer& hold_analyzer,
                                         size_t nworst) const {
    auto paths = path_collector_.collect_worst_hold_skew_paths(timing_graph_, timing_constraints_, hold_analyzer, nworst);

    os << "#Clock skew for hold timing startpoint/endpoint\n";
    os << "\n";
    report_skew(os, paths, TimingType::HOLD);
    os << "#End of clock skew for hold timing startpoint/endpoint report\n";
}

void TimingReporter::report_unconstrained_setup(std::string filename, 
                                                          const tatum::SetupTimingAnalyzer& setup_analyzer) const {
    std::ofstream os(filename);
    report_unconstrained_setup(os, setup_analyzer);
}

void TimingReporter::report_unconstrained_setup(std::ostream& os, 
                                                          const tatum::SetupTimingAnalyzer& setup_analyzer) const {
    detail::SetupTagRetriever tag_retriever(setup_analyzer);

    os << "#Unconstrained setup timing startpoint/endpoint\n";
    os << "\n";
    os << "timing_node_id node_type node_name\n";
    os << "-------------- --------- ---------\n";
    report_unconstrained(os, NodeType::SOURCE, tag_retriever);
    report_unconstrained(os, NodeType::SINK, tag_retriever);
    os << "\n";
    os << "#End of unconstrained setup startpoint/endpoint report\n";
}

void TimingReporter::report_unconstrained_hold(std::string filename, 
                                                         const tatum::HoldTimingAnalyzer& hold_analyzer) const {
    std::ofstream os(filename);
    report_unconstrained_hold(os, hold_analyzer);
}

void TimingReporter::report_unconstrained_hold(std::ostream& os, 
                                                         const tatum::HoldTimingAnalyzer& hold_analyzer) const {
    detail::HoldTagRetriever tag_retriever(hold_analyzer);

    os << "#Unconstrained hold timing startpoint/endpoint\n";
    os << "\n";
    os << "timing_node_id node_type node_name\n";
    os << "-------------- --------- ---------\n";
    report_unconstrained(os, NodeType::SOURCE, tag_retriever);
    report_unconstrained(os, NodeType::SINK, tag_retriever);
    os << "\n";
    os << "#End of unconstrained hold startpoint/endpoint report\n";
}

/*
 * Private member functions
 */

void TimingReporter::report_timing(std::ostream& os,
                                   const std::vector<TimingPath>& paths) const {
    tatum::OsFormatGuard flag_guard(os);

    os << "#Timing report of worst " << paths.size() << " path(s)\n";
    os << "# Unit scale: " << std::setprecision(0) << std::scientific << unit_scale_ << " seconds\n";
    os << "# Output precision: " << precision_ << "\n";
    os << "\n";

    size_t i = 0;
    for(const auto& path : paths) {
        os << "#Path " << ++i << "\n";
        report_timing_path(os, path);
        os << "\n";
    }

    os << "#End of timing report\n";
}

void TimingReporter::report_timing_path(std::ostream& os, const TimingPath& timing_path) const {
    std::string divider = "--------------------------------------------------------------------------------";

    TimingPathInfo path_info = timing_path.path_info();

    os << "Startpoint: " << name_resolver_.node_name(path_info.startpoint()) 
       << " (" << name_resolver_.node_type_name(path_info.startpoint())
        << " clocked by " << timing_constraints_.clock_domain_name(path_info.launch_domain())
        << ")\n";
    os << "Endpoint  : " << name_resolver_.node_name(path_info.endpoint()) 
        << " (" << name_resolver_.node_type_name(path_info.endpoint()) 
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
    detail::ReportTimingPathHelper path_helper(unit_scale_, precision_, point_print_width);
    path_helper.print_path_line(os, "Point", " Incr", " Path");
    path_helper.print_divider(os);


    //Launch path
    Time arr_time;
    Time arr_path;
    {
        path_helper.reset_path();

        arr_path = report_timing_clock_launch_subpath(os, path_helper, timing_path.clock_launch_path(), path_info.launch_domain(), path_info.type());

        arr_path = report_timing_data_arrival_subpath(os, path_helper, timing_path.data_arrival_path(), path_info.launch_domain(), path_info.type(), arr_path);

        {
            //Final arrival time
            const TimingPathElem& path_elem = *(--timing_path.data_arrival_path().elements().end());

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

        req_path = report_timing_clock_capture_subpath(os, path_helper, timing_path.clock_capture_path(), 
                                                       path_info.launch_domain(), path_info.capture_domain(), path_info.type());

        const TimingPathElem& path_elem = timing_path.data_required_element();

        req_path = report_timing_data_required_element(os, path_helper, path_elem,
                                                       path_info.capture_domain(), path_info.type(),
                                                       req_path);
        //Final arrival time
        req_time = path_elem.tag().time();
        path_helper.update_print_path_no_incr(os, "data required time", req_time);


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
    if (path_info.type() == TimingType::SETUP) {
        path_helper.print_path_line_no_incr(os, "data required time", req_time);
        path_helper.print_path_line_no_incr(os, "data arrival time", -arr_time);
    } else {
        TATUM_ASSERT(path_info.type() == TimingType::HOLD);
        path_helper.print_path_line_no_incr(os, "data required time", -req_time);
        path_helper.print_path_line_no_incr(os, "data arrival time", arr_time);
    }
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
    Time path_slack;
    if (path_info.type() == TimingType::SETUP) {
        path_slack = req_path - arr_path;
    } else {
        TATUM_ASSERT(path_info.type() == TimingType::HOLD);
        path_slack = arr_path - req_path;
    }
    if(!nearly_equal(slack, path_slack)) {
        os.flush();
        std::stringstream ss;
        ss << "Internal Error: analyzer slack (" << slack << ")"
           << " differs from report_timing path slack (" << path_slack << ")"
           << " beyond tolerance";
        throw tatum::Error(ss.str());
    }
}

void TimingReporter::report_unconstrained(std::ostream& os, const NodeType type, const detail::TagRetriever& tag_retriever) const {
    for(NodeId node : timing_graph_.nodes()) {
        NodeType node_type = timing_graph_.node_type(node);
        if(node_type == type) {
            auto tags = tag_retriever.tags(node);
            if(!is_constrained(node_type, tags)) {
                os << size_t(node)
                   << " " << name_resolver_.node_type_name(node)
                   << " " << name_resolver_.node_name(node)
                   << "\n";
            }
        }
    }
}

void TimingReporter::report_skew(std::ostream& os, const std::vector<SkewPath>& skew_paths, TimingType timing_type) const {
    tatum::OsFormatGuard flag_guard(os);

    int i = 1;
    for (const auto& skew_path : skew_paths) {
        os << "#Skew Path " << i << "\n";
        report_skew_path(os, skew_path, timing_type); 
        os << "\n";
        ++i;
    }
}

void TimingReporter::report_skew_path(std::ostream& os, const SkewPath& skew_path, TimingType timing_type) const {

    auto& launch_path = skew_path.clock_launch_path;
    auto& capture_path = skew_path.clock_capture_path;

    NodeId launch_node = skew_path.data_launch_node;
    NodeId capture_node = skew_path.data_capture_node;

    os << "Startpoint: " << name_resolver_.node_name(launch_node) 
       << " (" << name_resolver_.node_type_name(launch_node)
       << " clocked by " << timing_constraints_.clock_domain_name(skew_path.launch_domain)
       << ")\n";
    os << "Endpoint  : " << name_resolver_.node_name(capture_node) 
       << " (" << name_resolver_.node_type_name(capture_node) 
       << " clocked by " << timing_constraints_.clock_domain_name(skew_path.capture_domain)
       << ")\n";

    if(timing_type == TimingType::SETUP) {
        os << "Path Type : setup" << "\n";
    } else {
        TATUM_ASSERT_MSG(timing_type == TimingType::HOLD, "Expected path type SETUP or HOLD");
        os << "Path Type : hold" << "\n";
    }
    os << "\n";

    std::string launch_name = name_resolver_.node_name(launch_node) 
                            + " (" 
                            + name_resolver_.node_type_name(launch_node)
                            + ")";
    std::string capture_name = name_resolver_.node_name(capture_node) 
                            + " (" 
                            + name_resolver_.node_type_name(capture_node)
                            + ")";

    size_t point_print_width = std::max(launch_name.size(), capture_name.size());
    point_print_width = std::max(point_print_width, std::string("clock data capture (normalized)").size());

    detail::ReportTimingPathHelper path_helper(unit_scale_, precision_, point_print_width);


    path_helper.print_path_line(os, "Point", " Incr", " Path");
    path_helper.print_divider(os);

    Time data_launch_time = report_timing_clock_launch_subpath(os, path_helper, launch_path, skew_path.launch_domain, timing_type);
    TATUM_ASSERT(nearly_equal(data_launch_time, skew_path.clock_launch_arrival));

    path_helper.update_print_path_no_incr(os, "data launch", data_launch_time);
    os << "\n";

    os << "\n";

    path_helper.reset_path();

    Time data_capture_time = report_timing_clock_capture_subpath(os, path_helper, capture_path, skew_path.launch_domain, skew_path.capture_domain, timing_type);
    TATUM_ASSERT(nearly_equal(data_capture_time, skew_path.clock_capture_arrival));

    path_helper.update_print_path_no_incr(os, "data capture", data_capture_time);
    path_helper.print_divider(os);

    Time clock_constraint;
    if (timing_type == TimingType::SETUP) {
        clock_constraint = timing_constraints_.setup_constraint(skew_path.launch_domain, skew_path.capture_domain);
    } else {
        clock_constraint = timing_constraints_.hold_constraint(skew_path.launch_domain, skew_path.capture_domain);
    }
    path_helper.print_path_line_no_incr(os, "data capture", data_capture_time);
    path_helper.print_path_line_no_incr(os, "clock constraint", -clock_constraint);
    path_helper.print_path_line_no_incr(os, "data launch", -data_launch_time);
    path_helper.print_divider(os);

    Time skew = data_capture_time - clock_constraint - data_launch_time;
    TATUM_ASSERT(nearly_equal(skew, skew_path.clock_skew));
    path_helper.print_path_line_no_incr(os, "skew", skew);
}

Time TimingReporter::report_timing_clock_launch_subpath(std::ostream& os,
                                                        detail::ReportTimingPathHelper& path_helper,
                                                        const TimingSubPath& subpath,
                                                        DomainId domain,
                                                        TimingType timing_type) const {
    Time path(0.);

    {
        //Launch clock origin
        path += Time(0.);
        std::string point = "clock " + timing_constraints_.clock_domain_name(domain) + " (rise edge)";
        path_helper.update_print_path(os, point, path);
    }

    return report_timing_clock_subpath(os, path_helper, subpath, domain, timing_type, path);
}

Time TimingReporter::report_timing_clock_capture_subpath(std::ostream& os,
                                                        detail::ReportTimingPathHelper& path_helper,
                                                        const TimingSubPath& subpath,
                                                        DomainId launch_domain,
                                                        DomainId capture_domain,
                                                        TimingType timing_type) const {
    Time path(0.);

    {
        //Launch clock origin
        if (timing_type == TimingType::SETUP) {
            path += timing_constraints_.setup_constraint(launch_domain, capture_domain);
        } else {
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            path += timing_constraints_.hold_constraint(launch_domain, capture_domain);
        }
        std::string point = "clock " + timing_constraints_.clock_domain_name(capture_domain) + " (rise edge)";
        path_helper.update_print_path(os, point, path);
    }

    path = report_timing_clock_subpath(os, path_helper, subpath, capture_domain, timing_type, path);

    {
        //Uncertainty
        Time uncertainty;
        if(timing_type == TimingType::SETUP) {
            uncertainty = -Time(timing_constraints_.setup_clock_uncertainty(launch_domain, capture_domain));
        } else {
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            uncertainty = Time(timing_constraints_.hold_clock_uncertainty(launch_domain, capture_domain));
        }
        path += uncertainty;
        path_helper.update_print_path(os, "clock uncertainty", path);
    }

    return path;
}

Time TimingReporter::report_timing_clock_subpath(std::ostream& os,
                                                 detail::ReportTimingPathHelper& path_helper,
                                                 const TimingSubPath& subpath,
                                                 DomainId domain,
                                                 TimingType timing_type,
                                                 Time path) const {
    {
        //Launch clock latency
        Time latency;
        if (timing_type == TimingType::SETUP) {
            //Setup clock launches late
            latency = Time(timing_constraints_.source_latency(domain, ArrivalType::LATE));
        } else {
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            //Hold clock launches early
            latency = Time(timing_constraints_.source_latency(domain, ArrivalType::EARLY));
        }
        path += latency;
        std::string point = "clock source latency";
        path_helper.update_print_path(os, point, path);
    }


    DelayType delay_type;
    if (timing_type == TimingType::SETUP) {
        delay_type = DelayType::MAX;
    } else {
        delay_type = DelayType::MIN;
    }

    //Launch clock path
    for(const TimingPathElem& path_elem : subpath.elements()) {

        //Ask the application for a detailed breakdown of the edge delays
        auto delay_breakdown = name_resolver_.edge_delay_breakdown(path_elem.incomming_edge(), delay_type);
        if (!delay_breakdown.components.empty()) {
            //Application provided detailed delay breakdown of edge delay, report it
            for (auto& delay_component : delay_breakdown.components) {
                std::string point = "|";
                if (!delay_component.inst_name.empty()) {
                    point += " " + delay_component.inst_name;
                }
                if (!delay_component.type_name.empty()) {
                    point += " (" + delay_component.type_name + ")";
                }
                path += delay_component.delay;
                path_helper.update_print_path(os, point, path);
            }
            TATUM_ASSERT_MSG(nearly_equal(path, path_elem.tag().time()), "Delay breakdown must match calculated delay");
        }


        std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_type_name(path_elem.node()) + ")";

        path = path_elem.tag().time();

        path_helper.update_print_path(os, point, path);
    }

    return path;
}

Time TimingReporter::report_timing_data_arrival_subpath(std::ostream& os,
                                                        detail::ReportTimingPathHelper& path_helper,
                                                        const TimingSubPath& subpath,
                                                        DomainId domain,
                                                        TimingType timing_type,
                                                        Time path) const {

    {
        //Input constraint
        TATUM_ASSERT(subpath.elements().size() > 0);
        const TimingPathElem& path_elem = *(subpath.elements().begin());

        Time input_constraint;
        if (timing_type == TimingType::SETUP) {
            input_constraint = timing_constraints_.input_constraint(path_elem.node(), domain, DelayType::MAX);
        } else {
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            input_constraint = timing_constraints_.input_constraint(path_elem.node(), domain, DelayType::MIN);
        }

        if(input_constraint.valid()) {
            path += Time(input_constraint);

            path_helper.update_print_path(os, "input external delay", path);
        }
    }

    DelayType delay_type;
    if (timing_type == TimingType::SETUP) {
        delay_type = DelayType::MAX;
    } else {
        delay_type = DelayType::MIN;
    }

    //Launch data
    for(const TimingPathElem& path_elem : subpath.elements()) {

        //Ask the application for a detailed breakdown of the edge delays
        auto delay_breakdown = name_resolver_.edge_delay_breakdown(path_elem.incomming_edge(), delay_type);
        if (!delay_breakdown.components.empty()) {
            //Application provided detailed delay breakdown of edge delay, report it
            for (auto& delay_component : delay_breakdown.components) {
                std::string point = "|";
                if (!delay_component.inst_name.empty()) {
                    point += " " + delay_component.inst_name;
                }
                if (!delay_component.type_name.empty()) {
                    point += " (" + delay_component.type_name + ")";
                }
                path += delay_component.delay;
                path_helper.update_print_path(os, point, path);
            }
            TATUM_ASSERT_MSG(nearly_equal(path, path_elem.tag().time()), "Delay breakdown must match calculated delay");
        }

        std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_type_name(path_elem.node()) + ")";

        EdgeId in_edge = path_elem.incomming_edge();
        if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                point += " [clock-to-output]";
        }

        path = path_elem.tag().time();

        path_helper.update_print_path(os, point, path);
    }
    return path;
}

Time TimingReporter::report_timing_data_required_element(std::ostream& os,
                                                         detail::ReportTimingPathHelper& path_helper,
                                                         const TimingPathElem& data_required_elem,
                                                         DomainId capture_domain,
                                                         TimingType timing_type,
                                                         Time path) const {
    {
        TATUM_ASSERT(timing_graph_.node_type(data_required_elem.node()) == NodeType::SINK);

        //Setup/hold time
        EdgeId in_edge = data_required_elem.incomming_edge();
        if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
            std::string point;
            if(timing_type == TimingType::SETUP) {
                point = "cell setup time";
            } else {
                TATUM_ASSERT_MSG(timing_type == TimingType::HOLD, "Expected path type SETUP or HOLD");
                point = "cell hold time";
            }
            path = data_required_elem.tag().time();
            path_helper.update_print_path(os, point, path);
        }

        //Output constraint
        Time output_constraint;
        if (timing_type == TimingType::SETUP) {
            output_constraint = timing_constraints_.output_constraint(data_required_elem.node(), capture_domain, DelayType::MAX);
        } else {
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            output_constraint = timing_constraints_.output_constraint(data_required_elem.node(), capture_domain, DelayType::MIN);
        }
        if(output_constraint.valid()) {
            path += -Time(output_constraint);
            path_helper.update_print_path(os, "output external delay", path);
        }
    }

    return path;
}

bool TimingReporter::nearly_equal(const Time& lhs, const Time& rhs) const {
    return tatum::util::nearly_equal(lhs.value(), rhs.value(), absolute_error_tolerance_, relative_error_tolerance_);
}

size_t TimingReporter::estimate_point_print_width(const TimingPath& path) const {
    size_t width = 60; //default
    for(auto subpath : {path.clock_launch_path(), path.data_arrival_path(), path.clock_capture_path()}) {
        for(auto elem : subpath.elements()) {
            //Take the longest typical point name
            std::string point = name_resolver_.node_name(elem.node()) + " (" + name_resolver_.node_type_name(elem.node()) + ")";
            point += " [clock-to-output]";

            //Keep the max over all points
            width = std::max(width, point.size());
        }
    }
    return width;
}

} //namespace tatum
