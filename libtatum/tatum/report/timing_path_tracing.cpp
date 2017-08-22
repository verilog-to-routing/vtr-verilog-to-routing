#include "tatum/report/timing_path_tracing.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/tags/TimingTags.hpp"

namespace tatum { namespace detail {

Time calc_path_delay(const std::vector<TimingPathElem>& path);
NodeId find_startpoint(const std::vector<TimingPathElem>& path);
NodeId find_endpoint(const std::vector<TimingPathElem>& path);

TimingPath trace_path(const TimingGraph& timing_graph,
                      const detail::TagRetriever& tag_retriever, 
                      const DomainId launch_domain,
                      const DomainId capture_domain,
                      const NodeId sink_node) {

    TATUM_ASSERT(timing_graph.node_type(sink_node) == NodeType::SINK);

    /*
     * Back-trace the data launch path
     */
    TimingTag slack_tag;
    std::vector<TimingPathElem> data_arrival_elements;
    NodeId curr_node = sink_node;
    while(curr_node && timing_graph.node_type(curr_node) != NodeType::CPIN) {
        //Trace until we hit the origin, or a clock pin

        if(curr_node == sink_node) {
            //Record the slack at the sink node
            auto slack_tags = tag_retriever.slacks(curr_node);
            auto iter = find_tag(slack_tags, launch_domain, capture_domain);
            TATUM_ASSERT(iter != slack_tags.end());

            slack_tag = *iter;
        }

        auto data_tags = tag_retriever.tags(curr_node, TagType::DATA_ARRIVAL);

        //First try to find the exact tag match
        auto iter = find_tag(data_tags, launch_domain, capture_domain);
        if(iter == data_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(data_tags, launch_domain, DomainId::INVALID());
        }
        TATUM_ASSERT(iter != data_tags.end());

        EdgeId edge;
        if(iter->origin_node()) {
            edge = timing_graph.find_edge(iter->origin_node(), curr_node);
            TATUM_ASSERT(edge);
        }

        //Record
        data_arrival_elements.emplace_back(*iter, curr_node, edge);

        //Advance to the previous node
        curr_node = iter->origin_node();
    }
    //Since we back-traced from sink we reverse to get the forward order
    std::reverse(data_arrival_elements.begin(), data_arrival_elements.end());

    /*
     * Back-trace the launch clock path
     */
    std::vector<TimingPathElem> clock_launch_elements;
    EdgeId clock_launch_edge = timing_graph.node_clock_launch_edge(data_arrival_elements[0].node());
    if(clock_launch_edge) {
        //Mark the edge between clock and data paths
        data_arrival_elements[0].set_incomming_edge(clock_launch_edge);

        //Through the clock network
        curr_node = timing_graph.edge_src_node(clock_launch_edge);
        TATUM_ASSERT(timing_graph.node_type(curr_node) == NodeType::CPIN);

        while(curr_node) {
            auto launch_tags = tag_retriever.tags(curr_node, TagType::CLOCK_LAUNCH);
            auto iter = find_tag(launch_tags, launch_domain, capture_domain);
            if(iter == launch_tags.end()) {
                //Then look for incompletely specified (i.e. wildcard) capture clocks
                iter = find_tag(launch_tags, launch_domain, DomainId::INVALID());
            }
            TATUM_ASSERT(iter != launch_tags.end());

            EdgeId edge;
            if(iter->origin_node()) {
                edge = timing_graph.find_edge(iter->origin_node(), curr_node);
                TATUM_ASSERT(edge);
            }

            //Record
            clock_launch_elements.emplace_back(*iter, curr_node, edge);

            //Advance to the previous node
            curr_node = iter->origin_node();
        }
    }
    //Reverse back-trace
    std::reverse(clock_launch_elements.begin(), clock_launch_elements.end());

    /*
     * Back-trace the clock capture path
     */

    //Record the required time
    auto required_tags = tag_retriever.tags(sink_node, TagType::DATA_REQUIRED);
    auto req_iter = find_tag(required_tags, launch_domain, capture_domain);
    TATUM_ASSERT(req_iter != required_tags.end());
    TimingPathElem data_required_element = TimingPathElem(*req_iter, sink_node, EdgeId::INVALID());

    std::vector<TimingPathElem> clock_capture_elements;

    EdgeId clock_capture_edge = timing_graph.node_clock_capture_edge(sink_node);
    if(clock_capture_edge) {
        //Mark the edge between clock and data paths (i.e. setup/hold edge)
        data_required_element.set_incomming_edge(clock_capture_edge);

        curr_node = timing_graph.edge_src_node(clock_capture_edge);
        TATUM_ASSERT(timing_graph.node_type(curr_node) == NodeType::CPIN);

        while(curr_node) {

            //Record the clock capture tag
            auto capture_tags = tag_retriever.tags(curr_node, TagType::CLOCK_CAPTURE);
            auto iter = find_tag(capture_tags, launch_domain, capture_domain);
            TATUM_ASSERT(iter != capture_tags.end());

            EdgeId edge;
            if(iter->origin_node()) {
                edge = timing_graph.find_edge(iter->origin_node(), curr_node);
                TATUM_ASSERT(edge);
            }

            clock_capture_elements.emplace_back(*iter, curr_node, edge);

            //Advance to the previous node
            curr_node = iter->origin_node();
        }
    }
    //Reverse back-trace
    std::reverse(clock_capture_elements.begin(), clock_capture_elements.end());

    TimingPathInfo path_info(tag_retriever.type(),
                             calc_path_delay(data_arrival_elements),
                             slack_tag.time(),
                             find_startpoint(data_arrival_elements),
                             find_endpoint(data_arrival_elements),
                             launch_domain,
                             capture_domain);

    TimingPath path(path_info,
                    clock_launch_elements,
                    data_arrival_elements,
                    clock_capture_elements,
                    data_required_element,
                    slack_tag);

    return path;
}

Time calc_path_delay(const std::vector<TimingPathElem>& path) {
    TATUM_ASSERT(path.size() > 0);

    TimingTag first_arrival = path[0].tag();
    TimingTag last_arrival = path[path.size()-1].tag();

    return last_arrival.time() - first_arrival.time();
}

NodeId find_startpoint(const std::vector<TimingPathElem>& path) {
    TATUM_ASSERT(path.size() > 0);

    return path[0].node();
}

NodeId find_endpoint(const std::vector<TimingPathElem>& path) {
    TATUM_ASSERT(path.size() > 0);
    
    return path[path.size() - 1].node();
}

}} //namespace
