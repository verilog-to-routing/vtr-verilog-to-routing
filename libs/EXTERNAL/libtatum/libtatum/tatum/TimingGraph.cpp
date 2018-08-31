#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>

#include "tatum/util/tatum_assert.hpp"
#include "tatum/base/loop_detect.hpp"
#include "tatum/error.hpp"
#include "tatum/TimingGraph.hpp"



namespace tatum {


//Builds a mapping from old to new ids by skipping values marked invalid
template<typename Id>
tatum::util::linear_map<Id,Id> compress_ids(const tatum::util::linear_map<Id,Id>& ids) {
    tatum::util::linear_map<Id,Id> id_map(ids.size());
    size_t i = 0;
    for(auto id : ids) {
        if(id) {
            //Valid
            id_map.insert(id, Id(i));
            ++i;
        }
    }

    return id_map;
}

//Returns a vector based on 'values', which has had entries dropped and 
//re-ordered according according to 'id_map'.
//
// Each entry in id_map corresponds to the assoicated element in 'values'.
// The value of the id_map entry is the new ID of the entry in values.
//
// If it is an invalid ID, the element in values is dropped.
// Otherwise the element is moved to the new ID location.
template<typename Id, typename T>
tatum::util::linear_map<Id,T> clean_and_reorder_values(const tatum::util::linear_map<Id,T>& values, const tatum::util::linear_map<Id,Id>& id_map) {
    TATUM_ASSERT(values.size() == id_map.size());

    //Allocate space for the values that will not be dropped
    tatum::util::linear_map<Id,T> result;

    //Move over the valid entries to their new locations
    for(size_t cur_idx = 0; cur_idx < values.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            //There is a valid mapping
            result.insert(new_id, std::move(values[old_id]));
        }
    }

    return result;
}

//Returns the set of new valid Ids defined by 'id_map'
//TOOD: merge with clean_and_reorder_values
template<typename Id>
tatum::util::linear_map<Id,Id> clean_and_reorder_ids(const tatum::util::linear_map<Id,Id>& id_map) {
    //For IDs, the values are the new id's stored in the map

    //Allocate a new vector to store the values that have been not dropped
    tatum::util::linear_map<Id,Id> result;

    //Move over the valid entries to their new locations
    for(size_t cur_idx = 0; cur_idx < id_map.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            result.insert(new_id, new_id);
        }
    }

    return result;
}

template<typename Container, typename ValId>
Container update_valid_refs(const Container& values, const tatum::util::linear_map<ValId,ValId>& id_map) {
    Container updated;

    for(ValId orig_val : values) {
        if(orig_val) {
            //Original item valid

            ValId new_val = id_map[orig_val];
            if(new_val) {
                //The original item exists in the new mapping
                updated.emplace_back(new_val);
            }
        }
    }
    return updated;
}

//Updates the Ids in 'values' based on id_map, even if the original or new mapping is not valid
template<typename Container, typename ValId>
Container update_all_refs(const Container& values, const tatum::util::linear_map<ValId,ValId>& id_map) {
    Container updated;

    for(ValId orig_val : values) {
        //The original item was valid
        ValId new_val = id_map[orig_val]; 
        //The original item exists in the new mapping
        updated.emplace_back(new_val);
    }

    return updated;
}

//Recursive helper functions for collecting transitively connected nodes
void find_transitive_fanout_nodes_recurr(const TimingGraph& tg, 
                                         std::vector<NodeId>& nodes, 
                                         const NodeId node, 
                                         size_t max_depth=std::numeric_limits<size_t>::max(), 
                                         size_t depth=0);
void find_transitive_fanin_nodes_recurr(const TimingGraph& tg, 
                                        std::vector<NodeId>& nodes, 
                                        const NodeId node, 
                                        size_t max_depth=std::numeric_limits<size_t>::max(), 
                                        size_t depth=0);


EdgeId TimingGraph::node_clock_capture_edge(const NodeId node) const {

    if(node_type(node) == NodeType::SINK) {
        //Only sinks can have clock capture edges

        //Look through the edges for the incoming clock edge
        for(EdgeId edge : node_in_edges(node)) {
            if(edge_type(edge) == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                return edge;
            }
        }
    }

    return EdgeId::INVALID();
}

EdgeId TimingGraph::node_clock_launch_edge(const NodeId node) const {

    if(node_type(node) == NodeType::SOURCE) {
        //Only sources can have clock capture edges

        //Look through the edges for the incoming clock edge
        for(EdgeId edge : node_in_edges(node)) {
            if(edge_type(edge) == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                return edge;
            }
        }
    }

    return EdgeId::INVALID();
}


NodeId TimingGraph::add_node(const NodeType type) {
    //Invalidate the levelization
    is_levelized_ = false;

    //Reserve an ID
    NodeId node_id = NodeId(node_ids_.size());
    node_ids_.push_back(node_id);

    //Type
    node_types_.push_back(type);

    //Edges
    node_out_edges_.push_back(std::vector<EdgeId>());
    node_in_edges_.push_back(std::vector<EdgeId>());

    //Verify sizes
    TATUM_ASSERT(node_types_.size() == node_out_edges_.size());
    TATUM_ASSERT(node_types_.size() == node_in_edges_.size());

    //Return the ID of the added node
    return node_id;
}

EdgeId TimingGraph::add_edge(const EdgeType type, const NodeId src_node, const NodeId sink_node) {
    //We require that the source/sink node must already be in the graph,
    //  so we can update them with thier edge references
    TATUM_ASSERT(valid_node_id(src_node));
    TATUM_ASSERT(valid_node_id(sink_node));

    //Invalidate the levelization
    is_levelized_ = false;

    //Reserve an edge ID
    EdgeId edge_id = EdgeId(edge_ids_.size());
    edge_ids_.push_back(edge_id);

    //Create the edgge
    edge_types_.push_back(type);
    edge_src_nodes_.push_back(src_node);
    edge_sink_nodes_.push_back(sink_node);
    edges_disabled_.push_back(false);

    //Verify
    TATUM_ASSERT(edge_sink_nodes_.size() == edge_src_nodes_.size());

    //Update the nodes the edge references
    node_out_edges_[src_node].push_back(edge_id);
    node_in_edges_[sink_node].push_back(edge_id);

    TATUM_ASSERT(edge_type(edge_id) == type);
    TATUM_ASSERT(edge_src_node(edge_id) == src_node);
    TATUM_ASSERT(edge_sink_node(edge_id) == sink_node);

    //Return the edge id of the added edge
    return edge_id;
}


void TimingGraph::remove_node(const NodeId node_id) {
    TATUM_ASSERT(valid_node_id(node_id));

    //Invalidate the levelization
    is_levelized_ = false;

    //Invalidate all the references
    for(EdgeId in_edge : node_in_edges(node_id)) {
        if(!in_edge) continue;

        remove_edge(in_edge);
    }

    for(EdgeId out_edge : node_out_edges(node_id)) {
        if(!out_edge) continue;

        remove_edge(out_edge);
    }

    //Mark the node as invalid
    node_ids_[node_id] = NodeId::INVALID();
}

void TimingGraph::remove_edge(const EdgeId edge_id) {
    TATUM_ASSERT(valid_edge_id(edge_id));

    //Invalidate the levelization
    is_levelized_ = false;

    //Invalidate the upstream node to edge references
    NodeId src_node = edge_src_node(edge_id);    
    auto iter_out = std::find(node_out_edges_[src_node].begin(), node_out_edges_[src_node].end(), edge_id);
    TATUM_ASSERT(iter_out != node_out_edges_[src_node].end());
    *iter_out = EdgeId::INVALID();

    //Invalidate the downstream node to edge references
    NodeId sink_node = edge_sink_node(edge_id);    
    auto iter_in = std::find(node_in_edges_[sink_node].begin(), node_in_edges_[sink_node].end(), edge_id);
    TATUM_ASSERT(iter_in != node_in_edges_[sink_node].end());
    *iter_in = EdgeId::INVALID();

    //Mark the edge invalid
    edge_ids_[edge_id] = EdgeId::INVALID();
}

void TimingGraph::disable_edge(const EdgeId edge, bool disable) {
    TATUM_ASSERT(valid_edge_id(edge));

    if(edges_disabled_[edge] != disable) {
        //If we are changing edges the levelization is no longer valid
        is_levelized_ = false;
    }

    //Update the edge's disabled flag
    edges_disabled_[edge] = disable;
}

EdgeType TimingGraph::edge_type(const EdgeId edge) const {
    TATUM_ASSERT(valid_edge_id(edge));

    return edge_types_[edge];
}

EdgeId TimingGraph::find_edge(const tatum::NodeId src_node, const tatum::NodeId sink_node) const {
    TATUM_ASSERT(valid_node_id(src_node));
    TATUM_ASSERT(valid_node_id(sink_node));

    for(EdgeId edge : node_out_edges(src_node)) {
        if(edge_sink_node(edge) == sink_node) {
            return edge;
        }
    }
    return EdgeId::INVALID();
}

GraphIdMaps TimingGraph::compress() {
    auto node_id_map = compress_ids(node_ids_);
    auto edge_id_map = compress_ids(edge_ids_);

    remap_nodes(node_id_map);
    remap_edges(edge_id_map);

    levelize();
    validate();

    return {node_id_map, edge_id_map};
}

void TimingGraph::levelize() {
    if(!is_levelized_) {
        force_levelize();
    }
}

void TimingGraph::force_levelize() {
    //Levelizes the timing graph
    //This over-writes any previous levelization if it exists.
    //
    //Also records primary outputs

    //Clear any previous levelization
    level_nodes_.clear();
    level_ids_.clear();
    primary_inputs_.clear();
    logical_outputs_.clear();

    //Allocate space for the first level
    level_nodes_.resize(1);

    //Copy the number of input edges per-node
    //These will be decremented to know when all a node's upstream parents have been
    //placed in a previous level (indicating that the node goes in the current level)
    //
    //Also initialize the first level (nodes with no fanin)
    std::vector<int> node_fanin_remaining(nodes().size());
    for(NodeId node_id : nodes()) {
        size_t node_fanin = 0;
        for(EdgeId edge : node_in_edges(node_id)) {
            if(edge_disabled(edge)) continue;
            ++node_fanin;
        }
        node_fanin_remaining[size_t(node_id)] = node_fanin;

        //Initialize the first level
        if(node_fanin == 0) {
            level_nodes_[LevelId(0)].push_back(node_id);

            if (node_type(node_id) == NodeType::SOURCE) {
                //We require that all primary inputs (i.e. top-level circuit inputs) to
                //be SOURCEs. Due to disconnected nodes we may have non-SOURCEs which 
                //appear in the first level.
                primary_inputs_.push_back(node_id);
            }
        }

    }

    //Walk the graph from primary inputs (no fanin) to generate a topological sort
    //
    //We inspect the output edges of each node and decrement the fanin count of the
    //target node.  Once the fanin count for a node reaches zero it can be added
    //to the current level.
    int level_idx = 0;
    level_ids_.emplace_back(level_idx);

    bool inserted_node_in_level = true;
    while(inserted_node_in_level) { //If nothing was inserted we are finished
        inserted_node_in_level = false;

        for(const NodeId node_id : level_nodes_[LevelId(level_idx)]) {
            //Inspect the fanout
            for(EdgeId edge_id : node_out_edges(node_id)) {
                if(edge_disabled(edge_id)) continue;

                NodeId sink_node = edge_sink_node(edge_id);

                //Decrement the fanin count
                TATUM_ASSERT(node_fanin_remaining[size_t(sink_node)] > 0);
                node_fanin_remaining[size_t(sink_node)]--;

                //Add to the next level if all fanin has been seen
                if(node_fanin_remaining[size_t(sink_node)] == 0) {
                    //Ensure there is space by allocating the next level if required
                    level_nodes_.resize(level_idx+2);

                    //Add the node
                    level_nodes_[LevelId(level_idx+1)].push_back(sink_node);

                    inserted_node_in_level = true;
                }
            }

            //Also track the primary outputs (those with fan-in AND no fan-out)
            //
            // There may be some node with neither any fan-in or fan-out. 
            // We will treat them as primary inputs, so they should not be to
            // the primary outputs
            if(   node_out_edges(node_id).size() == 0 
               && node_in_edges(node_id).size() != 0
               && node_type(node_id) == NodeType::SINK) {
                logical_outputs_.push_back(node_id);
            }
        }

        if(inserted_node_in_level) {
            level_idx++;
            level_ids_.emplace_back(level_idx);
        }
    }

    //Mark the levelization as valid
    is_levelized_ = true;
}

bool TimingGraph::validate() const {
    bool valid = true;
    valid &= validate_sizes();
    valid &= validate_values();
    valid &= validate_structure();

    return valid;
}

GraphIdMaps TimingGraph::optimize_layout() {
    auto node_id_map = optimize_node_layout();
    remap_nodes(node_id_map);

    levelize();

    auto edge_id_map = optimize_edge_layout();
    remap_edges(edge_id_map);

    levelize();

    return {node_id_map, edge_id_map};
}

tatum::util::linear_map<EdgeId,EdgeId> TimingGraph::optimize_edge_layout() const {
    //Make all edges in a level be contiguous in memory

    //Determine the edges driven by each level of the graph
    std::vector<std::vector<EdgeId>> edge_levels;
    for(LevelId level_id : levels()) {
        edge_levels.push_back(std::vector<EdgeId>());
        for(auto node_id : level_nodes(level_id)) {

            //We walk the nodes according to the input-edge order.
            //This is the same order used by the arrival-time traversal (which is responsible
            //for most of the analyzer run-time), so matching it's order exactly results in
            //better cache locality
            for(EdgeId edge_id : node_in_edges(node_id)) {

                //edge_id is driven by nodes in level level_idx
                edge_levels[size_t(level_id)].push_back(edge_id);
            }
        }
    }

    //Maps from from original to new edge id, used to update node to edge refs
    tatum::util::linear_map<EdgeId,EdgeId> orig_to_new_edge_id(edges().size());

    //Determine the new order
    size_t iedge = 0;
    for(auto& edge_level : edge_levels) {
        for(const EdgeId orig_edge_id : edge_level) {

            //Save the new edge id to update nodes
            orig_to_new_edge_id[orig_edge_id] = EdgeId(iedge);

            ++iedge;
        }
    }

    for(auto new_id : orig_to_new_edge_id) {
        TATUM_ASSERT(new_id);
    }
    TATUM_ASSERT(iedge == edges().size());

    return orig_to_new_edge_id;
}

tatum::util::linear_map<NodeId,NodeId> TimingGraph::optimize_node_layout() const {
    //Make all nodes in a level be contiguous in memory

    /*
     * Keep a map of the old and new node ids to update edges
     * and node levels later
     */
    tatum::util::linear_map<NodeId,NodeId> orig_to_new_node_id(nodes().size());

    //Determine the new order
    size_t inode = 0;
    for(const LevelId level_id : levels()) {
        for(const NodeId old_node_id : level_nodes(level_id)) {
            //Record the new node id
            orig_to_new_node_id[old_node_id] = NodeId(inode);
            ++inode;
        }
    }

    for(auto new_id : orig_to_new_node_id) {
        TATUM_ASSERT(new_id);
    }
    TATUM_ASSERT(inode == nodes().size());

    return orig_to_new_node_id;
}

void TimingGraph::remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_id_map) {
    is_levelized_ = false;

    //Update values
    node_ids_ = clean_and_reorder_ids(node_id_map);
    node_types_ = clean_and_reorder_values(node_types_, node_id_map);
    node_in_edges_ = clean_and_reorder_values(node_in_edges_, node_id_map);
    node_out_edges_ = clean_and_reorder_values(node_out_edges_, node_id_map);

    //Update references
    edge_src_nodes_ = update_all_refs(edge_src_nodes_, node_id_map);
    edge_sink_nodes_ = update_all_refs(edge_sink_nodes_, node_id_map);
}

void TimingGraph::remap_edges(const tatum::util::linear_map<EdgeId,EdgeId>& edge_id_map) {
    is_levelized_ = false;

    //Update values
    edge_ids_ = clean_and_reorder_ids(edge_id_map);
    edge_types_ = clean_and_reorder_values(edge_types_, edge_id_map);
    edge_sink_nodes_ = clean_and_reorder_values(edge_sink_nodes_, edge_id_map);
    edge_src_nodes_ = clean_and_reorder_values(edge_src_nodes_, edge_id_map);
    edges_disabled_ = clean_and_reorder_values(edges_disabled_, edge_id_map);

    //Update cross-references
    for(auto& edges_ref : node_in_edges_) {
        edges_ref = update_valid_refs(edges_ref, edge_id_map);
    }
    for(auto& edges_ref : node_out_edges_) {
        edges_ref = update_valid_refs(edges_ref, edge_id_map);
    }
}

bool TimingGraph::valid_node_id(const NodeId node_id) const {
    return size_t(node_id) < node_ids_.size();
}

bool TimingGraph::valid_edge_id(const EdgeId edge_id) const {
    return size_t(edge_id) < edge_ids_.size();
}

bool TimingGraph::valid_level_id(const LevelId level_id) const {
    return size_t(level_id) < level_ids_.size();
}

bool TimingGraph::validate_sizes() const {
    if (   node_ids_.size() != node_types_.size()
        || node_ids_.size() != node_in_edges_.size()
        || node_ids_.size() != node_out_edges_.size()) {
        throw tatum::Error("Inconsistent node attribute sizes");
    }

    if (   edge_ids_.size() != edge_types_.size()
        || edge_ids_.size() != edge_sink_nodes_.size()
        || edge_ids_.size() != edge_src_nodes_.size()
        || edge_ids_.size() != edges_disabled_.size()) {
        throw tatum::Error("Inconsistent edge attribute sizes");
    }

    if (level_ids_.size() != level_nodes_.size()) {
        throw tatum::Error("Inconsistent level attribute sizes");
    }

    return true;
}

bool TimingGraph::validate_values() const {

    for(NodeId node_id : nodes()) {
        if(!valid_node_id(node_id)) {
            throw tatum::Error("Invalid node id", node_id);
        }

        for(EdgeId edge_id : node_in_edges_[node_id]) {
            if(!valid_edge_id(edge_id)) {
                throw tatum::Error("Invalid node-in-edge reference", node_id, edge_id);
            }

            //Check that the references are consistent
            if(edge_sink_nodes_[edge_id] != node_id) {
                throw tatum::Error("Mismatched edge-sink/node-in-edge reference", node_id, edge_id);
            }
        }
        for(EdgeId edge_id : node_out_edges_[node_id]) {
            if(!valid_edge_id(edge_id)) {
                throw tatum::Error("Invalid node-out-edge reference", node_id, edge_id);
            }

            //Check that the references are consistent
            if(edge_src_nodes_[edge_id] != node_id) {
                throw tatum::Error("Mismatched edge-src/node-out-edge reference", node_id, edge_id);
            }
        }
    }
    for(EdgeId edge_id : edges()) {
        if(!valid_edge_id(edge_id)) {
            throw tatum::Error("Invalid edge id", edge_id);
        }
        NodeId src_node = edge_src_nodes_[edge_id];
        if(!valid_node_id(src_node)) {
            throw tatum::Error("Invalid edge source node", src_node, edge_id);
        }

        NodeId sink_node = edge_sink_nodes_[edge_id];
        if(!valid_node_id(sink_node)) {
            throw tatum::Error("Invalid edge sink node", sink_node, edge_id);
        }
    }

    //TODO: more checking

    return true;
}

bool TimingGraph::validate_structure() const {
    //Verify that the timing graph connectivity is as expected

    if (!is_levelized_) {
        throw tatum::Error("Timing graph must be levelized for structural validation");
    }

    for(NodeId src_node : nodes()) {

        NodeType src_type = node_type(src_node);

        auto out_edges = node_out_edges(src_node);
        auto in_edges = node_in_edges(src_node);

        //Check expected number of fan-in/fan-out edges
        if(src_type == NodeType::SOURCE) {
            if(in_edges.size() > 1) {
                throw tatum::Error("SOURCE node has more than one active incoming edge (expected 0 if primary input, or 1 if clocked)", src_node);
            }
        } else if (src_type == NodeType::SINK) {
            if(out_edges.size() > 0) {
                throw tatum::Error("SINK node has out-going edges", src_node);
            }
        } else if (src_type == NodeType::IPIN) {
            if(in_edges.size() == 0 && !allow_dangling_combinational_nodes_) {
                throw tatum::Error("IPIN has no in-coming edges", src_node);
            }
            if(out_edges.size() == 0 && !allow_dangling_combinational_nodes_) {
                throw tatum::Error("IPIN has no out-going edges", src_node);
            }
        } else if (src_type == NodeType::OPIN) {
            //May have no incoming edges if a constant generator, so don't check that case

            if(out_edges.size() == 0 && !allow_dangling_combinational_nodes_) {
                throw tatum::Error("OPIN has no out-going edges", src_node);
            }
        } else {
            TATUM_ASSERT(src_type == NodeType::CPIN);
            if(in_edges.size() == 0) {
                throw tatum::Error("CPIN has no in-coming edges", src_node);
            }
            //We do not check for out-going cpin edges, since there is no reason that
            //a clock pin must be used
        }
        
        //Check node-type edge connectivity
        for(EdgeId out_edge : node_out_edges(src_node)) {
            NodeId sink_node = edge_sink_node(out_edge);
            NodeType sink_type = node_type(sink_node);

            EdgeType out_edge_type = edge_type(out_edge);

            //Check type connectivity
            if (src_type == NodeType::SOURCE) {

                if(   sink_type != NodeType::IPIN
                   && sink_type != NodeType::OPIN
                   && sink_type != NodeType::CPIN
                   && sink_type != NodeType::SINK) {
                    throw tatum::Error("SOURCE nodes should only drive IPIN, OPIN, CPIN or SINK nodes", src_node, out_edge);
                }

                if(sink_type == NodeType::SINK) {
                    if(   out_edge_type != EdgeType::INTERCONNECT
                       && out_edge_type != EdgeType::PRIMITIVE_COMBINATIONAL) {
                        throw tatum::Error("SOURCE to SINK edges should always be either INTERCONNECT or PRIMTIIVE_COMBINATIONAL type edges", src_node, out_edge);
                    }
                    
                } else if (sink_type == NodeType::OPIN) {
                    if(out_edge_type != EdgeType::PRIMITIVE_COMBINATIONAL) {
                        throw tatum::Error("SOURCE to OPIN edges should always be PRIMITIVE_COMBINATIONAL type edges", src_node, out_edge);
                    }
                } else {
                    TATUM_ASSERT(sink_type == NodeType::IPIN || sink_type == NodeType::CPIN);
                    if(out_edge_type != EdgeType::INTERCONNECT) {
                        throw tatum::Error("SOURCE to IPIN/CPIN edges should always be INTERCONNECT type edges", src_node, out_edge);
                    }
                }

            } else if (src_type == NodeType::SINK) {
                throw tatum::Error("SINK nodes should not have out-going edges", sink_node);
            } else if (src_type == NodeType::IPIN) {
                if(sink_type != NodeType::OPIN && sink_type != NodeType::SINK) {
                    throw tatum::Error("IPIN nodes should only drive OPIN or SINK nodes", src_node, out_edge);
                }

                if(out_edge_type != EdgeType::PRIMITIVE_COMBINATIONAL) {
                    throw tatum::Error("IPIN to OPIN/SINK edges should always be PRIMITIVE_COMBINATIONAL type edges", src_node, out_edge);
                }

            } else if (src_type == NodeType::OPIN) {
                if(   sink_type != NodeType::IPIN
                   && sink_type != NodeType::CPIN
                   && sink_type != NodeType::SINK) {
                    throw tatum::Error("OPIN nodes should only drive IPIN, CPIN or SINK nodes", src_node, out_edge);
                }

                if(out_edge_type != EdgeType::INTERCONNECT) {
                    throw tatum::Error("OPIN out edges should always be INTERCONNECT type edges", src_node, out_edge);
                }

            } else if (src_type == NodeType::CPIN) {
                if(   sink_type != NodeType::SOURCE
                   && sink_type != NodeType::SINK) {
                    throw tatum::Error("CPIN nodes should only drive SOURCE or SINK nodes", src_node, out_edge);
                }

                if(sink_type == NodeType::SOURCE && out_edge_type != EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    throw tatum::Error("CPIN to SOURCE edges should always be PRIMITIVE_CLOCK_LAUNCH type edges", src_node, out_edge);
                } else if (sink_type == NodeType::SINK && out_edge_type != EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    throw tatum::Error("CPIN to SINK edges should always be PRIMITIVE_CLOCK_CAPTURE type edges", src_node, out_edge);
                }
            } else {
                throw tatum::Error("Unrecognized node type", src_node, out_edge);
            }
        }
    }

    //Record the nodes assoicated with each edge
    std::map<std::pair<NodeId,NodeId>,std::vector<EdgeId>> edge_nodes;
    for(EdgeId edge : edges()) {
        NodeId src_node = edge_src_node(edge);
        NodeId sink_node = edge_sink_node(edge);

        edge_nodes[{src_node,sink_node}].push_back(edge);
    }

    //Check for duplicate edges between pairs of nodes
    for(const auto& kv : edge_nodes) {
        const auto& edge_ids = kv.second;

        TATUM_ASSERT_MSG(edge_ids.size() > 0, "Node pair must have at least one edge");
        if(edge_ids.size() > 1) {
            NodeId src_node = kv.first.first;
            NodeId sink_node = kv.first.second;
            std::stringstream ss;
            ss << "Dulplicate timing edges found " << src_node << " -> " << sink_node
               << ", duplicate edges: ";
            for(EdgeId edge : edge_ids) {
                ss << edge << " ";
            }
            throw tatum::Error(ss.str(), src_node);
        }
    }

    for(NodeId node : primary_inputs()) {
        if(!node_in_edges(node).empty()) {
            throw tatum::Error("Primary input nodes should have no incoming edges", node);
        }

        if(node_type(node) != NodeType::SOURCE) {
            throw tatum::Error("Primary inputs should be only SOURCE nodes", node);
        }
    }

    for(NodeId node : logical_outputs()) {
        if(!node_out_edges(node).empty()) {
            throw tatum::Error("Logical output node should have no outgoing edges", node);
        }

        if(node_type(node) != NodeType::SINK) {
            throw tatum::Error("Logical outputs should be only SINK nodes", node);
        }
    }

    auto sccs = identify_combinational_loops(*this);
    if(!sccs.empty()) {
        throw Error("Timing graph contains active combinational loops. "
                    "Either disable timing edges (to break the loops) or restructure the graph."); 

        //Future work: 
        //
        //  We could handle this internally by identifying the incoming and outgoing edges of the SCC,
        //  and estimating a 'max' delay through the SCC from each incoming to each outgoing edge.
        //  The SCC could then be replaced with a clique between SCC input and output edges.
        //
        //  One possible estimate is to trace the longest path through the SCC without visiting a node 
        //  more than once (although this is not gaurenteed to be conservative). 
    }


    return true;
}

size_t TimingGraph::count_active_edges(edge_range edges_to_check) const {
    size_t active_cnt = 0;
    for(EdgeId edge : edges_to_check) {
        if(!edge_disabled(edge)) {
            ++active_cnt;
        }
    }
    return active_cnt;
}

//Returns sets of nodes involved in combinational loops
std::vector<std::vector<NodeId>> identify_combinational_loops(const TimingGraph& tg) {
    constexpr size_t MIN_LOOP_SCC_SIZE = 2; //Any SCC of size >= 2 is a loop in the timing graph
    return identify_strongly_connected_components(tg, MIN_LOOP_SCC_SIZE);
}

std::vector<NodeId> find_transitively_connected_nodes(const TimingGraph& tg, 
                                                      const std::vector<NodeId> through_nodes, 
                                                      size_t max_depth) {
    std::vector<NodeId> nodes;

    for(NodeId through_node : through_nodes) {
        find_transitive_fanin_nodes_recurr(tg, nodes, through_node, max_depth);
        find_transitive_fanout_nodes_recurr(tg, nodes, through_node, max_depth);
    }

    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

    return nodes;
}

std::vector<NodeId> find_transitive_fanin_nodes(const TimingGraph& tg, 
                                                const std::vector<NodeId> sinks, 
                                                size_t max_depth) {
    std::vector<NodeId> nodes;

    for(NodeId sink : sinks) {
        find_transitive_fanin_nodes_recurr(tg, nodes, sink, max_depth);
    }

    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

    return nodes;
}

std::vector<NodeId> find_transitive_fanout_nodes(const TimingGraph& tg, 
                                                 const std::vector<NodeId> sources, 
                                                 size_t max_depth) {
    std::vector<NodeId> nodes;

    for(NodeId source : sources) {
        find_transitive_fanout_nodes_recurr(tg, nodes, source, max_depth);
    }

    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

    return nodes;
}

void find_transitive_fanin_nodes_recurr(const TimingGraph& tg, 
                                        std::vector<NodeId>& nodes, 
                                        const NodeId node, 
                                        size_t max_depth, 
                                        size_t depth) {
    if(depth > max_depth) return;

    nodes.push_back(node);

    for(EdgeId in_edge : tg.node_in_edges(node)) {
        if(tg.edge_disabled(in_edge)) continue;
        NodeId src_node = tg.edge_src_node(in_edge);
        find_transitive_fanin_nodes_recurr(tg, nodes, src_node, max_depth, depth + 1);
    }
}

void find_transitive_fanout_nodes_recurr(const TimingGraph& tg, 
                                         std::vector<NodeId>& nodes,
                                         const NodeId node,
                                         size_t max_depth,
                                         size_t depth) {
    if(depth > max_depth) return;

    nodes.push_back(node);

    for(EdgeId out_edge : tg.node_out_edges(node)) {
        if(tg.edge_disabled(out_edge)) continue;
        NodeId sink_node = tg.edge_sink_node(out_edge);
        find_transitive_fanout_nodes_recurr(tg, nodes, sink_node, max_depth, depth+1);
    }
}

EdgeType infer_edge_type(const TimingGraph& tg, EdgeId edge) {
    NodeId src_node = tg.edge_src_node(edge);
    NodeId sink_node = tg.edge_sink_node(edge);

    NodeType src_node_type = tg.node_type(src_node);
    NodeType sink_node_type = tg.node_type(sink_node);

    if(src_node_type == NodeType::IPIN && sink_node_type == NodeType::OPIN) {
        return EdgeType::PRIMITIVE_COMBINATIONAL;
    } else if (   (src_node_type == NodeType::OPIN || src_node_type == NodeType::SOURCE)
               && (sink_node_type == NodeType::IPIN || sink_node_type == NodeType::SINK || sink_node_type == NodeType::CPIN)) {
        return EdgeType::INTERCONNECT;
    } else if (src_node_type == NodeType::CPIN) {
        if (sink_node_type == NodeType::SOURCE) {
            return EdgeType::PRIMITIVE_CLOCK_LAUNCH;
        } else if(sink_node_type == NodeType::SINK) {
            return EdgeType::PRIMITIVE_CLOCK_CAPTURE;
        } else {
            throw tatum::Error("Invalid edge sink node (CPIN source node must connect to SOURCE or SINK sink node)", sink_node, edge);
        }
    } else {
        throw tatum::Error("Invalid edge sink/source nodes", edge);
    }
}

//Stream output for NodeType
std::ostream& operator<<(std::ostream& os, const NodeType type) {
    if      (type == NodeType::SOURCE)              os << "SOURCE";
    else if (type == NodeType::SINK)                os << "SINK";
    else if (type == NodeType::IPIN)                os << "IPIN";
    else if (type == NodeType::OPIN)                os << "OPIN";
    else if (type == NodeType::CPIN)                os << "CPIN";
    else throw std::domain_error("Unrecognized NodeType");
    return os;
}

//Stream output for EdgeType
std::ostream& operator<<(std::ostream& os, const EdgeType type) {
    if      (type == EdgeType::PRIMITIVE_COMBINATIONAL) os << "PRIMITIVE_COMBINATIONAL";
    else if (type == EdgeType::PRIMITIVE_CLOCK_LAUNCH)  os << "PRIMITIVE_CLOCK_LAUNCH"; 
    else if (type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) os << "PRIMITIVE_CLOCK_CAPTURE";
    else if (type == EdgeType::INTERCONNECT)            os << "INTERCONNECT";
    else throw std::domain_error("Unrecognized EdgeType");
    return os;
}

std::ostream& operator<<(std::ostream& os, NodeId node_id) {
    if(node_id == NodeId::INVALID()) {
        return os << "Node(INVALID)";
    } else {
        return os << "Node(" << size_t(node_id) << ")";
    }
}

std::ostream& operator<<(std::ostream& os, EdgeId edge_id) {
    if(edge_id == EdgeId::INVALID()) {
        return os << "Edge(INVALID)";
    } else {
        return os << "Edge(" << size_t(edge_id) << ")";
    }
}

std::ostream& operator<<(std::ostream& os, DomainId domain_id) {
    if(domain_id == DomainId::INVALID()) {
        return os << "Domain(INVALID)";
    } else {
        return os << "Domain(" << size_t(domain_id) << ")";
    }
}

std::ostream& operator<<(std::ostream& os, LevelId level_id) {
    if(level_id == LevelId::INVALID()) {
        return os << "Level(INVALID)";
    } else {
        return os << "Level(" << size_t(level_id) << ")";
    }
}


} //namepsace

