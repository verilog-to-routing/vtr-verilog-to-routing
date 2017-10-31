#include "tatum/TimingGraph.hpp"
#include "tatum/report/TimingPathCollector.hpp"
#include "tatum/report/TimingReportTagRetriever.hpp"
#include "tatum/report/timing_path_tracing.hpp"
#include <map>

namespace tatum {

namespace detail {
std::vector<TimingPath> collect_worst_paths(const TimingGraph& timing_graph, const detail::TagRetriever& tag_retriever, size_t npaths);

std::vector<TimingPath> collect_worst_paths(const TimingGraph& timing_graph, const detail::TagRetriever& tag_retriever, size_t npaths) {
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

} //namespace detail

std::vector<TimingPath> TimingPathCollector::collect_worst_setup_paths(const TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const {
    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    return collect_worst_paths(timing_graph, tag_retriever, npaths);
}

std::vector<TimingPath> TimingPathCollector::collect_worst_hold_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const {
    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    return collect_worst_paths(timing_graph, tag_retriever, npaths);
}


} //namespace tatum
