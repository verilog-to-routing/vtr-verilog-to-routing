#include <map>

#include "assert.hpp"

#include "TimingGraph.hpp"


NodeId TimingGraph::add_node(const TimingNode& new_node) {
    //Type
    TN_Type new_node_type = new_node.type();
    node_types_.push_back(new_node.type());

    //Domain
    node_clock_domains_.push_back(new_node.clock_domain());  
    node_logical_blocks_.push_back(new_node.logical_block());

    //Save primary outputs as we build the graph
    if(new_node_type == TN_Type::OUTPAD_SINK ||
       new_node_type == TN_Type::FF_SINK) {
        primary_outputs_.push_back(node_types_.size() - 1);
    }

    //Out edges
    std::vector<EdgeId> out_edges = std::vector<EdgeId>();
    out_edges.reserve(new_node.num_out_edges());
    for(int i = 0; i < new_node.num_out_edges(); i++) {
        EdgeId edge_id = new_node.out_edge_id(i);
        out_edges.emplace_back(edge_id);
    }
    node_out_edges_.push_back(std::move(out_edges));
    
    //In edges
    //  these get filled in later, after all nodes have been added
    std::vector<EdgeId> in_edges = std::vector<EdgeId>();
    node_in_edges_.push_back(std::move(in_edges));

    //Verify sizes
    ASSERT(node_types_.size() == node_clock_domains_.size());
    ASSERT(node_types_.size() == node_out_edges_.size());
    ASSERT(node_types_.size() == node_in_edges_.size());

    //Return the ID of the added node
    return node_types_.size() - 1;
}

EdgeId TimingGraph::add_edge(const TimingEdge& new_edge) {
    edge_sink_nodes_.push_back(new_edge.to_node_id());
    edge_src_nodes_.push_back(new_edge.from_node_id());
    edge_delays_.push_back(new_edge.delay());

    ASSERT(edge_sink_nodes_.size() == edge_src_nodes_.size());
    ASSERT(edge_sink_nodes_.size() == edge_delays_.size());

    //Return the edge id of the added edge
    return edge_delays_.size() - 1;
}

void TimingGraph::fill_back_edges() {
    //Add reverse/back links for all edges in the timing graph
    for(int i = 0; i < num_nodes(); i++) {
        for(EdgeId edge_id : node_out_edges_[i]) {
            NodeId sink_node = edge_sink_node(edge_id);

            //edge_id drives the sink so it is an in edge
            node_in_edges_[sink_node].push_back(edge_id);
        }
    }
}

void TimingGraph::add_launch_capture_edges() {
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

    for(NodeId node_id = 0; node_id < num_nodes(); node_id++) {
        if(node_type(node_id) == TN_Type::FF_CLOCK) {
            logical_block_FF_clocks[node_logical_block(node_id)].push_back(node_id);
        } else if (node_type(node_id) == TN_Type::FF_SOURCE) {
            logical_block_FF_sources[node_logical_block(node_id)].push_back(node_id);
        } else if (node_type(node_id) == TN_Type::FF_SINK) {
            logical_block_FF_sinks[node_logical_block(node_id)].push_back(node_id);
        } 
    }

    //Loop through each FF_CLOCK and add edges to FF_SINKs and FF_SOURCEs
    for(const auto clock_kv : logical_block_FF_clocks) {
        BlockId logical_block_id = clock_kv.first;
        VERIFY(clock_kv.second.size() == 1);
        NodeId ff_clock_node_id = clock_kv.second[0];

        //Check for FF_SOURCEs associated with this FF_CLOCK pin
        auto src_iter = logical_block_FF_sources.find(logical_block_id);
        if(src_iter != logical_block_FF_sources.end()) {
            //Go through each assoicated source and add an edge to it
            for(NodeId ff_src_node_id : src_iter->second) {
                //TODO: remove TimingEdge class
                EdgeId edge_id = add_edge(TimingEdge(Time(0.), ff_clock_node_id, ff_src_node_id));
                node_out_edges_[ff_clock_node_id].push_back(edge_id);
            }
        }

        //Check for FF_SINKs associated with this FF_CLOCK pin
        auto sink_iter = logical_block_FF_sinks.find(logical_block_id);
        if(sink_iter != logical_block_FF_sinks.end()) {
            //Go through each assoicated sink and add an edge to it
            for(NodeId ff_sink_node_id : sink_iter->second) {
                //TODO: remove TimingEdge class
                EdgeId edge_id = add_edge(TimingEdge(Time(0.), ff_clock_node_id, ff_sink_node_id));
                node_out_edges_[ff_clock_node_id].push_back(edge_id);
            }
        }
    }
}

void TimingGraph::levelize() {
    VERIFY(0);
}

void TimingGraph::contiguize_level_edges() {
    //Make all edges in a level be contiguous in memory
    std::cout << "Re-allocating edges so levels are in contiguous memory" << std::endl;

    //Collect levels for edges
    std::vector<std::vector<EdgeId>> edge_levels;
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        edge_levels.push_back(std::vector<EdgeId>());
        for(auto node_id : node_levels_[level_idx]) {
            for(int i = 0; i < num_node_out_edges(node_id); i++) {
                EdgeId edge_id = node_out_edge(node_id, i);

                edge_levels[level_idx].push_back(edge_id);
            }
        }
    }

    //Save a map from old edge_id to new edge_id, will be used to update node refs
    std::map<EdgeId,EdgeId> old_edge_to_new_edge;
    int cnt = 0;
    for(std::vector<EdgeId>& edge_level : edge_levels) {
        for(EdgeId orig_edge_id : edge_level) {
            old_edge_to_new_edge[orig_edge_id] = cnt; 
            cnt++;
        }
    }

    /*
     * Re-allocate edges to be contiguous in memory
     */

    //Save the old values while we write the new ones
    std::vector<NodeId> old_edge_sink_nodes_;
    std::vector<NodeId> old_edge_src_nodes_;
#ifdef TIME_MEM_ALIGN
    std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> old_edge_delays_;
#else
    std::vector<Time> old_edge_delays_;
#endif

    //Swap them
    std::swap(old_edge_sink_nodes_, edge_sink_nodes_);
    std::swap(old_edge_src_nodes_, edge_src_nodes_);
    std::swap(old_edge_delays_, edge_delays_);

    //Update values
    for(auto& edge_level : edge_levels) {
        for(auto& orig_edge_id : edge_level) {
            //Write edges in the new contiguous order
            edge_sink_nodes_.push_back(old_edge_sink_nodes_[orig_edge_id]);
            edge_src_nodes_.push_back(old_edge_src_nodes_[orig_edge_id]);
            edge_delays_.push_back(old_edge_delays_[orig_edge_id]);
        }
    }

    /*
     * Update edge refs in nodes
     */
    for(int i = 0; i < num_nodes(); i++) {
        for(size_t j = 0; j < node_out_edges_[i].size(); j++) {
            EdgeId old_edge_id = node_out_edges_[i][j];
            node_out_edges_[i][j] = old_edge_to_new_edge[old_edge_id];
        }
        for(size_t j = 0; j < node_in_edges_[i].size(); j++) {
            EdgeId old_edge_id = node_in_edges_[i][j];
            node_in_edges_[i][j] = old_edge_to_new_edge[old_edge_id];
        }
    }
}

std::map<NodeId,NodeId> TimingGraph::contiguize_level_nodes() {
    //Make all nodes in a level be contiguous in memory
    std::cout << "Re-allocating nodes so levels are in contiguous memory" << std::endl;

    /*
     * Build a map of the old and new node ids to update edges
     * and node levels later
     */
    std::map<NodeId,NodeId> old_node_to_new_node;
    int cnt = 0;
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId node_id : node_levels_[level_idx]) {
            old_node_to_new_node[node_id] = cnt;
            cnt++;
        }
    }


    /*
     * Re-allocate nodes so levels are in contiguous memory
     */
    std::vector<TN_Type> old_node_types;
    std::vector<DomainId> old_node_clock_domains;
    std::vector<std::vector<EdgeId>> old_node_out_edges;
    std::vector<std::vector<EdgeId>> old_node_in_edges;

    //Swap the values
    std::swap(old_node_types, node_types_);
    std::swap(old_node_clock_domains, node_clock_domains_);
    std::swap(old_node_out_edges, node_out_edges_);
    std::swap(old_node_in_edges, node_in_edges_);

    //Update the values
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId old_node_id : node_levels_[level_idx]) {
            node_types_.push_back(old_node_types[old_node_id]);
            node_clock_domains_.push_back(old_node_clock_domains[old_node_id]);
            node_out_edges_.push_back(old_node_out_edges[old_node_id]);
            node_in_edges_.push_back(old_node_in_edges[old_node_id]);
        }
    }

    /*
     * Update old references to node_ids with thier new values
     */
    //The node levels
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(size_t i = 0; i < node_levels_[level_idx].size(); i++) {
            NodeId old_node_id = node_levels_[level_idx][i];
            node_levels_[level_idx][i] = old_node_to_new_node[old_node_id];
        }
    }

    //The primary outputs
    for(size_t i = 0; i < primary_outputs_.size(); i++) {
        NodeId old_node_id = primary_outputs_[i];
        primary_outputs_[i] = old_node_to_new_node[old_node_id];
    }

    //The Edges
    for(int i = 0; i < num_edges(); i++) {
        NodeId old_sink_node = edge_sink_nodes_[i];
        NodeId old_src_node = edge_src_nodes_[i];

        edge_sink_nodes_[i] = old_node_to_new_node[old_sink_node];
        edge_src_nodes_[i] = old_node_to_new_node[old_src_node];
    }

    return old_node_to_new_node;
}
