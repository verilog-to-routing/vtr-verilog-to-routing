#include "tatum/report/timing_path_tracing.hpp"
#include "tatum/report/TimingPath.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/tags/TimingTags.hpp"

namespace tatum { namespace detail {

NodeId find_startpoint(const TimingSubPath& path);
NodeId find_endpoint(const TimingSubPath& path);

TimingPath trace_path(const TimingGraph& timing_graph,
                      const detail::TagRetriever& tag_retriever, 
                      const DomainId launch_domain,
                      const DomainId capture_domain,
                      const NodeId data_capture_node) {

    TATUM_ASSERT(timing_graph.node_type(data_capture_node) == NodeType::SINK);


    //Record the slack at the sink node
    TimingTag slack_tag;
    auto slack_tags = tag_retriever.slacks(data_capture_node);
    auto iter = find_tag(slack_tags, launch_domain, capture_domain);
    TATUM_ASSERT(iter != slack_tags.end());
    slack_tag = *iter;

    TimingSubPath data_arrival_path = trace_data_arrival_path(timing_graph,
                                                              tag_retriever,
                                                              launch_domain,
                                                              capture_domain,
                                                              data_capture_node);

    TATUM_ASSERT(!data_arrival_path.elements().empty());
    NodeId data_launch_node = data_arrival_path.elements().begin()->node();
    TimingSubPath clock_launch_path = trace_clock_launch_path(timing_graph,
                                                              tag_retriever,
                                                              launch_domain,
                                                              capture_domain,
                                                              data_launch_node);

    TimingSubPath clock_capture_path = trace_clock_capture_path(timing_graph,
                                                                tag_retriever,
                                                                launch_domain,
                                                                capture_domain,
                                                                data_capture_node);
    //Record the required time
    auto required_tags = tag_retriever.tags(data_capture_node, TagType::DATA_REQUIRED);
    auto req_iter = find_tag(required_tags, launch_domain, capture_domain);
    TATUM_ASSERT(req_iter != required_tags.end());
    TimingPathElem data_required_element = TimingPathElem(*req_iter, data_capture_node, EdgeId::INVALID());

    EdgeId clock_capture_edge = timing_graph.node_clock_capture_edge(data_capture_node);
    if(clock_capture_edge) {
        //Mark the edge between clock and data paths (i.e. setup/hold edge)
        data_required_element.set_incomming_edge(clock_capture_edge);
    }

    TimingPathInfo path_info(tag_retriever.type(),
                             calc_path_delay(data_arrival_path),
                             slack_tag.time(),
                             find_startpoint(data_arrival_path),
                             find_endpoint(data_arrival_path),
                             launch_domain,
                             capture_domain);

    TimingPath path(path_info,
                    clock_launch_path,
                    data_arrival_path,
                    clock_capture_path,
                    data_required_element,
                    slack_tag);

    return path;
}

TimingSubPath trace_clock_launch_path(const TimingGraph& timing_graph,
                                      const detail::TagRetriever& tag_retriever, 
                                      const DomainId launch_domain,
                                      const DomainId capture_domain,
                                      const NodeId data_launch_node) {
    TATUM_ASSERT(timing_graph.node_type(data_launch_node) == NodeType::SOURCE);

    /*
     * Backtrace the launch clock path
     */
    std::vector<TimingPathElem> clock_launch_elements;
    EdgeId clock_launch_edge = timing_graph.node_clock_launch_edge(data_launch_node);
    if(clock_launch_edge) {

        //Through the clock network
        NodeId curr_node = timing_graph.edge_src_node(clock_launch_edge);
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
    //Reverse backtrace
    std::reverse(clock_launch_elements.begin(), clock_launch_elements.end());

    return TimingSubPath(clock_launch_elements);
}

TimingSubPath trace_data_arrival_path(const TimingGraph& timing_graph,
                                      const detail::TagRetriever& tag_retriever, 
                                      const DomainId launch_domain,
                                      const DomainId capture_domain,
                                      const NodeId data_capture_node) {
    TATUM_ASSERT(timing_graph.node_type(data_capture_node) == NodeType::SINK);

    /*
     * Backtrace the data launch path
     */
    std::vector<TimingPathElem> data_arrival_elements;
    NodeId curr_node = data_capture_node;
    while(curr_node && timing_graph.node_type(curr_node) != NodeType::CPIN) {
        //Trace until we hit the origin, or a clock pin
        auto data_tags = tag_retriever.tags(curr_node, TagType::DATA_ARRIVAL);

        //First try to find the exact tag match
        auto iter = find_tag(data_tags, launch_domain, capture_domain);
        if(iter == data_tags.end()) {
            //Then look for incompletely specified (i.e. wildcard) capture clocks
            iter = find_tag(data_tags, launch_domain, DomainId::INVALID());

            //Look for a constant generator
            if (iter == data_tags.end()) {
                iter = find_tag(data_tags, DomainId::INVALID(), DomainId::INVALID()); 
                TATUM_ASSERT(iter != data_tags.end());
                TATUM_ASSERT(is_const_gen_tag(*iter));
            }
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

    EdgeId clock_launch_edge = timing_graph.node_clock_launch_edge(data_arrival_elements.back().node());
    if(clock_launch_edge) {
        //Mark the edge between clock and data paths (i.e. setup/hold edge)
        data_arrival_elements.back().set_incomming_edge(clock_launch_edge);
    }

    //Since we backtraced from sink we reverse to get the forward order
    std::reverse(data_arrival_elements.begin(), data_arrival_elements.end());

    return TimingSubPath(data_arrival_elements);
}

TimingSubPath trace_clock_capture_path(const TimingGraph& timing_graph,
                                       const detail::TagRetriever& tag_retriever, 
                                       const DomainId launch_domain,
                                       const DomainId capture_domain,
                                       const NodeId data_capture_node) {
    TATUM_ASSERT(timing_graph.node_type(data_capture_node) == NodeType::SINK);
    /*
     * Backtrace the clock capture path
     */
    std::vector<TimingPathElem> clock_capture_elements;

    EdgeId clock_capture_edge = timing_graph.node_clock_capture_edge(data_capture_node);
    if(clock_capture_edge) {
        NodeId curr_node = timing_graph.edge_src_node(clock_capture_edge);
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
    //Reverse backtrace
    std::reverse(clock_capture_elements.begin(), clock_capture_elements.end());

    return TimingSubPath(clock_capture_elements);
}

TimingSubPath trace_skew_clock_launch_path(const TimingGraph& timing_graph,
                                           const detail::TagRetriever& tag_retriever, 
                                           const DomainId launch_domain,
                                           const DomainId capture_domain,
                                           const NodeId data_launch_node) {
    TimingSubPath subpath = trace_clock_launch_path(timing_graph, tag_retriever, launch_domain, capture_domain, data_launch_node);

    //A primary input may have no actual clock path, since the data arrival time is marked directly
    if (subpath.elements().empty()) {
        std::vector<TimingPathElem> elements;

        auto data_arrival_tags = tag_retriever.tags(data_launch_node, TagType::DATA_ARRIVAL);
        auto iter = find_tag(data_arrival_tags, launch_domain, DomainId::INVALID());
        TATUM_ASSERT(iter != data_arrival_tags.end());
        elements.emplace_back(*iter, data_launch_node, EdgeId::INVALID());

        subpath = TimingSubPath(elements);
    }
    return subpath;
}

TimingSubPath trace_skew_clock_capture_path(const TimingGraph& timing_graph,
                                            const detail::TagRetriever& tag_retriever, 
                                            const DomainId launch_domain,
                                            const DomainId capture_domain,
                                            const NodeId data_capture_node) {
    TimingSubPath subpath = trace_clock_capture_path(timing_graph, tag_retriever, launch_domain, capture_domain, data_capture_node);

    //A primary output may have no actual clock path, since the data required time is marked directly
    if (subpath.elements().empty()) {
        std::vector<TimingPathElem> elements;

        auto data_arrival_tags = tag_retriever.tags(data_capture_node, TagType::DATA_REQUIRED);
        auto iter = find_tag(data_arrival_tags, launch_domain, capture_domain);
        TATUM_ASSERT(iter != data_arrival_tags.end());
        elements.emplace_back(*iter, data_capture_node, EdgeId::INVALID());

        subpath = TimingSubPath(elements);
    }
    return subpath;
}

Time calc_path_delay(const TimingSubPath& path) {
    if (path.elements().size() > 0) {
        TimingTag first_arrival = path.elements().begin()->tag();
        TimingTag last_arrival = (--path.elements().end())->tag();

        return last_arrival.time() - first_arrival.time();
    } else {
        return Time(0.);
    }
}

Time path_end(const TimingSubPath& path) {
    if (path.elements().size() > 0) {
        TimingTag last_arrival = (--path.elements().end())->tag();
        return last_arrival.time();
    } else {
        return Time(0.);
    }
}

NodeId find_startpoint(const TimingSubPath& path) {
    TATUM_ASSERT(path.elements().size() > 0);

    return path.elements().begin()->node();
}

NodeId find_endpoint(const TimingSubPath& path) {
    TATUM_ASSERT(path.elements().size() > 0);
    
    return (--path.elements().end())->node();
}

}} //namespace
