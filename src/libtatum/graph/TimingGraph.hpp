#pragma once

#include <vector>
#include <iosfwd>

#include "timing_graph_fwd.hpp"

/**
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
 * each node and edge in the graph is given a unique identifier (e.g. NodeId, EdgeId). These
 * ID's can then be used to access the required data through the appropriate member function.
 *
 * Implementation
 * ================
 * The 'TimingGraph' class represents the timing graph in a "Struct of Arrays (SoA)" manner,
 * rather than the more typical "Array of Structs (AoS)" data layout.
 *
 * By using a SoA layout we keep all data for a particular field (e.g. node types) in contiguous
 * memory.  Using an AoS layout the various fields accross nodes would *not* be contiguous
 * (although the different fields within each object (e.g. a TimingNode class) would be contiguous.
 * Since we typically perform operations on particular fields accross nodes the SoA layout performs
 * better (and enables memory ordering optimizations). The edges are also stored in a SOA format.
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
 * and ensures that each cache line pulled into the cache will (likely) be accessed multiple times
 * before being evicted.
 *
 * Note that performing these optimizations is currently done explicity by calling the optimize_edge_layout()
 * and optimize_node_layout() member functions.  In the future (particularily if incremental modification
 * support is added), it may be a good idea apply these modifications automatically as needed.
 *
 */
class TimingGraph {
    public:
        /*
         * Node data accessors
         */
        ///\param id The id of a node
        ///\returns The type of the node
        TN_Type node_type(NodeId id) const { return node_types_[id]; }

        ///\param id The id of a node
        ///\returns The clock domain of the node
        DomainId node_clock_domain(const NodeId id) const { return node_clock_domains_[id]; }

        ///\param id The id of a node
        ///\returns Whether the is the source of a clock
        bool node_is_clock_source(const NodeId id) const { return node_is_clock_source_[id]; }

        /*
         * Node edge accessors
         */
        ///\param id The id of a node
        ///\returns The number of out-going edges the node drives
        int num_node_out_edges(const NodeId id) const { return node_out_edges_[id].size(); }

        ///\param id The id of a node
        ///\returns The number of in-coming edges the node sinks
        int num_node_in_edges(const NodeId id) const { return node_in_edges_[id].size(); }

        ///\param node_id The id of a node
        ///\param edge_idx The out-going edge number at this node
        ///\returns The edge id of the edge_idx'th edge driven by node_id
        EdgeId node_out_edge(const NodeId node_id, int edge_idx) const { return node_out_edges_[node_id][edge_idx]; }

        ///\param node_id The id of a node
        ///\param edge_idx The in-coming edge number at this node
        ///\returns The edge id of the edge_idx'th edge sunk by node_id
        EdgeId node_in_edge(const NodeId node_id, int edge_idx) const { return node_in_edges_[node_id][edge_idx]; }

        /*
         * Edge accessors
         */
        ///\param id The id of an edge
        ///\returns The node id of the edge's sink
        NodeId edge_sink_node(const EdgeId id) const { return edge_sink_nodes_[id]; }

        ///\param id The id of an edge
        ///\returns The node id of the edge's source (driver)
        NodeId edge_src_node(const EdgeId id) const { return edge_src_nodes_[id]; }

        /*
         * Graph accessors
         */
        ///\returns The total number of nodes in the graph
        NodeId num_nodes() const { return node_types_.size(); }

        ///\returns The total number of edges in the graph
        EdgeId num_edges() const { return edge_src_nodes_.size(); }

        ///\returns The total number of levels in the graph
        LevelId num_levels() const { return node_levels_.size(); }

        /*
         * Node collection accessors
         */
        ///\param level_id The level index in the graph
        ///\pre The graph must been levelized.
        ///\returns The nodes in the level
        ///\see levelize()
        const std::vector<NodeId>& level(const LevelId level_id) const { return node_levels_[level_id]; }

        ///\pre The graph must be levelized.
        ///\returns The nodes which are primary inputs
        ///\see levelize()
        const std::vector<NodeId>& primary_inputs() const { return node_levels_[0]; } //After levelizing PIs will be 1st level

        ///\pre The primary outputs have been identified.
        ///\returns The nodes which are primary outputs
        ///\warning The primary outputs may be on different levels of the graph
        ///\see levelize()
        const std::vector<NodeId>& primary_outputs() const { return primary_outputs_; }

        /*
         * Graph modifiers
         */
        ///Adds a node to the timing graph
        ///\param type The type of the node to be added
        ///\param clock_domain The clock domain id of the node to be added
        ///\param is_clk_src Identifies if the node to be added is the source of a clock
        ///\warning Graph will likely need to be re-levelized after modification
        NodeId add_node(const TN_Type type, const DomainId clock_domain, const bool is_clk_src);

        ///Adds an edge to the timing graph
        ///\param src_node The node id of the edge's driving node
        ///\param sink_node The node id of the edge's sink node
        ///\pre The src_node and sink_node must have been already added to the graph
        ///\warning Graph will likely need to be re-levelized after modification
        EdgeId add_edge(const NodeId src_node, const NodeId sink_node);

        /*
         * Graph-level modification operations
         */
        ///Levelizes the graph.
        ///\post The graph topologically ordered (i.e. the level of each node is known)
        ///\post The primary outputs have been identified
        void levelize();

        /*
         * Memory layout optimization operations
         */
        ///Optimizes the memory layout of edges in the graph by re-ordering them
        ///for improved spatial/temporal cache locality.
        ///\pre The graph must be levelized
        ///\warning Old edge ids are invalidated
        ///\returns A mapping from old to new edge ids
        ///\see levelize()
        std::vector<EdgeId> optimize_edge_layout();

        ///Optimizes the memory layout of nodes in the graph by re-ordering them
        ///for improved spatial/temporal cache locality.
        ///\pre The graph must be levelized
        ///\warning Old node ids are invalidated
        ///\returns A mapping from old to new node ids
        ///\see levelize()
        std::vector<NodeId> optimize_node_layout();

    private:
        /*
         * For improved memory locality, we use a Struct of Arrays (SoA)
         * data layout, rather than Array of Structs (AoS)
         */
        //Node data
        std::vector<TN_Type> node_types_; //Type of node [0..num_nodes()-1]
        std::vector<DomainId> node_clock_domains_; //Clock domain of node [0..num_nodes()-1]
        std::vector<std::vector<EdgeId>> node_out_edges_; //Out going edge IDs for node 'node_id' [0..num_nodes()-1][0..num_node_out_edges(node_id)-1]
        std::vector<std::vector<EdgeId>> node_in_edges_; //Incomiing edge IDs for node 'node_id' [0..num_nodes()-1][0..num_node_in_edges(node_id)-1]
        std::vector<bool> node_is_clock_source_; //Indicates if a node is the start of clock [0..num_nodes()-1]

        //Edge data
        std::vector<NodeId> edge_sink_nodes_; //Sink node for each edge [0..num_edges()-1]
        std::vector<NodeId> edge_src_nodes_; //Source node for each edge [0..num_edges()-1]

        //Auxilary graph-level info, filled in by levelize()
        std::vector<std::vector<NodeId>> node_levels_; //Nodes in each level [0..num_levels()-1]
        std::vector<NodeId> primary_outputs_; //Primary output nodes of the timing graph.
                                              //NOTE: we track this separetely (unlike Primary Inputs) since these are
                                              //      scattered through the graph and do not exist on a single level
};

/**
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
