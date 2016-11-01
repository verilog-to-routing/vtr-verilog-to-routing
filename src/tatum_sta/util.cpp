#include <set>
#include <vector>
#include <map>
#include <cmath>

#include "util.hpp"

#include "TimingGraph.hpp"

using tatum::TimingGraph;
using tatum::NodeId;
using tatum::EdgeId;
using tatum::NodeType;

void identify_constant_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& const_gen_fanout_nodes);
void identify_clock_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& clock_gen_fanout_nodes);

std::set<NodeId> identify_constant_gen_fanout(const TimingGraph& tg) {
    //Walk the timing graph and identify nodes that are in the fanout of a constant generator
    std::set<NodeId> const_gen_fanout_nodes;
    for(NodeId node_id : tg.primary_inputs()) {
        if(tg.node_type(node_id) == NodeType::CONSTANT_GEN_SOURCE) {
            identify_constant_gen_fanout_helper(tg, node_id, const_gen_fanout_nodes);
        }
    }
    return const_gen_fanout_nodes;
}

void identify_constant_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& const_gen_fanout_nodes) {
    if(const_gen_fanout_nodes.count(node_id) == 0) {
        //Haven't seen this node before
        const_gen_fanout_nodes.insert(node_id);
        for(EdgeId edge_id : tg.node_out_edges(node_id)) {
            identify_constant_gen_fanout_helper(tg, tg.edge_sink_node(edge_id), const_gen_fanout_nodes);
        }
    }
}

std::set<NodeId> identify_clock_gen_fanout(const TimingGraph& tg) {
    std::set<NodeId> clock_gen_fanout_nodes;
    for(NodeId node_id : tg.primary_inputs()) {
        if(tg.node_type(node_id) == NodeType::CLOCK_SOURCE) {
            identify_clock_gen_fanout_helper(tg, node_id, clock_gen_fanout_nodes);
        }
    }
    return clock_gen_fanout_nodes;
}

void identify_clock_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& clock_gen_fanout_nodes) {
    if(clock_gen_fanout_nodes.count(node_id) == 0) {
        //Haven't seen this node before
        clock_gen_fanout_nodes.insert(node_id);
        for(EdgeId edge_id : tg.node_out_edges(node_id)) {
            identify_clock_gen_fanout_helper(tg, tg.edge_sink_node(edge_id), clock_gen_fanout_nodes);
        }
    }

}

void add_ff_clock_to_source_sink_edges(TimingGraph& tg, const std::vector<BlockId> node_logical_blocks, std::vector<float>& edge_delays) {
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
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_clocks;
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_sources;
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_sinks;

    for(NodeId node_id : tg.nodes()) {
        BlockId blk_id = node_logical_blocks[size_t(node_id)];
        if(tg.node_type(node_id) == NodeType::FF_CLOCK) {
            logical_block_FF_clocks[blk_id].push_back(node_id);
        } else if (tg.node_type(node_id) == NodeType::FF_SOURCE) {
            logical_block_FF_sources[blk_id].push_back(node_id);
        } else if (tg.node_type(node_id) == NodeType::FF_SINK) {
            logical_block_FF_sinks[blk_id].push_back(node_id);
        }
    }

    //Loop through each FF_CLOCK and add edges to FF_SINKs and FF_SOURCEs
    for(const auto clock_kv : logical_block_FF_clocks) {
        BlockId logical_block_id = clock_kv.first;
        TATUM_ASSERT(clock_kv.second.size() == 1);
        NodeId ff_clock_node_id = clock_kv.second[0];

        //Check for FF_SOURCEs associated with this FF_CLOCK pin
        auto src_iter = logical_block_FF_sources.find(logical_block_id);
        if(src_iter != logical_block_FF_sources.end()) {
            //Go through each assoicated source and add an edge to it
            for(NodeId ff_src_node_id : src_iter->second) {
                tg.add_edge(ff_clock_node_id, ff_src_node_id);

                //Mark edge as having zero delay
                edge_delays.push_back(0.);
            }
        }

        //Check for FF_SINKs associated with this FF_CLOCK pin
        auto sink_iter = logical_block_FF_sinks.find(logical_block_id);
        if(sink_iter != logical_block_FF_sinks.end()) {
            //Go through each assoicated sink and add an edge to it
            for(NodeId ff_sink_node_id : sink_iter->second) {
                tg.add_edge(ff_clock_node_id, ff_sink_node_id);

                //Mark edge as having zero delay
                edge_delays.push_back(0.);
            }
        }
    }
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

