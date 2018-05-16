#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/report/TimingPathCollector.hpp"
#include "tatum/report/TimingReportTagRetriever.hpp"
#include "tatum/report/timing_path_tracing.hpp"
#include <map>

namespace tatum {

namespace detail {
std::vector<TimingPath> collect_worst_timing_paths(const TimingGraph& timing_graph, const detail::TagRetriever& tag_retriever, size_t npaths);
std::vector<SkewPath> collect_worst_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraings,
                                              const detail::TagRetriever& tag_retriever, TimingType timing_type, size_t npaths);

std::vector<TimingPath> collect_worst_timing_paths(const TimingGraph& timing_graph, const detail::TagRetriever& tag_retriever, size_t npaths) {
    std::vector<TimingPath> paths;

    //Sort the sinks by slack
    std::map<TimingTag,NodeId,TimingTagValueComp> tags_and_sinks;

    //Add the slacks of all sink
    for(NodeId node : timing_graph.logical_outputs()) {
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

        TimingPath path = detail::trace_path(timing_graph, tag_retriever, sink_tag.launch_clock_domain(), sink_tag.capture_clock_domain(), sink_node); 

        paths.push_back(path);

        if(paths.size() >= npaths) break;
    }

    return paths;
}

std::vector<SkewPath> collect_worst_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, 
                                               const detail::TagRetriever& tag_retriever, TimingType timing_type, size_t npaths) {
    std::vector<SkewPath> paths;

    for(NodeId node : timing_graph.nodes()) {
        NodeType node_type = timing_graph.node_type(node);
        if (node_type != NodeType::SINK) continue;

        const auto& required_tags = tag_retriever.tags(node, TagType::DATA_REQUIRED);

        for (auto& required_tag : required_tags) {
            SkewPath path;

            path.launch_domain = required_tag.launch_clock_domain();
            path.capture_domain = required_tag.capture_clock_domain();

            TimingSubPath data_arrival_path = detail::trace_data_arrival_path(timing_graph, tag_retriever, path.launch_domain, path.capture_domain, node);

            TATUM_ASSERT(!data_arrival_path.elements().empty());
            auto& data_launch_elem = *data_arrival_path.elements().begin(); 

            //Constant generators do not have skew
            if (is_const_gen_tag(data_launch_elem.tag())) continue;

            NodeId launch_node = data_launch_elem.node();

            path.clock_launch_path = detail::trace_skew_clock_launch_path(timing_graph, tag_retriever, path.launch_domain, path.capture_domain, launch_node);
            path.clock_capture_path = detail::trace_skew_clock_capture_path(timing_graph, tag_retriever, path.launch_domain, path.capture_domain, node);

            path.clock_launch_arrival = calc_path_delay(path.clock_launch_path);
            path.clock_capture_arrival = calc_path_delay(path.clock_capture_path);
            if (timing_type == TimingType::SETUP) {
                path.clock_constraint = timing_constraints.setup_constraint(path.launch_domain, path.capture_domain);
            } else {
                TATUM_ASSERT(timing_type == TimingType::HOLD);
                path.clock_constraint = timing_constraints.hold_constraint(path.launch_domain, path.capture_domain);
            }

            path.clock_skew = path.clock_capture_arrival - path.clock_launch_arrival - path.clock_constraint;

            paths.push_back(path);
        }
    }

    auto skew_order = [&](const SkewPath& lhs, const SkewPath& rhs) {
        if (timing_type == TimingType::SETUP) {
            //Positive skew helps setup paths (since the capture clock edge is delayed, 
            //lengthening the clock period), so show the most negative skews first.
            return lhs.clock_skew < rhs.clock_skew;
        } else {
            //Positive skew hurts hold paths (since the capture clock edge is delay,
            //this gives the data more time to catch-up to the capture clock),
            //so show the most positive skews first.
            TATUM_ASSERT(timing_type == TimingType::HOLD);
            return lhs.clock_skew > rhs.clock_skew;
        }
    };
    std::sort(paths.begin(), paths.end(), skew_order);

    //TODO: not very efficient, since we generate all paths first and then trim to npaths...
    paths.resize(std::min(paths.size(), npaths));

    return paths;
}

} //namespace detail

std::vector<TimingPath> TimingPathCollector::collect_worst_setup_timing_paths(const TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const {
    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    return collect_worst_timing_paths(timing_graph, tag_retriever, npaths);
}

std::vector<TimingPath> TimingPathCollector::collect_worst_hold_timing_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const {
    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    return collect_worst_timing_paths(timing_graph, tag_retriever, npaths);
}

std::vector<SkewPath> TimingPathCollector::collect_worst_setup_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const {
    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    return collect_worst_skew_paths(timing_graph, timing_constraints, tag_retriever, TimingType::SETUP, npaths);
}

std::vector<SkewPath> TimingPathCollector::collect_worst_hold_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const {
    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    return collect_worst_skew_paths(timing_graph, timing_constraints, tag_retriever, TimingType::HOLD, npaths);
}

} //namespace tatum
