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

