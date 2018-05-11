#ifndef VPR_PACK_PATTERNS_H
#define VPR_PACK_PATTERNS_H
#include "vpr_context.h"

//An edge in a pack pattern graph
struct t_arch_pack_pattern_edge {
    int from_node_id = OPEN; //From node's id in t_arch_pack_pattern::nodes
    int to_node_id = OPEN; //To node's id in t_arch_pack_pattern::nodes
};

//A node in a pack pattern graph
struct t_arch_pack_pattern_node {
    t_arch_pack_pattern_node(t_pb_graph_pin* pin)
        : pb_graph_pin(pin) {}

    t_pb_graph_pin* pb_graph_pin = nullptr; //Associated PB graph pin

    std::vector<int> in_edge_ids; //Incoming edge ids in t_arch_pack_pattern::edges
    std::vector<int> out_edge_ids; //Outgoing edge ids in t_arch_pack_pattern::edges
};

//A pack pattern represented as a graph
//
//This is usually a strict tree, unless is_chain is true
struct t_arch_pack_pattern {
    //The name of the pack pattern
    std::string name;

    //The root node id of the pattern
    std::vector<int> root_node_ids;

    //If true represents a chain-like structure which 
    //stretches accross multiple logic blocks
    //
    //Note that if this is a chain, then the graph is not
    //strictly a tree, but the root (top-level) will have both
    //in-coming and out-going edges forming a loop.
    bool is_chain = false; 

    //The set of pins in the pack pattern graph
    std::vector<t_arch_pack_pattern_node> nodes;

    //The set of edges in the pack pattern graph
    std::vector<t_arch_pack_pattern_edge> edges;
};

struct t_netlist_pack_pattern_pin {
    int node_id = OPEN; //Associated node in netlist pack pattern graph

    t_model* model = nullptr;
    t_model_ports* model_port = nullptr;
    int port_pin = OPEN; //Pin index within model_port

    bool required = false;
};

struct t_netlist_pack_pattern_edge {
    t_netlist_pack_pattern_pin from_pin;
    std::vector<t_netlist_pack_pattern_pin> to_pins;
};

struct t_netlist_pack_pattern_node {
    t_netlist_pack_pattern_node(bool external)
        : is_external_(external) {}

    t_model* model_type = nullptr;

    std::vector<int> in_edge_ids;
    std::vector<int> out_edge_ids;

    bool is_leaf() const { return out_edge_ids.size() == 0; }
    bool is_root() const { return in_edge_ids.size() == 0; }

    bool is_external() const { return is_external_; }
    bool is_internal() const { return !is_external(); }

    private:
        bool is_external_; //Indicates node is external (i.e. outside a cluster)
};

struct t_netlist_pack_pattern {
    std::string name;

    int root_node = OPEN;

    std::vector<t_netlist_pack_pattern_node> nodes;
    std::vector<t_netlist_pack_pattern_edge> edges;

    int create_node(bool external) {
        nodes.emplace_back(external);
        return nodes.size() - 1;

    }

    int create_edge() {
        edges.emplace_back();
        return edges.size() - 1;
    }
};

struct NetlistPatternMatch {
    struct Edge {
        Edge(AtomPinId from, AtomPinId to, int edge_id, int sink_idx)
            : from_pin(from)
            , to_pin(to)
            , pattern_edge_id(edge_id)
            , pattern_edge_sink(sink_idx) {}

        AtomPinId from_pin;
        AtomPinId to_pin;
        int pattern_edge_id = OPEN; //Pattern edge which matched these pins
        int pattern_edge_sink = OPEN; //Matching sink number within pattern edge
    };

    //Evaluates true when the match is non-empty
    operator bool() {
        return netlist_edges.size() > 0;
    }

    std::vector<Edge> netlist_edges;
};


std::vector<t_arch_pack_pattern> init_arch_pack_patterns(const DeviceContext& device_ctx);
std::vector<t_netlist_pack_pattern> abstract_arch_pack_patterns(const std::vector<t_arch_pack_pattern>& arch_pack_patterns);

std::vector<NetlistPatternMatch> collect_pattern_matches_in_netlist(const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
std::set<AtomBlockId> collect_internal_blocks_in_match(const NetlistPatternMatch& match, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);

#endif
