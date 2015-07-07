#pragma once

#include <vector>
#include <map>
#include <forward_list>
#include <iostream>
#include <iomanip>

#include "timing_graph_fwd.hpp"

/*
 * The 'TimingGraph' class represents a timing graph.
 *
 * Logically the timing graph is a directed graph connecting Primary Inputs (nodes with no
 * fan-in, e.g. circuit inputs Flip-Flop Q pins) to Primary Outputs (nodes with no fan-out,
 * e.g. circuit outputs, Flip-Flop D pins), connecting through intermediate nodes (nodes with
 * both fan-in and fan-out, e.g. combinational logic).
 *
 * To make performing the forward/backward traversals through the timing graph easier, we actually
 * store all edges as bi-directional edges.
 *
 * NOTE: We store only the static connectivity and node information in the 'TimingGraph' class.
 *       Other dynamic information (edge delays, node arrival/required times) is stored seperately.
 *       This means that most actions opearting on the timing graph (e.g. TimingAnalyzers) only
 *       require read-only access to the timing graph.
 *
 * Accessing Graph Data
 * ======================
 * For performance reasons (see Implementation section for details) we store all graph data
 * in the 'TimingGraph' class, and do not use separate edge/node objects.  To facilitate this,
 * each node and edge in the graph is given a unique identifier (e.g. NodeId or EdgeId). These
 * ID's can then be used to access the required data through the appropriate member function.
 *
 * Implementation
 * ================
 * The 'TimingGraph' class represents the timing graph in a "Struct of Arrays (SoA)" manner,
 * rather than as an "Array of Structs (AoS)" which would be the more inuitive data layout.
 *
 * By using a SoA layout we keep all data for a particular field (e.g. node types) in a contiguous
 * memory.  Using an AoS layout, while each object (e.g. a TimingNode class) would be contiguous the
 * various fields accross nodes would NOT be contiguous.  Since we typically perform operations on
 * particular fields accross nodes the SoA layout performs better.
 *
 * The SoA layout also motivates the ID based approach, which allows direct indexing into the required
 * vector to retrieve data.
 *
 * Memory Ordering Optimizations
 * ===============================
 * SoA also allows several additional memory layout optimizations.  In particular,  we know the
 * order that a (serial) timing analyzer will walk the timing graph (i.e. level-by-level, from the
 * start to end node in each level).
 *
 * Using this information we can re-arrange the node and edge data to match this traversal order.
 * This greatly improves caching behaviour, since pulling in data for one node immediately pulls
 * in data for the next node/edge to be processed. This exploits both spatial and temporal locality,
 * and ensures that each cache line pulled into the cache will likely be accessed multiple times before
 * being evicted.
 *
 * Note that performing these optimizations is currently done explicity by calling the optimize_edge_layout()
 * and optimize_node_layout() member functions.  In the future (particularily if incremental modification
 * support is added), it may be a good idea apply these modifications automatically as needed.
 *
 */
class TimingGraph {
    public:
        //Node accessors
        TN_Type node_type(NodeId id) const { return node_types_[id]; }
        DomainId node_clock_domain(const NodeId id) const { return node_clock_domains_[id]; }
        bool node_is_clock_source(const NodeId id) const { return node_is_clock_source_[id]; }
        int num_node_out_edges(const NodeId id) const { return node_out_edges_[id].size(); }
        int num_node_in_edges(const NodeId id) const { return node_in_edges_[id].size(); }
        EdgeId node_out_edge(const NodeId node_id, int edge_idx) const { return node_out_edges_[node_id][edge_idx]; }
        EdgeId node_in_edge(const NodeId node_id, int edge_idx) const { return node_in_edges_[node_id][edge_idx]; }

        //Edge accessors
        NodeId edge_sink_node(const EdgeId id) const { return edge_sink_nodes_[id]; }
        NodeId edge_src_node(const EdgeId id) const { return edge_src_nodes_[id]; }

        //Graph accessors
        NodeId num_nodes() const { return node_types_.size(); }
        EdgeId num_edges() const { return edge_src_nodes_.size(); }
        LevelId num_levels() const { return node_levels_.size(); }

        //Node collection operations
        const std::vector<NodeId>& level(const NodeId level_id) const { return node_levels_[level_id]; }
        const std::vector<NodeId>& primary_inputs() const { return node_levels_[0]; }
        const std::vector<NodeId>& primary_outputs() const { return primary_outputs_; }

        //Graph modifiers
        NodeId add_node(const TN_Type type, const DomainId clock_domain, const bool is_clk_src);
        EdgeId add_edge(const NodeId src_node, const NodeId sink_node);

        //Graph-level operations
        void levelize();
        std::vector<EdgeId> optimize_edge_layout();
        std::vector<NodeId> optimize_node_layout();

    private:
        /*
         * For improved memory locality, we use a Struct of Arrays (SoA)
         * data layout, rather than Array of Structs (AoS)
         */
        //Node data
        std::vector<TN_Type> node_types_;
        std::vector<DomainId> node_clock_domains_;
        std::vector<std::vector<EdgeId>> node_out_edges_;
        std::vector<std::vector<EdgeId>> node_in_edges_;
        std::vector<bool> node_is_clock_source_;

        //Edge data
        std::vector<EdgeId> edge_sink_nodes_;
        std::vector<EdgeId> edge_src_nodes_;

        //Auxilary info
        std::vector<std::vector<NodeId>> node_levels_;
        std::vector<NodeId> primary_outputs_;
};

/*
 * Potential types for nodes in the timing graph
 */
enum class TN_Type {
	INPAD_SOURCE, //Driver of an input I/O pad
	INPAD_OPIN, //Output pin of an input I/O pad
	OUTPAD_IPIN, //Input pin of an output I/O pad
	OUTPAD_SINK, //Sink of an output I/O pad
	PRIMITIVE_IPIN, //Input pin to a primitive (e.g. LUT)
	PRIMITIVE_OPIN, //Output pin from a primitive (e.g. LUT)
	FF_IPIN, //Input pin to a flip-flop - goes to FF_SINK
	FF_OPIN, //Output pin from a flip-flop - comes from FF_SOURCE
	FF_SINK, //Sink (D) pin of flip-flop
	FF_SOURCE, //Source (Q) pin of flip-flop
	FF_CLOCK, //Clock pin of flip-flop
    CLOCK_SOURCE, //A clock generator such as a PLL
    CLOCK_OPIN, //Output pin from an on-chip clock source - comes from CLOCK_SOURCE
	CONSTANT_GEN_SOURCE, //Source of a constant logic 1 or 0
    UNKOWN //Unrecognized type, if encountered this is almost certainly an error
};

//Stream operators for TN_Type
std::ostream& operator<<(std::ostream& os, const TN_Type type);
std::istream& operator>>(std::istream& os, TN_Type& type);
