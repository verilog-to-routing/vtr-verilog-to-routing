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

    struct TagNode {
        TagNode(TimingTag t, NodeId n) noexcept
            : tag(t), node(n) {}

        TimingTag tag;
        NodeId node;

    };

    std::vector<TagNode> tags_and_sinks;

    //Add the slacks of all sink
    for(NodeId node : timing_graph.logical_outputs()) {
        for(TimingTag tag : tag_retriever.slacks(node)) {
            tags_and_sinks.emplace_back(tag,node);
        }
    }

    //Sort in ascending slack order so most negative slacks are first
    auto ascending_slack_order = [](const TagNode& lhs, const TagNode& rhs) {
        return lhs.tag.time() < rhs.tag.time();
    };
    std::sort(tags_and_sinks.begin(), tags_and_sinks.end(), ascending_slack_order);


    //Trace the paths for each tag/node pair
    // The the map will sort the nodes and tags from worst to best slack (i.e. ascending slack order),
    // so the first pair is the most critical end-point
    for(const auto& tag_node : tags_and_sinks) {
        NodeId sink_node = tag_node.node;
        TimingTag sink_tag = tag_node.tag;

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

            path.data_launch_node = data_launch_elem.node();
            path.data_capture_node = node;

            path.clock_launch_path = detail::trace_clock_launch_path(timing_graph, tag_retriever, path.launch_domain, path.capture_domain, path.data_launch_node);
            path.clock_capture_path = detail::trace_clock_capture_path(timing_graph, tag_retriever, path.launch_domain, path.capture_domain, path.data_capture_node);

            if (path.clock_launch_path.elements().empty()) {
                //Primary input
                path.clock_launch_arrival = data_launch_elem.tag().time();

                //Adjust for input delay
                if (timing_type == TimingType::SETUP) {
                    path.clock_launch_arrival -= timing_constraints.input_constraint(path.data_launch_node, path.launch_domain, DelayType::MAX);
                } else {
                    TATUM_ASSERT(timing_type == TimingType::HOLD);
                    path.clock_launch_arrival -= timing_constraints.input_constraint(path.data_launch_node, path.launch_domain, DelayType::MIN);
                }
            } else {
                //FF source
                path.clock_launch_arrival = path_end(path.clock_launch_path);
            }

            if (path.clock_capture_path.elements().empty()) {
                //Primary output
                path.clock_capture_arrival = required_tag.time();

                //Adjust for output delay and clock uncertainty
                if (timing_type == TimingType::SETUP) {
                    path.clock_capture_arrival += timing_constraints.output_constraint(path.data_capture_node, path.capture_domain, DelayType::MAX);
                } else {
                    TATUM_ASSERT(timing_type == TimingType::HOLD);
                    path.clock_capture_arrival += timing_constraints.output_constraint(path.data_capture_node, path.capture_domain, DelayType::MIN);
                }
                //TODO: need to think about why we don't need to adjust for uncertainty on these paths...
            } else {
                //FF capture
                path.clock_capture_arrival = path_end(path.clock_capture_path);

                //Adjust for clock uncertainty
                if (timing_type == TimingType::SETUP) {
                    path.clock_capture_arrival -= timing_constraints.setup_clock_uncertainty(path.launch_domain, path.capture_domain);
                } else {
                    TATUM_ASSERT(timing_type == TimingType::HOLD);
                    path.clock_capture_arrival += timing_constraints.hold_clock_uncertainty(path.launch_domain, path.capture_domain);
                }
            }


            //Record period constraint
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
