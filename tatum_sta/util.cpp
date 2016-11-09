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

struct NodeRemap {
    NodeId old_node;
    NodeId new_node;
    float edge_delay;
}

void rebuild_timing_graph(TimingGraph& tg, TimingConstraints& tc, const VprFfInfo& ff_info, std::vector<float>& edge_delays, VprArrReqTimes& arr_req_times) {
    TimingGraph new_tg;
    TimingConstraints new_tc;
    std::vector<float> new_edge_delays;
    VprArrReqTimes new_arr_req_times;

    //Rebuild the graph to elminate redundant nodes (e.g. IPIN/OPINs in front of SOURCES/SINKS)

    for(NodeId node : tg.nodes()) {
        if(tg.node_type(node) == NodeType::SOURCE) {
            for(EdgeId out_edge : tg.node_out_edges(node)) {
                NodeId sink_node = tg.edge_sink_node(out_edge);

                if(tg.node_type(sink_node) == NodeType::OPIN) {

                    if(tg.node_in_edges(node).size() == 0) {
                        //Primary Input source node
                        std::cout << "Remove OPIN " << sink_node << " Input Constraint on " << node << " " << edge_delays[size_t(out_edge)] << "\n";
                    } else {
                        //FF source
                        TATUM_ASSERT_MSG(tg.node_in_edges(node).size() == 1, "Single clock input");

                        float tcq = edge_delays[size_t(out_edge)];

                        EdgeId in_edge = *tg.node_in_edges(node).begin();

                        NodeId clock_sink = tg.edge_src_node(in_edge);

                        std::cout << "Remove OPIN " << sink_node << " Tcq " << tcq << " " << clock_sink << " -> " << node << "\n";

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

                    NodeId src_node = tg.edge_src_node(in_edge);

                    TATUM_ASSERT(tg.node_type(src_node) == NodeType::IPIN);

                    float output_constraint = edge_delays[size_t(in_edge)];

                    std::cout << "Remove IPIN " << src_node << " Output Constraint on " << node << " " << output_constraint << "\n";

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
                    
                    std::cout << "Remove IPIN " << ff_ipin << " Tsu " << tsu << " " << ff_clock << " -> " << node << "\n";
                }
            }
            
        }
    }
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

