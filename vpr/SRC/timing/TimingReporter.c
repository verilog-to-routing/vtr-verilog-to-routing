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

using tatum::Time;
using tatum::EdgeId;
using tatum::NodeId;
using tatum::DomainId;

TimingReporter::TimingReporter(const AtomNetlist& netlist,
                               const AtomMap& netlist_map,
                               std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                               std::shared_ptr<const tatum::TimingConstraints> timing_constraints, 
                               std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer,
                               float unit_scale,
                               size_t precision)
    : netlist_(netlist)
    , netlist_map_(netlist_map)
    , timing_graph_(timing_graph)
    , timing_constraints_(timing_constraints)
    , setup_analyzer_(setup_analyzer)
    , unit_scale_(unit_scale)
    , precision_(precision) {
    //pass
}

void TimingReporter::report_timing(std::string filename, 
                                   size_t npaths) const {
    std::ofstream os(filename);
    report_timing(os, npaths);
}

void TimingReporter::report_timing(std::ostream& os, 
                                   size_t npaths) const {
    os << "#Timing report of worst " << npaths << " path(s)\n";
    os << "# Unit scale: " << std::setprecision(0) << std::scientific << unit_scale_ << " seconds\n";
    os << "# Output precision: " << precision_ << "\n";
    os << "\n";

    auto paths = collect_worst_paths(npaths);

    for(const auto& path : paths) {
        report_path(os, path);
        os << "\n";
    }
}
void TimingReporter::report_path(std::ostream& os, const TimingPath& timing_path) const {
    std::string divider = "--------------------------------------------------------------------------------";

    AtomPinId startpin = netlist_map_.pin_tnode[timing_path.startpoint()];
    AtomPinId endpin = netlist_map_.pin_tnode[timing_path.endpoint()];



    os << "Startpoint: " << netlist_.pin_name(startpin) << "\n";
    os << "Endpoint  : " << netlist_.pin_name(endpin) << "\n";
    os << "Path Type : max (setup)" << "\n";
    os << "\n";
    print_path_line(os, "Point", " Incr", " Path");
    os << divider << "\n";
    //Launch path
    Time arr_time;
    {
        Time prev_path = Time(0.);
        Time path = Time(0.);

        //Clock
        if(timing_path.clock_launch.size() > 0) {
            std::string point = "clock " + timing_constraints_->clock_domain_name(timing_path.launch_domain) + " (rise edge)";
            print_path_line(os, point, Time(0.), Time(0.));

            for(TimingPathElem path_elem : timing_path.clock_launch) {
                AtomPinId pin = netlist_map_.pin_tnode[path_elem.node];
                AtomBlockId blk = netlist_.pin_block(pin);

                point = netlist_.pin_name(pin) + " (" + netlist_.block_model(blk)->name + ")";
                path = path_elem.tag.time();
                Time incr = path - prev_path;

                print_path_line(os, point, incr, path);

                prev_path = path;
            }
        }

        //Data
        for(TimingPathElem path_elem : timing_path.data_launch) {
            AtomPinId pin = netlist_map_.pin_tnode[path_elem.node];
            AtomBlockId blk = netlist_.pin_block(pin);

            std::string point = netlist_.pin_name(pin) + " (" + netlist_.block_model(blk)->name + ")";
            path = path_elem.tag.time();
            Time incr = path - prev_path;
            if(path_elem.incomming_edge 
               && timing_graph_->edge_type(path_elem.incomming_edge) == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    point += " [clk-to-q]";
            }


            print_path_line(os, point, incr, path);

            prev_path = path;
        }
        arr_time = path;
        print_path_line(os, "data arrival time", arr_time);
        os << "\n";
    }

    //Capture path (required time)
    Time req_time;
    {
        Time prev_path = Time(0.);
        Time path = Time(0.);

        std::string point = "clock " + timing_constraints_->clock_domain_name(timing_path.capture_domain) + " (rise edge)";
        print_path_line(os, point, path, path);

        for(TimingPathElem path_elem : timing_path.clock_capture) {
            AtomPinId pin = netlist_map_.pin_tnode[path_elem.node];
            AtomBlockId blk = netlist_.pin_block(pin);

            point = netlist_.pin_name(pin) + " (" + netlist_.block_model(blk)->name + ")";
            path = path_elem.tag.time();
            Time incr = path - prev_path;

            if(path_elem.incomming_edge 
               && timing_graph_->edge_type(path_elem.incomming_edge) == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    point = "cell setup time";
            }

            print_path_line(os, point, incr, path);

            prev_path = path;
        }

        Time incr = Time(timing_constraints_->setup_constraint(timing_path.launch_domain, timing_path.capture_domain));
        path += incr;
        print_path_line(os, "constraint", incr, path);

        req_time = path;
        print_path_line(os, "data required time", req_time);
    }

    //Summary and slack
    os << divider << "\n";
    print_path_line(os, "data required time", req_time);
    print_path_line(os, "data arrival time", -arr_time);
    os << divider << "\n";
    Time slack = req_time - arr_time;
    if(slack.value() > 0) {
        print_path_line(os, "slack (MET)", slack);
    } else {
        print_path_line(os, "slack (VIOLATED)", slack);
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

std::vector<TimingPath> TimingReporter::collect_worst_paths(size_t npaths) const {
    std::vector<TimingPath> paths;

    //Sort the sinks by slack
    std::map<tatum::TimingTag,NodeId,TimingTagValueComp> tags_and_sinks;

    //Add the slacks of all sink
    for(NodeId node : timing_graph_->logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer_->setup_slacks(node)) {
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

        TimingPath path = trace_path(sink_tag, sink_node); 

        paths.push_back(path);

        if(paths.size() >= npaths) break;
    }

    return paths;
}

TimingPath TimingReporter::trace_path(const tatum::TimingTag& sink_tag, const NodeId sink_node) const {
    VTR_ASSERT(timing_graph_->node_type(sink_node) == tatum::NodeType::SINK);
    VTR_ASSERT(sink_tag.type() == tatum::TagType::SLACK);

    TimingPath path;
    path.launch_domain = sink_tag.launch_clock_domain();
    path.capture_domain = sink_tag.capture_clock_domain();

    //Trace the data launch path
    NodeId curr_node = sink_node;
    while(curr_node && timing_graph_->node_type(curr_node) != tatum::NodeType::CPIN) {
        //Trace until we hit the origin, or a clock pin

        auto data_tags = setup_analyzer_->setup_tags(curr_node, tatum::TagType::DATA_ARRIVAL);
        auto iter = find_tag(data_tags, path.launch_domain, DomainId::INVALID());
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
        VTR_ASSERT(timing_graph_->node_type(curr_node) == tatum::NodeType::CPIN);

        while(curr_node) {
            auto launch_tags = setup_analyzer_->setup_tags(curr_node, tatum::TagType::CLOCK_LAUNCH);
            auto iter = find_tag(launch_tags, path.launch_domain, DomainId::INVALID());
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
        //Reverse back-trace
        std::reverse(path.clock_launch.begin(), path.clock_launch.end());
    }

    //Trace the clock capture path
    curr_node = sink_node;
    while(curr_node) {
        auto capture_tags = setup_analyzer_->setup_tags(curr_node, tatum::TagType::CLOCK_CAPTURE);
        auto iter = find_tag(capture_tags, DomainId::INVALID(), path.capture_domain);
        VTR_ASSERT(iter != capture_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph_->find_edge(iter->origin_node(), curr_node);
            VTR_ASSERT(edge);
        }

        //Record
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
    if(val.value() >= 0.) ss << " ";
    ss << std::fixed << std::setprecision(precision_) << convert_to_printable_units(val.value());

    return ss.str();
}
