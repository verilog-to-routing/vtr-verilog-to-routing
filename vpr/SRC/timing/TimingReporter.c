#include "TimingReporter.h"

#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

#include "atom_netlist.h"
#include "timing_util.h"
#include "sta_util.hpp"

#include "globals.h"

using tatum::Time;
using tatum::EdgeId;
using tatum::NodeId;
using tatum::DomainId;

TimingReporter::TimingReporter(const TimingGraphNameResolver& name_resolver,
                               std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                               std::shared_ptr<const tatum::TimingConstraints> timing_constraints, 
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
                                         std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer,
                                         size_t npaths) const {
    std::ofstream os(filename);
    report_timing_setup(os, setup_analyzer, npaths);
}

void TimingReporter::report_timing_setup(std::ostream& os, 
                                         std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer,
                                         size_t npaths) const {
    os << "#Timing report of worst " << npaths << " path(s)\n";
    os << "# Unit scale: " << std::setprecision(0) << std::scientific << unit_scale_ << " seconds\n";
    os << "# Output precision: " << precision_ << "\n";
    os << "\n";

    auto paths = collect_worst_setup_paths(setup_analyzer, npaths);

    size_t i = 0;
    for(const auto& path : paths) {
        os << "#Path " << ++i << "\n";
        report_path_setup(os, path);
        os << "\n";
    }

    os << "#End of timing report\n";
}
void TimingReporter::report_path_setup(std::ostream& os, const TimingPath& timing_path) const {
    std::string divider = "--------------------------------------------------------------------------------";

    os << "Startpoint: " << name_resolver_.node_name(timing_path.startpoint()) << " (" << name_resolver_.node_block_type_name(timing_path.startpoint()) << ")\n";
    os << "Endpoint  : " << name_resolver_.node_name(timing_path.endpoint()) << " (" << name_resolver_.node_block_type_name(timing_path.endpoint()) << ")\n";
    os << "Path Type : max [setup]" << "\n";
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
            if(!path_elem.tag.origin_node() && path_elem.tag.type() == tatum::TagType::CLOCK_LAUNCH) {
                Time latency = Time(timing_constraints_->source_latency(timing_path.launch_domain));
                Time orig = path_elem.tag.time() - latency;

                point = "clock " + timing_constraints_->clock_domain_name(timing_path.launch_domain) + " (rise edge)";
                print_path_line(os, point, orig, orig);
                prev_path = orig;

                point = "clock source latency";
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
                float input_constraint = timing_constraints_->input_constraint(path_elem.node, timing_path.capture_domain);
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
               && timing_graph_->edge_type(path_elem.incomming_edge) == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    point += " [clk-to-q]";
            }

            print_path_line(os, point, incr, path);

            prev_path = path;
        }

        //The last tag is the final arrival time
        const auto& last_tag = timing_path.data_launch[timing_path.data_launch.size() - 1].tag;
        VTR_ASSERT(last_tag.type() == tatum::TagType::DATA_ARRIVAL);
        arr_time = last_tag.time();
        print_path_line(os, "data arrival time", arr_time);
        os << "\n";
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
                VTR_ASSERT(!path_elem.tag.origin_node());
                VTR_ASSERT(path_elem.tag.type() == tatum::TagType::CLOCK_CAPTURE);

                Time latency = Time(timing_constraints_->source_latency(timing_path.launch_domain));
                Time orig = path_elem.tag.time() - latency;
                prev_path = orig;

                std::string point = "clock " + timing_constraints_->clock_domain_name(timing_path.capture_domain) + " (rise edge)";
                print_path_line(os, point, orig, orig);

                path = path_elem.tag.time();
                Time incr = path - prev_path;
                print_path_line(os, "clock source latency", incr, path);
            } else if (ielem == timing_path.clock_capture.size() - 1) {
                //End
                VTR_ASSERT(!path_elem.tag.origin_node());
                VTR_ASSERT(path_elem.tag.type() == tatum::TagType::DATA_REQUIRED);
                
                //Uncertainty
                Time uncertainty = -Time(timing_constraints_->setup_clock_uncertainty(timing_path.launch_domain, timing_path.capture_domain));
                path += uncertainty;
                print_path_line(os, "clock uncertainty", uncertainty, path);

                //Output constraint
                Time output_constraint = -Time(timing_constraints_->output_constraint(path_elem.node, timing_path.capture_domain));
                if(!std::isnan(output_constraint.value())) {
                    path += output_constraint;
                    print_path_line(os, "output external delay", output_constraint, path);
                }

                //Final arrival time
                req_time = path_elem.tag.time();
                print_path_line(os, "data required time", req_time);

            } else {
                //Intermediate
                VTR_ASSERT(path_elem.tag.type() == tatum::TagType::CLOCK_CAPTURE);
                VTR_ASSERT(path_elem.incomming_edge);

                tatum::EdgeType edge_type = timing_graph_->edge_type(path_elem.incomming_edge);

                std::string point;
                if(edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    point = "cell setup time";
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

std::vector<TimingPath> TimingReporter::collect_worst_setup_paths(std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer, size_t npaths) const {
    std::vector<TimingPath> paths;

    //Sort the sinks by slack
    std::map<tatum::TimingTag,NodeId,TimingTagValueComp> tags_and_sinks;

    //Add the slacks of all sink
    for(NodeId node : timing_graph_->logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer->setup_slacks(node)) {
            tags_and_sinks.emplace(tag,node);
        }
    }

    //Trace the paths for each tag/node pair
    // The the map will sort the nodes and tags from worst to best slack (i.e. ascending slack order),
    // so the first pair is the most critical end-point
    for(const auto& kv : tags_and_sinks) {
        NodeId sink_node;
        tatum::TimingTag sink_tag;
        std::tie(sink_tag, sink_node) = kv;

        TimingPath path = trace_setup_path(setup_analyzer, sink_tag, sink_node); 

        paths.push_back(path);

        if(paths.size() >= npaths) break;
    }

    return paths;
}

TimingPath TimingReporter::trace_setup_path(std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer, 
                                            const tatum::TimingTag& sink_tag, 
                                            const NodeId sink_node) const {
    VTR_ASSERT(timing_graph_->node_type(sink_node) == tatum::NodeType::SINK);
    VTR_ASSERT(sink_tag.type() == tatum::TagType::SLACK);

    TimingPath path;
    path.launch_domain = sink_tag.launch_clock_domain();
    path.capture_domain = sink_tag.capture_clock_domain();

    //Trace the data launch path
    NodeId curr_node = sink_node;
    while(curr_node && timing_graph_->node_type(curr_node) != tatum::NodeType::CPIN) {
        //Trace until we hit the origin, or a clock pin

        if(curr_node == sink_node) {
            //Record the slack at the sink node
            auto slack_tags = setup_analyzer->setup_slacks(curr_node);
            auto iter = find_tag(slack_tags, path.launch_domain, path.capture_domain);
            VTR_ASSERT(iter != slack_tags.end());

            path.slack_tag = *iter;
        }

        auto data_tags = setup_analyzer->setup_tags(curr_node, tatum::TagType::DATA_ARRIVAL);

        //First try to find the exact tag match
        auto iter = find_tag(data_tags, path.launch_domain, path.capture_domain);
        if(iter == data_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(data_tags, path.launch_domain, DomainId::INVALID());
        }
        VTR_ASSERT(iter != data_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph_->find_edge(iter->origin_node(), curr_node);
            VTR_ASSERT(edge);
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
        VTR_ASSERT(timing_graph_->node_type(curr_node) == tatum::NodeType::CPIN);

        while(curr_node) {
            auto launch_tags = setup_analyzer->setup_tags(curr_node, tatum::TagType::CLOCK_LAUNCH);
            auto iter = find_tag(launch_tags, path.launch_domain, path.capture_domain);
            VTR_ASSERT(iter != launch_tags.end());

            EdgeId edge;
            if(iter->origin_node()) {
                edge = timing_graph_->find_edge(iter->origin_node(), curr_node);
                VTR_ASSERT(edge);
            }

            //Record
            path.clock_launch.emplace_back(*iter, curr_node, edge);

            //Advance to the previous node
            curr_node = iter->origin_node();
        }
    } else {
        //From the primary input launch tag
        VTR_ASSERT(path.data_launch.size() > 0);
        tatum::NodeId launch_node = path.data_launch[0].node;
        auto clock_launch_tags = setup_analyzer->setup_tags(launch_node, tatum::TagType::CLOCK_LAUNCH);
        //First try to find the exact tag match
        auto iter = find_tag(clock_launch_tags, path.launch_domain, path.capture_domain);
        if(iter == clock_launch_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(clock_launch_tags, path.launch_domain, DomainId::INVALID());
        }
        VTR_ASSERT(iter != clock_launch_tags.end());

        path.clock_launch.emplace_back(*iter, launch_node, EdgeId::INVALID());

    }
    //Reverse back-trace
    std::reverse(path.clock_launch.begin(), path.clock_launch.end());

    //Trace the clock capture path
    curr_node = sink_node;
    while(curr_node) {

        if(curr_node == sink_node) {
            //Record the required time
            auto required_tags = setup_analyzer->setup_tags(curr_node, tatum::TagType::DATA_REQUIRED);
            auto iter = find_tag(required_tags, path.launch_domain, path.capture_domain);
            VTR_ASSERT(iter != required_tags.end());
            path.clock_capture.emplace_back(*iter, curr_node, EdgeId::INVALID());
        }


        //Record the clock capture tag
        auto capture_tags = setup_analyzer->setup_tags(curr_node, tatum::TagType::CLOCK_CAPTURE);
        auto iter = find_tag(capture_tags, path.launch_domain, path.capture_domain);
        VTR_ASSERT(iter != capture_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph_->find_edge(iter->origin_node(), curr_node);
            VTR_ASSERT(edge);
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


std::string VprTimingGraphNameResolver::node_name(const tatum::NodeId node) const {
    AtomPinId pin = g_atom_lookup.tnode_atom_pin(node);
    return g_atom_nl.pin_name(pin);
}

std::string VprTimingGraphNameResolver::node_block_type_name(const tatum::NodeId node) const {
    AtomPinId pin = g_atom_lookup.tnode_atom_pin(node);
    AtomBlockId blk = g_atom_nl.pin_block(pin);
    return g_atom_nl.block_model(blk)->name;
}
