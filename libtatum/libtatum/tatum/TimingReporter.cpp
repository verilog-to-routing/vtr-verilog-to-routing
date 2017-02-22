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

    os << "Startpoint: " << name_resolver_.node_name(timing_path.startpoint()) << " (" << name_resolver_.node_block_type_name(timing_path.startpoint()) << ")\n";
    os << "Endpoint  : " << name_resolver_.node_name(timing_path.endpoint()) << " (" << name_resolver_.node_block_type_name(timing_path.endpoint()) << ")\n";

    if(timing_path.type == TimingPathType::SETUP) {
        os << "Path Type : max [setup]" << "\n";
    } else {
        TATUM_ASSERT_MSG(timing_path.type == TimingPathType::HOLD, "Expected path type SETUP or HOLD");
        os << "Path Type : min [hold]" << "\n";
    }

    os << "\n";
    print_path_line(os, "Point", " Incr", " Path");
    os << divider << "\n";

    //Launch path
    Time arr_time;
    {
        Time prev_path = Time(0.);
        Time path = Time(0.);

        //Clock
        for(TimingPathElem path_elem : timing_path.clock_launch) {
            std::string point;
            if(!path_elem.tag.origin_node() && path_elem.tag.type() == TagType::CLOCK_LAUNCH) {
                point = "clock " + timing_constraints_.clock_domain_name(timing_path.launch_domain) + " (rise edge)";
                print_path_line(os, point, path - prev_path, path);

                point = "clock source latency";
                Time latency = Time(timing_constraints_.source_latency(timing_path.launch_domain));
                prev_path = path;
                path = latency;

            } else {
                point = name_resolver_.node_name(path_elem.node) + " (" + name_resolver_.node_block_type_name(path_elem.node) + ")";
            }
            path = path_elem.tag.time();
            Time incr = path - prev_path;

            print_path_line(os, point, incr, path);

            prev_path = path;
        }

        //Data
        for(size_t ielem = 0; ielem < timing_path.data_launch.size(); ++ielem) {
            const TimingPathElem& path_elem = timing_path.data_launch[ielem];

            if(ielem == 0) {
                //Start

                //Input constraint
                float input_constraint = timing_constraints_.input_constraint(path_elem.node, timing_path.capture_domain);
                if(!std::isnan(input_constraint)) {
                    path += Time(input_constraint);

                    print_path_line(os, "input external delay", Time(input_constraint), path);

                    prev_path = path;
                }
            }

            std::string point = name_resolver_.node_name(path_elem.node) + " (" + name_resolver_.node_block_type_name(path_elem.node) + ")";
            path = path_elem.tag.time();
            Time incr = path - prev_path;

            if(path_elem.incomming_edge 
               && timing_graph_.edge_type(path_elem.incomming_edge) == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    point += " [clk-to-q]";
            }

            print_path_line(os, point, incr, path);

            prev_path = path;
        }

        //The last tag is the final arrival time
        const auto& last_tag = timing_path.data_launch[timing_path.data_launch.size() - 1].tag;
        TATUM_ASSERT(last_tag.type() == TagType::DATA_ARRIVAL);
        arr_time = last_tag.time();
        print_path_line(os, "data arrival time", arr_time);
        os << "\n";

        //Sanity check the arrival time calculated by this timing report (i.e. path) and the one calculated by
        //the analyzer (i.e. arr_time) agree
        if(!nearly_equal(arr_time, path)) {
            os.flush();
            std::stringstream ss;
            ss << "Internal Error: analyzer arrival time (" << arr_time.value() << ")"
               << " differs from timing report path arrival time (" << path.value() << ")"
               << " beyond tolerance";
            throw tatum::Error(ss.str());
        }
    }


    //Capture path (required time)
    Time req_time;
    {
        Time prev_path = Time(0.);
        Time path = Time(0.);

        for(size_t ielem = 0; ielem < timing_path.clock_capture.size(); ++ielem) {
            const TimingPathElem& path_elem = timing_path.clock_capture[ielem];

            if(ielem == 0) {
                //Start
                TATUM_ASSERT(!path_elem.tag.origin_node());
                TATUM_ASSERT(path_elem.tag.type() == TagType::CLOCK_CAPTURE);

                Time constraint;
                if(timing_path.type == TimingPathType::SETUP) {
                    constraint = Time(timing_constraints_.setup_constraint(timing_path.launch_domain, timing_path.capture_domain));
                } else {
                    constraint = Time(timing_constraints_.hold_constraint(timing_path.launch_domain, timing_path.capture_domain));
                }

                std::string point = "clock " + timing_constraints_.clock_domain_name(timing_path.capture_domain) + " (rise edge)";
                print_path_line(os, point, constraint, constraint);
                prev_path = constraint;

                path = path_elem.tag.time();
                Time incr = path - prev_path;
                print_path_line(os, "clock source latency", incr, path);
            } else if (ielem == timing_path.clock_capture.size() - 1) {
                //End
                TATUM_ASSERT(!path_elem.tag.origin_node());
                TATUM_ASSERT(path_elem.tag.type() == TagType::DATA_REQUIRED);
                
                //Uncertainty
                Time uncertainty;
                
                if(timing_path.type == TimingPathType::SETUP) {
                    uncertainty = -Time(timing_constraints_.setup_clock_uncertainty(timing_path.launch_domain, timing_path.capture_domain));
                } else {
                    uncertainty = Time(timing_constraints_.hold_clock_uncertainty(timing_path.launch_domain, timing_path.capture_domain));
                }
                path += uncertainty;
                print_path_line(os, "clock uncertainty", uncertainty, path);

                //Output constraint
                Time output_constraint = -Time(timing_constraints_.output_constraint(path_elem.node, timing_path.capture_domain));
                if(!std::isnan(output_constraint.value())) {
                    path += output_constraint;
                    print_path_line(os, "output external delay", output_constraint, path);
                }

                //Final arrival time
                req_time = path_elem.tag.time();
                print_path_line(os, "data required time", req_time);

            } else {
                //Intermediate
                TATUM_ASSERT(path_elem.tag.type() == TagType::CLOCK_CAPTURE);
                TATUM_ASSERT(path_elem.incomming_edge);

                EdgeType edge_type = timing_graph_.edge_type(path_elem.incomming_edge);

                std::string point;
                if(edge_type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    if(timing_path.type == TimingPathType::SETUP) {
                        point = "cell setup time";
                    } else {
                        TATUM_ASSERT_MSG(timing_path.type == TimingPathType::HOLD, "Expected path type SETUP or HOLD");
                        point = "cell hold time";
                    }
                } else {
                    //Regular node
                    point = name_resolver_.node_name(path_elem.node) + " (" + name_resolver_.node_block_type_name(path_elem.node) + ")";
                }

                path = path_elem.tag.time();
                Time incr = path - prev_path;
                print_path_line(os, point, incr, path);
            }

            prev_path = path;
        }

        //Sanity check required time
        if(!nearly_equal(req_time, path)) {
            os.flush();
            std::stringstream ss;
            ss << "Internal Error: analyzer required time (" << req_time.value() << ")"
               << " differs from report_timing path required time (" << path.value() << ")"
               << " beyond tolerance";
            throw tatum::Error(ss.str());
        }
    }

    //Summary and slack
    os << divider << "\n";
    print_path_line(os, "data required time", req_time);
    print_path_line(os, "data arrival time", -arr_time);
    os << divider << "\n";
    Time slack = timing_path.slack_tag.time();
    if(slack.value() < 0. || std::signbit(slack.value())) {
        print_path_line(os, "slack (VIOLATED)", slack);
    } else {
        print_path_line(os, "slack (MET)", slack);
    }
    os << "\n";
    os.flush();
}

void TimingReporter::print_path_line(std::ostream& os, std::string point, Time incr, Time path) const {

    print_path_line(os, point, to_printable_string(incr), to_printable_string(path));
}

void TimingReporter::print_path_line(std::ostream& os, std::string point, Time path) const {
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

        TimingPath path = trace_path(tag_retriever, sink_tag, sink_node); 

        paths.push_back(path);

        if(paths.size() >= npaths) break;
    }

    return paths;
}

TimingPath TimingReporter::trace_path(const detail::TagRetriever& tag_retriever, 
                                      const TimingTag& sink_tag, 
                                      const NodeId sink_node) const {
    TATUM_ASSERT(timing_graph_.node_type(sink_node) == NodeType::SINK);
    TATUM_ASSERT(sink_tag.type() == TagType::SLACK);

    TimingPath path;
    path.launch_domain = sink_tag.launch_clock_domain();
    path.capture_domain = sink_tag.capture_clock_domain();
    path.type = tag_retriever.type();

    //Trace the data launch path
    NodeId curr_node = sink_node;
    while(curr_node && timing_graph_.node_type(curr_node) != NodeType::CPIN) {
        //Trace until we hit the origin, or a clock pin

        if(curr_node == sink_node) {
            //Record the slack at the sink node
            auto slack_tags = tag_retriever.slacks(curr_node);
            auto iter = find_tag(slack_tags, path.launch_domain, path.capture_domain);
            TATUM_ASSERT(iter != slack_tags.end());

            path.slack_tag = *iter;
        }

        auto data_tags = tag_retriever.tags(curr_node, TagType::DATA_ARRIVAL);

        //First try to find the exact tag match
        auto iter = find_tag(data_tags, path.launch_domain, path.capture_domain);
        if(iter == data_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(data_tags, path.launch_domain, DomainId::INVALID());
        }
        TATUM_ASSERT(iter != data_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph_.find_edge(iter->origin_node(), curr_node);
            TATUM_ASSERT(edge);
        }

        //Record
        path.data_launch.emplace_back(*iter, curr_node, edge);

        //Advance to the previous node
        curr_node = iter->origin_node();
    }
    //Since we back-traced from sink we reverse to get the forward order
    std::reverse(path.data_launch.begin(), path.data_launch.end());

    //Trace the launch clock path
    if(curr_node) {
        //Through the clock network
        TATUM_ASSERT(timing_graph_.node_type(curr_node) == NodeType::CPIN);

        while(curr_node) {
            auto launch_tags = tag_retriever.tags(curr_node, TagType::CLOCK_LAUNCH);
            auto iter = find_tag(launch_tags, path.launch_domain, path.capture_domain);
            TATUM_ASSERT(iter != launch_tags.end());

            EdgeId edge;
            if(iter->origin_node()) {
                edge = timing_graph_.find_edge(iter->origin_node(), curr_node);
                TATUM_ASSERT(edge);
            }

            //Record
            path.clock_launch.emplace_back(*iter, curr_node, edge);

            //Advance to the previous node
            curr_node = iter->origin_node();
        }
    } else {
        //From the primary input launch tag
        TATUM_ASSERT(path.data_launch.size() > 0);
        NodeId launch_node = path.data_launch[0].node;
        auto clock_launch_tags = tag_retriever.tags(launch_node, TagType::CLOCK_LAUNCH);
        //First try to find the exact tag match
        auto iter = find_tag(clock_launch_tags, path.launch_domain, path.capture_domain);
        if(iter == clock_launch_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(clock_launch_tags, path.launch_domain, DomainId::INVALID());
        }
        TATUM_ASSERT(iter != clock_launch_tags.end());

        path.clock_launch.emplace_back(*iter, launch_node, EdgeId::INVALID());

    }
    //Reverse back-trace
    std::reverse(path.clock_launch.begin(), path.clock_launch.end());

    //Trace the clock capture path
    curr_node = sink_node;
    while(curr_node) {

        if(curr_node == sink_node) {
            //Record the required time
            auto required_tags = tag_retriever.tags(curr_node, TagType::DATA_REQUIRED);
            auto iter = find_tag(required_tags, path.launch_domain, path.capture_domain);
            TATUM_ASSERT(iter != required_tags.end());
            path.clock_capture.emplace_back(*iter, curr_node, EdgeId::INVALID());
        }


        //Record the clock capture tag
        auto capture_tags = tag_retriever.tags(curr_node, TagType::CLOCK_CAPTURE);
        auto iter = find_tag(capture_tags, path.launch_domain, path.capture_domain);
        TATUM_ASSERT(iter != capture_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph_.find_edge(iter->origin_node(), curr_node);
            TATUM_ASSERT(edge);
        }

        path.clock_capture.emplace_back(*iter, curr_node, edge);

        //Advance to the previous node
        curr_node = iter->origin_node();
    }
    std::reverse(path.clock_capture.begin(), path.clock_capture.end());

    return path;
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
