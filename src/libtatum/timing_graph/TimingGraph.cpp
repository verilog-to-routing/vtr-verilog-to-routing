#include <map>

#include "assert.hpp"

#include "TimingGraph.hpp"


NodeId TimingGraph::add_node(const TimingNode& new_node) {
    //Type
    TN_Type new_node_type = new_node.type();
    node_types_.push_back(new_node.type());

    //Domain
    node_clock_domains_.push_back(new_node.clock_domain());

    //Logical block
    node_logical_blocks_.push_back(new_node.logical_block());

    //Clock source
    node_is_clock_source_.push_back(new_node.is_clock_source());

    NodeId node_id = node_types_.size() - 1;

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

    //Orig Node ID map
    //This defaults to the current index
    orig_node_id_map_.push_back(node_id);

    //Verify sizes
    ASSERT(node_types_.size() == node_clock_domains_.size());
    ASSERT(node_types_.size() == node_out_edges_.size());
    ASSERT(node_types_.size() == node_in_edges_.size());

    //Verify that the index mapping is correct
    ASSERT(orig_node_id_map_[node_id] == node_id);

    //Return the ID of the added node
    return node_id;
}

EdgeId TimingGraph::add_edge(const TimingEdge& new_edge) {
    edge_sink_nodes_.push_back(new_edge.to_node_id());
    edge_src_nodes_.push_back(new_edge.from_node_id());
    edge_delays_.push_back(new_edge.delay());

    EdgeId edge_id = edge_sink_nodes_.size() - 1;

    //Orig Edge ID map
    //Defaults to the current index
    orig_edge_id_map_.push_back(edge_id);
    ASSERT(edge_sink_nodes_.size() == edge_src_nodes_.size());
    ASSERT(edge_sink_nodes_.size() == edge_delays_.size());

    //Verify inex mapping is correct
    ASSERT(orig_edge_id_map_[edge_id] == edge_id);

    //Return the edge id of the added edge
    return edge_id;
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
    //Levelizes the timing graph
    //This over-writes any previous levelization if it exists

    //Clear the previous levelization
    node_levels_.clear();
    primary_outputs_.clear();

    //Allocate space for the first level
    node_levels_.resize(1);

    //Copy the number of input edges per-node
    //These will be decremented to know when a node is done
    //
    //Also initialize the first level (nodes with no fanin)
    std::vector<int> node_fanin_remaining(num_nodes());
    for(NodeId node_id = 0; node_id < num_nodes(); node_id++) {
        int node_fanin = num_node_in_edges(node_id);
        node_fanin_remaining[node_id] = node_fanin;

        if(node_fanin == 0) {
            //Add a primary input
            node_levels_[0].push_back(node_id);
        }
    }

    //Walk the graph from primary inputs (no fanin) to generate a topological sort
    //
    //We inspect the output edges of each node and decrement the fanin count of the
    //target node.  Once the fanin count for a node reaches zero it can be added
    //to the level.
    int level_id = 0;
    bool inserted_node_in_level = true;
    while(inserted_node_in_level) { //If nothing was inserted we are finished
        inserted_node_in_level = false;

        for(const NodeId node_id : node_levels_[level_id]) {
            //Inspect the fanout
            for(int edge_idx = 0; edge_idx < num_node_out_edges(node_id); edge_idx++) {
                EdgeId edge_id = node_out_edge(node_id, edge_idx);
                NodeId sink_node = edge_sink_node(edge_id);

                //Decrement the fanin count
                ASSERT(node_fanin_remaining[sink_node] > 0);
                node_fanin_remaining[sink_node]--;

                //Add to the next level if all fanin has been seen
                if(node_fanin_remaining[sink_node] == 0) {
                    //Ensure there is space by allocating the next level if required
                    node_levels_.resize(level_id+2);

                    //Add the node
                    node_levels_[level_id+1].push_back(sink_node);

                    inserted_node_in_level = true;
                }
            }

            //Also track the primary outputs
            if(num_node_out_edges(node_id) == 0) {
                primary_outputs_.push_back(node_id);
            }
        }
        level_id++;
    }
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
    orig_edge_id_map_ = std::vector<NodeId>(num_edges(), -1);
    int cnt = 0;
    for(std::vector<EdgeId>& edge_level : edge_levels) {
        for(EdgeId orig_edge_id : edge_level) {
            orig_edge_id_map_[orig_edge_id] = cnt;
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
            node_out_edges_[i][j] = orig_edge_id_map_[old_edge_id];
        }
        for(size_t j = 0; j < node_in_edges_[i].size(); j++) {
            EdgeId old_edge_id = node_in_edges_[i][j];
            node_in_edges_[i][j] = orig_edge_id_map_[old_edge_id];
        }
    }
}

std::vector<NodeId> TimingGraph::contiguize_level_nodes() {
    //Make all nodes in a level be contiguous in memory
    std::cout << "Re-allocating nodes so levels are in contiguous memory" << std::endl;

    /*
     * Build a map of the old and new node ids to update edges
     * and node levels later
     */
    orig_node_id_map_ = std::vector<NodeId>(num_nodes(), -1);
    int cnt = 0;
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId node_id : node_levels_[level_idx]) {
            orig_node_id_map_[node_id] = cnt;
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
    std::vector<bool> old_node_is_clock_source;

    //Swap the values
    std::swap(old_node_types, node_types_);
    std::swap(old_node_clock_domains, node_clock_domains_);
    std::swap(old_node_out_edges, node_out_edges_);
    std::swap(old_node_in_edges, node_in_edges_);
    std::swap(old_node_is_clock_source, node_is_clock_source_);

    //Update the values
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(NodeId old_node_id : node_levels_[level_idx]) {
            node_types_.push_back(old_node_types[old_node_id]);
            node_clock_domains_.push_back(old_node_clock_domains[old_node_id]);
            node_out_edges_.push_back(old_node_out_edges[old_node_id]);
            node_in_edges_.push_back(old_node_in_edges[old_node_id]);
            node_is_clock_source_.push_back(old_node_is_clock_source[old_node_id]);
        }
    }

    /*
     * Update old references to node_ids with thier new values
     */
    //The node levels
    for(int level_idx = 0; level_idx < num_levels(); level_idx++) {
        for(size_t i = 0; i < node_levels_[level_idx].size(); i++) {
            NodeId old_node_id = node_levels_[level_idx][i];
            node_levels_[level_idx][i] = orig_node_id_map_[old_node_id];
        }
    }

    //The primary outputs
    for(size_t i = 0; i < primary_outputs_.size(); i++) {
        NodeId old_node_id = primary_outputs_[i];
        primary_outputs_[i] = orig_node_id_map_[old_node_id];
    }

    //The Edges
    for(int i = 0; i < num_edges(); i++) {
        NodeId old_sink_node = edge_sink_nodes_[i];
        NodeId old_src_node = edge_src_nodes_[i];

        edge_sink_nodes_[i] = orig_node_id_map_[old_sink_node];
        edge_src_nodes_[i] = orig_node_id_map_[old_src_node];
    }

    return orig_node_id_map_;
}
