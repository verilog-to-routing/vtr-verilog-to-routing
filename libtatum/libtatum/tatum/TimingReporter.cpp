#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "tatum/util/tatum_math.hpp"
#include "tatum/util/tatum_error.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/report/timing_path_tracing.hpp"

namespace tatum {

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
    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    auto paths = collect_worst_paths(tag_retriever, npaths);

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
    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    auto paths = collect_worst_paths(tag_retriever, npaths);

    report_timing(os, paths, npaths);
}

void TimingReporter::report_timing(std::ostream& os,
                                   const std::vector<TimingPath>& paths, 
                                   size_t npaths) const {
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

    if(path_info.type() == TimingPathType::SETUP) {
        os << "Path Type : max [setup]" << "\n";
    } else {
        TATUM_ASSERT_MSG(path_info.type() == TimingPathType::HOLD, "Expected path type SETUP or HOLD");
        os << "Path Type : min [hold]" << "\n";
    }

    os << "\n";
    print_path_line(os, "Point", " Incr", " Path");
    os << divider << "\n";

    //Launch path
    Time arr_time;
    Time arr_path;
    {
        reset_path();

        {
            //Launch clock origin
            arr_path = Time(0.);
            std::string point = "clock " + timing_constraints_.clock_domain_name(path_info.launch_domain()) + " (rise edge)";
            update_print_path(os, point, arr_path);
        }

        {
            //Launch clock latency
            Time latency = Time(timing_constraints_.source_latency(path_info.launch_domain()));
            arr_path += latency;
            std::string point = "clock source latency";
            update_print_path(os, point, arr_path);
        }

        //Launch clock path
        for(const TimingPathElem& path_elem : timing_path.clock_launch_elements()) {
            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";
            arr_path = path_elem.tag().time();

            update_print_path(os, point, arr_path);
        }

        {
            //Input constraint
            TATUM_ASSERT(timing_path.data_arrival_elements().size() > 0);
            const TimingPathElem& path_elem = *(timing_path.data_arrival_elements().begin());

            Time input_constraint = timing_constraints_.input_constraint(path_elem.node(), path_info.launch_domain());
            if(input_constraint.valid()) {
                arr_path += Time(input_constraint);

                update_print_path(os, "input external delay", arr_path);
            }
        }

        //Launch data
        for(const TimingPathElem& path_elem : timing_path.data_arrival_elements()) {

            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";

            EdgeId in_edge = path_elem.incomming_edge();
            if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    point += " [clk-to-q]";
            }

            arr_path = path_elem.tag().time();

            update_print_path(os, point, arr_path);
        }

        {
            //Final arrival time
            const TimingPathElem& path_elem = *(--timing_path.data_arrival_elements().end());

            TATUM_ASSERT(timing_graph_.node_type(path_elem.node()) == NodeType::SINK);

            TATUM_ASSERT(path_elem.tag().type() == TagType::DATA_ARRIVAL);
            arr_time = path_elem.tag().time();
            update_print_path_no_incr(os, "data arrival time", arr_time);
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
        reset_path();

        {
            //Capture clock origin
            req_path = Time(0.);

            Time constraint;
            if(path_info.type() == TimingPathType::SETUP) {
                constraint = Time(timing_constraints_.setup_constraint(path_info.launch_domain(), path_info.capture_domain()));
            } else {
                constraint = Time(timing_constraints_.hold_constraint(path_info.launch_domain(), path_info.capture_domain()));
            }

            req_path += constraint;
            std::string point = "clock " + timing_constraints_.clock_domain_name(path_info.capture_domain()) + " (rise edge)";
            update_print_path(os, point, req_path);
        }

        {
            //Capture clock source latency
            Time latency = Time(timing_constraints_.source_latency(path_info.capture_domain()));
            req_path += latency;
            std::string point = "clock source latency";
            update_print_path(os, point, req_path);
        }

        //Clock capture path
        for(const TimingPathElem& path_elem : timing_path.clock_capture_elements()) {
            TATUM_ASSERT(path_elem.tag().type() == TagType::CLOCK_CAPTURE);

            req_path = path_elem.tag().time();
            std::string point = name_resolver_.node_name(path_elem.node()) + " (" + name_resolver_.node_block_type_name(path_elem.node()) + ")";
            update_print_path(os, point, req_path);
        }

        //Data required
        {
            const TimingPathElem& path_elem = timing_path.data_required_element();

            TATUM_ASSERT(timing_graph_.node_type(path_elem.node()) == NodeType::SINK);

            //Setup/hold time
            EdgeId in_edge = path_elem.incomming_edge();
            if(in_edge && timing_graph_.edge_type(in_edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                std::string point;
                if(path_info.type() == TimingPathType::SETUP) {
                    point = "cell setup time";
                } else {
                    TATUM_ASSERT_MSG(path_info.type() == TimingPathType::HOLD, "Expected path type SETUP or HOLD");
                    point = "cell hold time";
                }
                req_path = path_elem.tag().time();
                update_print_path(os, point, req_path);
            }

            //Output constraint
            Time output_constraint = timing_constraints_.output_constraint(path_elem.node(), path_info.capture_domain());
            if(output_constraint.valid()) {
                req_path += -Time(output_constraint);
                update_print_path(os, "output external delay", req_path);
            }

            //Uncertainty
            Time uncertainty;
            if(path_info.type() == TimingPathType::SETUP) {
                uncertainty = -Time(timing_constraints_.setup_clock_uncertainty(path_info.launch_domain(), path_info.capture_domain()));
            } else {
                uncertainty = Time(timing_constraints_.hold_clock_uncertainty(path_info.launch_domain(), path_info.capture_domain()));
            }
            req_path += uncertainty;
            update_print_path(os, "clock uncertainty", req_path);

            //Final arrival time
            req_time = path_elem.tag().time();
            update_print_path_no_incr(os, "data required time", req_time);
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
    os << divider << "\n";
    print_path_line_no_incr(os, "data required time", req_time);
    print_path_line_no_incr(os, "data arrival time", -arr_time);
    os << divider << "\n";
    Time slack = timing_path.slack_tag().time();
    if(slack.value() < 0. || std::signbit(slack.value())) {
        print_path_line_no_incr(os, "slack (VIOLATED)", slack);
    } else {
        print_path_line_no_incr(os, "slack (MET)", slack);
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

void TimingReporter::update_print_path(std::ostream& os, std::string point, Time path) const {

    Time incr = path - prev_path_;

    print_path_line(os, point, to_printable_string(incr), to_printable_string(path));

    prev_path_ = path;
}

void TimingReporter::update_print_path_no_incr(std::ostream& os, std::string point, Time path) const {
    TATUM_ASSERT(nearly_equal(path, prev_path_));

    print_path_line(os, point, "", to_printable_string(path));

    prev_path_ = path;
}

void TimingReporter::reset_path() const {
    prev_path_ = Time(0.);
}

void TimingReporter::print_path_line_no_incr(std::ostream& os, std::string point, Time path) const {
    print_path_line(os, point, "", to_printable_string(path));
}

void TimingReporter::print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const {
    os << std::setw(60) << std::left << point;
    os << std::setw(10) << std::left << incr;
    os << std::setw(10) << std::left << path;
    os << "\n";
}

std::vector<TimingPath> TimingReporter::collect_worst_paths(const detail::TagRetriever& tag_retriever, size_t npaths) const {
    std::vector<TimingPath> paths;

    //Sort the sinks by slack
    std::map<TimingTag,NodeId,TimingTagValueComp> tags_and_sinks;

    //Add the slacks of all sink
    for(NodeId node : timing_graph_.logical_outputs()) {
        for(TimingTag tag : tag_retriever.slacks(node)) {
            tags_and_sinks.emplace(tag,node);
        }
    }

    //Trace the paths for each tag/node pair
    // The the map will sort the nodes and tags from worst to best slack (i.e. ascending slack order),
    // so the first pair is the most critical end-point
    for(const auto& kv : tags_and_sinks) {
        NodeId sink_node;
        TimingTag sink_tag;
        std::tie(sink_tag, sink_node) = kv;

        TimingPath path = detail::trace_path(timing_graph_, tag_retriever, sink_tag.launch_clock_domain(), sink_tag.capture_clock_domain(), sink_node); 

        paths.push_back(path);

        if(paths.size() >= npaths) break;
    }

    return paths;
}

float TimingReporter::convert_to_printable_units(float value) const {
    return value / unit_scale_;
}

std::string TimingReporter::to_printable_string(Time val) const {
    std::stringstream ss;
    if(!std::signbit(val.value())) ss << " "; //Pad possitive values so they align with negative prefixed with -
    ss << std::fixed << std::setprecision(precision_) << convert_to_printable_units(val.value());

    return ss.str();
}

bool TimingReporter::nearly_equal(const Time& lhs, const Time& rhs) const {
    return tatum::util::nearly_equal(lhs.value(), rhs.value(), absolute_error_tolerance_, relative_error_tolerance_);
}

} //namespace
