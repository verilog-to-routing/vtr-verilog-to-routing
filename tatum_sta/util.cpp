#include <set>
#include <vector>
#include <map>
#include <cmath>

#include "util.hpp"

#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

using tatum::TimingGraph;
using tatum::TimingConstraints;
using tatum::NodeId;
using tatum::EdgeId;
using tatum::NodeType;

void rebuild_timing_graph(TimingGraph& tg, TimingConstraints& tc, const VprFfInfo& ff_info, std::vector<float>& edge_delays, VprArrReqTimes& arr_req_times) {
    TimingConstraints new_tc;

    //Rebuild the graph to elminate redundant nodes (e.g. IPIN/OPINs in front of SOURCES/SINKS)

    for(NodeId node : tg.nodes()) {
        if(tg.node_type(node) == NodeType::SOURCE) {
            for(EdgeId out_edge : tg.node_out_edges(node)) {
                NodeId sink_node = tg.edge_sink_node(out_edge);

                if(tg.node_type(sink_node) == NodeType::OPIN) {

                    if(tg.node_in_edges(node).size() == 0) {
                        //Primary Input source node
                        std::cout << "Remove PI OPIN " << sink_node << " Input Constraint on " << node << " " << edge_delays[size_t(out_edge)] << "\n";
                    } else {
                        //FF source
                        TATUM_ASSERT_MSG(tg.node_in_edges(node).size() == 1, "Single clock input");

                        float tcq = edge_delays[size_t(out_edge)];

                        EdgeId in_edge = *tg.node_in_edges(node).begin();

                        NodeId clock_sink = tg.edge_src_node(in_edge);

                        std::cout << "Remove FF OPIN " << sink_node << " Tcq " << tcq << " " << clock_sink << " -> " << node << "\n";

                    }
                }
            }
        } else if(tg.node_type(node) == NodeType::SINK) {
            if (tg.node_out_edges(node).size() > 0) {
                //Pass, a clock sink
            } else {
                TATUM_ASSERT(tg.node_out_edges(node).size() == 0);

                if(tg.node_in_edges(node).size() == 1) {
                    //Primary Output sink node
                    EdgeId in_edge = *tg.node_in_edges(node).begin();

                    NodeId ipin_node = tg.edge_src_node(in_edge);
                    TATUM_ASSERT(tg.node_type(ipin_node) == NodeType::IPIN);

                    float output_constraint = edge_delays[size_t(in_edge)];

                    NodeId ipin_sink_node = node;

                    TATUM_ASSERT(tg.node_in_edges(ipin_node).size() == 1);
                    EdgeId ipin_in_edge = *tg.node_in_edges(ipin_node).begin();
                    NodeId ipin_src_node = tg.edge_src_node(ipin_in_edge);
                    float ipin_in_delay = edge_delays[size_t(ipin_in_edge)];

                    std::cout << "Remove PO IPIN " << ipin_node << " Output Constraint on " << node << " " << output_constraint << "\n";

                    //Remove the pin (also removes it's connected edges)
                    tg.remove_node(ipin_node);

                    //Add in the replacement edge
                    EdgeId new_edge_id = tg.add_edge(ipin_src_node, ipin_sink_node);

                    //Specify the delay
                    edge_delays.resize(size_t(new_edge_id) + 1); //Make space
                    edge_delays[size_t(new_edge_id)] = ipin_in_delay;

                    //Specify the constraint
                    tc.set_output_constraint(node, tc.node_clock_domain(node), output_constraint);

                } else {
                    //FF sink
                    TATUM_ASSERT_MSG(tg.node_in_edges(node).size() == 2, "FF Sinks have at most two edges (data and clock)");

                    NodeId ff_clock;
                    NodeId ff_ipin;
                    float tsu = NAN;
                    for(EdgeId in_edge : tg.node_in_edges(node)) {
                        NodeId src_node = tg.edge_src_node(in_edge);

                        if(tg.node_type(src_node) == NodeType::SINK) {
                            ff_clock = src_node;
                        } else {
                            TATUM_ASSERT(tg.node_type(src_node) == NodeType::IPIN);
                            ff_ipin = src_node;
                            tsu = edge_delays[size_t(in_edge)];
                        }
                    }
                    TATUM_ASSERT(ff_clock);
                    TATUM_ASSERT(ff_ipin);
                    TATUM_ASSERT(!isnan(tsu));
                    
                    std::cout << "Remove FF IPIN " << ff_ipin << " Tsu " << tsu << " " << ff_clock << " -> " << node << "\n";
                }
            }
            
        }
    }

    auto id_maps = tg.compress();

    tg.levelize();

    //Remap the delays
    std::vector<float> new_edge_delays(edge_delays.size(), NAN); //Ensure there is space
    for(size_t i = 0; i < edge_delays.size(); ++i) {
        EdgeId new_id = id_maps.edge_id_map[EdgeId(i)];
        if(new_id) {
            std::cout << "Edge delay " << i << " " << edge_delays[i] << " new id " << new_id << "\n";
            new_edge_delays[size_t(new_id)] = edge_delays[i];
        }
    }
    edge_delays = new_edge_delays; //Update

    //Remap the arr/req times
    VprArrReqTimes new_arr_req_times;
    new_arr_req_times.set_num_nodes(tg.nodes().size());

    for(auto src_domain : arr_req_times.domains()) {
        //For every clock domain pair
        for(int i = 0; i < arr_req_times.get_num_nodes(); i++) {
            NodeId new_id = id_maps.node_id_map[NodeId(i)];
            if(new_id) {
                new_arr_req_times.add_arr_time(src_domain, new_id, arr_req_times.get_arr_time(src_domain, NodeId(i)));
                new_arr_req_times.add_req_time(src_domain, new_id, arr_req_times.get_req_time(src_domain, NodeId(i)));
            }
        }
    }
    arr_req_times = new_arr_req_times; //Update

    //Remap the constraints
    tc.remap_nodes(id_maps.node_id_map);
}

void add_ff_clock_to_source_sink_edges(TimingGraph& tg, const VprFfInfo& ff_info, std::vector<float>& edge_delays) {
    //We represent the dependancies between the clock and data paths
    //As edges in the graph from FF_CLOCK pins to FF_SOURCES (launch path)
    //and FF_SINKS (capture path)
    //
    //We use the information about the logical blocks associated with each
    //timing node to infer these edges.  That is, we look for FF_SINKs and FF_SOURCEs
    //that share the same logical block as an FF_CLOCK.
    //
    //TODO: Currently this just works for VPR's timing graph where only one FF_CLOCK exists
    //      per basic logic block.  This will need to be generalized.

    //Build a map from logical block id to the tnodes we care about
    std::cout << "FF_CLOCK: " << ff_info.logical_block_FF_clocks.size() << "\n";
    std::cout << "FF_SOURCE: " << ff_info.logical_block_FF_clocks.size() << "\n";
    std::cout << "FF_SINK: " << ff_info.logical_block_FF_clocks.size() << "\n";

    size_t num_edges_added = 0;
    //Loop through each FF_CLOCK and add edges to FF_SINKs and FF_SOURCEs
    for(const auto clock_kv : ff_info.logical_block_FF_clocks) {
        BlockId logical_block_id = clock_kv.first;
        NodeId ff_clock_node_id = clock_kv.second;

        //Check for FF_SOURCEs associated with this FF_CLOCK pin
        auto src_range = ff_info.logical_block_FF_sources.equal_range(logical_block_id);
        //Go through each assoicated source and add an edge to it
        for(auto kv : tatum::util::make_range(src_range.first, src_range.second)) {
            NodeId ff_src_node_id = kv.second;
            tg.add_edge(ff_clock_node_id, ff_src_node_id);

            //Mark edge as having zero delay
            edge_delays.push_back(0.);

            ++num_edges_added;
        }

        //Check for FF_SINKs associated with this FF_CLOCK pin
        auto sink_range = ff_info.logical_block_FF_sinks.equal_range(logical_block_id);
        //Go through each assoicated source and add an edge to it
        for(auto kv : tatum::util::make_range(sink_range.first, sink_range.second)) {
            NodeId ff_sink_node_id = kv.second;
            tg.add_edge(ff_clock_node_id, ff_sink_node_id);

            //Mark edge as having zero delay
            edge_delays.push_back(0.);

            ++num_edges_added;
        }
    }

    std::cout << "FF related Edges added: " << num_edges_added << "\n";
}

float relative_error(float A, float B) {
    if (A == B) {
        return 0.;
    }

    if (fabs(B) > fabs(A)) {
        return fabs((A - B) / B);
    } else {
        return fabs((A - B) / A);
    }
}

