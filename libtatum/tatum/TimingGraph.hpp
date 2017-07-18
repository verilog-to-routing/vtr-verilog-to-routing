#pragma once
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
#include <vector>
#include <set>
#include <limits>

#include "tatum/util/tatum_range.hpp"
#include "tatum/util/tatum_linear_map.hpp"
#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

class TimingGraph {
    public: //Public types
        //Iterators
        typedef tatum::util::linear_map<EdgeId,EdgeId>::const_iterator edge_iterator;
        typedef tatum::util::linear_map<NodeId,NodeId>::const_iterator node_iterator;
        typedef tatum::util::linear_map<LevelId,LevelId>::const_iterator level_iterator;
        typedef tatum::util::linear_map<LevelId,LevelId>::const_reverse_iterator reverse_level_iterator;

        //Ranges
        typedef tatum::util::Range<node_iterator> node_range;
        typedef tatum::util::Range<edge_iterator> edge_range;
        typedef tatum::util::Range<level_iterator> level_range;
        typedef tatum::util::Range<reverse_level_iterator> reverse_level_range;
    public: //Public accessors
        /*
         * Node data accessors
         */
        ///\param id The id of a node
        ///\returns The type of the node
        NodeType node_type(const NodeId id) const { return node_types_[id]; }

        ///\param id The node id
        ///\returns A range of all out-going edges the node drives
        edge_range node_out_edges(const NodeId id) const { return tatum::util::make_range(node_out_edges_[id].begin(), node_out_edges_[id].end()); }

        ///\param id The node id
        ///\returns A range of all in-coming edges the node drives
        edge_range node_in_edges(const NodeId id) const { return tatum::util::make_range(node_in_edges_[id].begin(), node_in_edges_[id].end()); }


        ///\param id The node id
        ///\returns The edge id corresponding to the incoming clock capture edge, or EdgeId::INVALID() if none
        EdgeId node_clock_capture_edge(const NodeId id) const;

        ///\param id The node id
        ///\returns The edge id corresponding to the incoming clock launch edge, or EdgeId::INVALID() if none
        EdgeId node_clock_launch_edge(const NodeId id) const;

        /*
         * Edge accessors
         */
        ///\param edge The id of an edge
        ///\returns The type of the edge
        EdgeType edge_type(const EdgeId id) const;

        ///\param id The id of an edge
        ///\returns The node id of the edge's sink
        NodeId edge_sink_node(const EdgeId id) const { return edge_sink_nodes_[id]; }

        ///\param id The id of an edge
        ///\returns The node id of the edge's source (driver)
        NodeId edge_src_node(const EdgeId id) const { 
            return edge_src_nodes_[id]; 
        }

        ///\param edge The id of an edge
        ///\returns Whether the edge is disabled (i.e. ignored during timing analysis)
        bool edge_disabled(const EdgeId id) const { return edges_disabled_[id]; }

        ///\param src_node the edge's source node
        ///\param sink_node the edge's sink node
        ///\returns The edge betwen these the source and sink nodes, or EdgeId::INVALID() if none exists
        EdgeId find_edge(const tatum::NodeId src_node, const tatum::NodeId sink_node) const;

        /*
         * Level accessors
         */
        ///\param level_id The level index in the graph
        ///\pre The graph must be levelized.
        ///\returns A range containing the nodes in the level
        ///\see levelize()
        node_range level_nodes(const LevelId level_id) const { 
            TATUM_ASSERT_MSG(is_levelized_, "Timing graph must be levelized");
            return tatum::util::make_range(level_nodes_[level_id].begin(),
                                           level_nodes_[level_id].end()); 
        }

        ///\pre The graph must be levelized.
        ///\returns A range containing the nodes which are primary inputs (i.e. SOURCE's with no fanin, corresponding to top level design inputs pins)
        ///\warning Not all SOURCE nodes in the graph are primary inputs (e.g. FF Q pins are SOURCE's but have incomming edges from the clock network)
        ///\see levelize()
        node_range primary_inputs() const { 
            TATUM_ASSERT_MSG(is_levelized_, "Timing graph must be levelized");
            return tatum::util::make_range(primary_inputs_.begin(), primary_inputs_.end()); 
        }

        ///\pre The graph must be levelized.
        ///\returns A range containing the nodes which are logical outputs (i.e. nodes with no fan-out 
        //          corresponding to: top level design output pins and FF D pins)
        ///\warning The logical outputs may be on different levels of the graph
        ///\see levelize()
        node_range logical_outputs() const { 
            TATUM_ASSERT_MSG(is_levelized_, "Timing graph must be levelized");
            return tatum::util::make_range(logical_outputs_.begin(), logical_outputs_.end()); 
        }

        /*
         * Graph aggregate accessors
         */
        //\returns A range containing all nodes in the graph
        node_range nodes() const { return tatum::util::make_range(node_ids_.begin(), node_ids_.end()); }

        //\returns A range containing all edges in the graph
        edge_range edges() const { return tatum::util::make_range(edge_ids_.begin(), edge_ids_.end()); }

        //\returns A range containing all levels in the graph
        level_range levels() const { 
            TATUM_ASSERT_MSG(is_levelized_, "Timing graph must be levelized");
            return tatum::util::make_range(level_ids_.begin(), level_ids_.end()); 
        }

        //\returns A range containing all levels in the graph in *reverse* order
        reverse_level_range reversed_levels() const { 
            TATUM_ASSERT_MSG(is_levelized_, "Timing graph must be levelized");
            return tatum::util::make_range(level_ids_.rbegin(), level_ids_.rend()); 
        }

        //\returns true if the timing graph is internally consistent, throws an exception if not
        bool validate() const;

    public: //Mutators
        /*
         * Graph modifiers
         */
        ///Adds a node to the timing graph
        ///\param type The type of the node to be added
        ///\warning Graph will likely need to be re-levelized after modification
        NodeId add_node(const NodeType type);

        ///Adds an edge to the timing graph
        ///\param type The edge's type
        ///\param src_node The node id of the edge's driving node
        ///\param sink_node The node id of the edge's sink node
        ///\pre The src_node and sink_node must have been already added to the graph
        ///\warning Graph will likely need to be re-levelized after modification
        EdgeId add_edge(const EdgeType type, const NodeId src_node, const NodeId sink_node);

        ///Removes a node (and it's associated edges) from the timing graph
        ///\param node_id The node to remove
        ///\warning This will leave invalid ID references in the timing graph until compress() is called
        ///\see add_node(), compress()
        void remove_node(const NodeId node_id);

        ///Removes an edge from the timing graph
        ///\param edge_id The edge to remove
        ///\warning This will leave invalid ID references in the timing graph until compress() is called
        ///\see add_edge(), compress()
        void remove_edge(const EdgeId edge_id);

        ///Disables an edge in the timing graph (e.g. to break a combinational loop)
        ///\param edge_id The edge to disable
        ///\see identify_combinational_loops()
        void disable_edge(const EdgeId edge_id, bool disable=true);

        ///Compresses the Edge and Node ID spaces to eliminate invalid entries
        ///\returns A structure containing mappings from old to new IDs
        GraphIdMaps  compress();

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
        ///Optimizes the graph's internal memory layout for better performance
        ///\warning Old IDs will be invalidated
        ///\returns The mapping from old to new IDs
        GraphIdMaps optimize_layout();


        ///Sets whether dangling combinational nodes is an error (if true) or not
        void set_allow_dangling_combinational_nodes(bool value) {
            allow_dangling_combinational_nodes_ = value;
        }

    private: //Internal helper functions
        ///\returns A mapping from old to new edge ids which is optimized for performance
        //          (i.e. cache locality)
        tatum::util::linear_map<EdgeId,EdgeId> optimize_edge_layout() const;

        ///\returns A mapping from old to new edge ids which is optimized for performance
        //          (i.e. cache locality)
        tatum::util::linear_map<NodeId,NodeId> optimize_node_layout() const;

        void remap_nodes(const tatum::util::linear_map<NodeId,NodeId>& node_id_map);
        void remap_edges(const tatum::util::linear_map<EdgeId,EdgeId>& edge_id_map);

        void force_levelize();

        bool valid_node_id(const NodeId node_id) const;
        bool valid_edge_id(const EdgeId edge_id) const;
        bool valid_level_id(const LevelId level_id) const;

        bool validate_sizes() const;
        bool validate_values() const;
        bool validate_structure() const;

        size_t count_active_edges(edge_range) const;

    private: //Data
        /*
         * For improved memory locality, we use a Struct of Arrays (SoA)
         * data layout, rather than Array of Structs (AoS)
         */
        //Node data
        tatum::util::linear_map<NodeId,NodeId> node_ids_; //The node IDs in the graph
        tatum::util::linear_map<NodeId,NodeType> node_types_; //Type of node
        tatum::util::linear_map<NodeId,std::vector<EdgeId>> node_in_edges_; //Incomiing edge IDs for node
        tatum::util::linear_map<NodeId,std::vector<EdgeId>> node_out_edges_; //Out going edge IDs for node

        //Edge data
        tatum::util::linear_map<EdgeId,EdgeId> edge_ids_; //The edge IDs in the graph
        tatum::util::linear_map<EdgeId,EdgeType> edge_types_; //Type of edge
        tatum::util::linear_map<EdgeId,NodeId> edge_sink_nodes_; //Sink node for each edge
        tatum::util::linear_map<EdgeId,NodeId> edge_src_nodes_; //Source node for each edge
        tatum::util::linear_map<EdgeId,bool>   edges_disabled_;

        //Auxilary graph-level info, filled in by levelize()
        tatum::util::linear_map<LevelId,LevelId> level_ids_; //The level IDs in the graph
        tatum::util::linear_map<LevelId,std::vector<NodeId>> level_nodes_; //Nodes in each level
        std::vector<NodeId> primary_inputs_; //Primary input nodes of the timing graph.
        std::vector<NodeId> logical_outputs_; //Logical output nodes of the timing graph.
        bool is_levelized_ = false; //Inidcates if the current levelization is valid

        bool allow_dangling_combinational_nodes_ = false;

};

//Returns the set of nodes (Strongly Connected Components) that form loops in the timing graph
std::vector<std::vector<NodeId>> identify_combinational_loops(const TimingGraph& tg);

//Returns the set of nodes transitively connected (either fanin or fanout) to nodes in through_nodes
//up to max_depth (default infinite) hops away
std::vector<NodeId> find_transitively_connected_nodes(const TimingGraph& tg, 
                                                      const std::vector<NodeId> through_nodes, 
                                                      size_t max_depth=std::numeric_limits<size_t>::max());

//Returns the set of nodes in the transitive fanin of nodes in sinks up to max_depth (default infinite) hops away
std::vector<NodeId> find_transitive_fanin_nodes(const TimingGraph& tg, 
                                                const std::vector<NodeId> sinks, 
                                                size_t max_depth=std::numeric_limits<size_t>::max());

//Returns the set of nodes in the transitive fanout of nodes in sources up to max_depth (default infinite) hops away
std::vector<NodeId> find_transitive_fanout_nodes(const TimingGraph& tg,
                                                 const std::vector<NodeId> sources, 
                                                 size_t max_depth=std::numeric_limits<size_t>::max());

EdgeType infer_edge_type(const TimingGraph& tg, EdgeId edge);

//Mappings from old to new IDs
struct GraphIdMaps {
    GraphIdMaps(tatum::util::linear_map<NodeId,NodeId> node_map,
                tatum::util::linear_map<EdgeId,EdgeId> edge_map)
        : node_id_map(node_map), edge_id_map(edge_map) {}
    tatum::util::linear_map<NodeId,NodeId> node_id_map;
    tatum::util::linear_map<EdgeId,EdgeId> edge_id_map;
};



} //namepsace

