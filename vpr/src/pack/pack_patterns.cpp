#include "pack_patterns.h"

#include "atom_netlist.h"
#include "pb_graph_walker.h"
#include "globals.h"

#include <fstream>
#include <stack>
#include <queue>
#include <algorithm>

/*
 * Local Data Types
 */

struct t_arch_pack_pattern_connection {
    t_pb_graph_pin* from_pin;
    t_pb_graph_pin* to_pin;
    std::string pattern_name;
};

//Walks the PB graph collecting any pack patterns found on edges
class PbGraphPackPatternCollector : public PbGraphWalker {
    public:
        void on_edge(t_pb_graph_edge* edge) override {
            if (edge->num_pack_patterns < 1) return;

            for (int ipin = 0; ipin < edge->num_input_pins; ++ipin) {
                for (int opin = 0; opin < edge->num_output_pins; ++opin) {
                    for (int ipattern = 0; ipattern < edge->num_pack_patterns; ++ipattern) {
                        t_arch_pack_pattern_connection conn;
                        conn.from_pin = edge->input_pins[ipin];
                        conn.to_pin = edge->output_pins[opin];
                        conn.pattern_name = edge->pack_pattern_names[ipattern];
                        
                        pack_pattern_connections[conn.pattern_name].push_back(conn);
                    }
                }
            }
        }

        std::map<std::string,std::vector<t_arch_pack_pattern_connection>> pack_pattern_connections;
};

/*
 * Local Function Delcarations
 */
static std::map<std::string,std::vector<t_arch_pack_pattern_connection>> collect_type_pack_pattern_connections(t_type_ptr type);
static t_arch_pack_pattern build_pack_pattern(std::string pattern_name, const std::vector<t_arch_pack_pattern_connection>& connections);
static t_netlist_pack_pattern abstract_arch_pack_pattern(const t_arch_pack_pattern& arch_pack_pattern);


static NetlistPatternMatch match_largest(AtomBlockId blk, const t_netlist_pack_pattern& pattern, const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);
static bool match_largest_recur(NetlistPatternMatch& match, const AtomBlockId blk, 
                                int pattern_node_id, const t_netlist_pack_pattern& pattern,
                                const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist);
static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);
static AtomBlockId find_parent_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static bool matches_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static bool matches_pattern_node(const AtomBlockId blk, const t_netlist_pack_pattern_node& pattern_node, const AtomNetlist& netlist);
static bool is_wildcard_node(const t_netlist_pack_pattern_node& pattern_node);
static bool is_wildcard_pin(const t_netlist_pack_pattern_pin& pattern_pin);
static bool matches_pattern_pin(const AtomPinId from_pin, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist);

/*
 * Global Function Implementations
 */

std::vector<t_arch_pack_pattern> identify_arch_pack_patterns(const DeviceContext& device_ctx) {
    std::vector<t_arch_pack_pattern> arch_pack_patterns;

    //For every complex block
    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        //Collect the pack pattern connections defined in the architecture
        const auto& pack_pattern_connections = collect_type_pack_pattern_connections(&device_ctx.block_types[itype]);

        //vtr::printf("TYPE: %s\n", device_ctx.block_types[itype].name);

        //Convert each set of connections to an architecture pack pattern
        for (auto kv : pack_pattern_connections) {
            t_arch_pack_pattern pack_pattern = build_pack_pattern(kv.first, kv.second);

            arch_pack_patterns.push_back(pack_pattern);
        }
    }

    return arch_pack_patterns;
}

std::vector<t_netlist_pack_pattern> abstract_arch_pack_patterns(const std::vector<t_arch_pack_pattern>& arch_pack_patterns) {
    std::vector<t_netlist_pack_pattern> netlist_pack_patterns;

    for (auto arch_pattern : arch_pack_patterns) {
        auto netlist_pattern = abstract_arch_pack_pattern(arch_pattern);
        netlist_pack_patterns.push_back(netlist_pattern);

        //Debug
        std::ofstream ofs("netlist_pack_pattern." + netlist_pattern.name + ".echo");
        write_netlist_pack_pattern_dot(ofs, netlist_pattern);
    }

    return netlist_pack_patterns;
}

t_netlist_pack_pattern create_atom_default_pack_pattern() {
    t_netlist_pack_pattern atom_pack_pattern;

    //A single internal wild-card node
    atom_pack_pattern.root_node = atom_pack_pattern.create_node(false);

    atom_pack_pattern.name = ATOM_DEFAULT_PACK_PATTERN_NAME;

    return atom_pack_pattern;
}


std::vector<NetlistPatternMatch> collect_pattern_matches_in_netlist(const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    std::vector<NetlistPatternMatch> matches;

    std::set<AtomBlockId> covered_blks;

    for (auto blk : netlist.blocks()) {
        if (covered_blks.count(blk)) continue; //Already in molecule
        if (!matches_pattern_root(blk, pattern, netlist)) continue; //Not a valid root

        //Initially the current block is the only candidate
        std::stack<AtomBlockId> root_candidates;
        root_candidates.push(blk);

        //Collect any root candidates upstream of the current blk
        while (root_candidates.top()) {
            AtomBlockId next_root_candidate = find_parent_pattern_root(root_candidates.top(), pattern, netlist); 

            if (!next_root_candidate) break; //No more potential roots
            if (covered_blks.count(next_root_candidate)) break; //Already used

            root_candidates.push(next_root_candidate);

            VTR_ASSERT_SAFE(matches_pattern_root(next_root_candidate, pattern, netlist));
        }


        //Try each candidate root from the most upstream downward
        // This ensures we get the largest match from the most upstream root connected to the
        // original blk
        NetlistPatternMatch match;
        while (!match && !root_candidates.empty()) {
            match = match_largest(root_candidates.top(), pattern, covered_blks, netlist);
            root_candidates.pop();
        }

        if (match) {
            //Save the match
            matches.push_back(match);

            //Record the blocks covered so blocks don't end up in multiple matches
            covered_blks.insert(match.internal_blocks.begin(), match.internal_blocks.end());
        }
    }

    return matches;
}

std::vector<NetlistPatternMatch> filter_netlist_pattern_matches(std::vector<NetlistPatternMatch> unfiltered_matches) {
    std::vector<NetlistPatternMatch> filtered_matches;

    //Sort the matches by descending size
    auto by_match_size = [](const NetlistPatternMatch& lhs , NetlistPatternMatch& rhs) {
        return lhs.internal_blocks.size() > rhs.internal_blocks.size();
    };
    std::sort(unfiltered_matches.begin(), unfiltered_matches.end(), by_match_size);

    //Walk through the matches in descending order of size,
    //recording those with no overlaps
    std::set<AtomBlockId> covered_blocks;
    for (auto& netlist_match : unfiltered_matches) {

        const auto& internal_blocks = netlist_match.internal_blocks;

        auto not_covered = [&](const AtomBlockId blk) {
            return !covered_blocks.count(blk);
        };

        if (std::all_of(internal_blocks.begin(), internal_blocks.end(), not_covered)) {
            //Add
            filtered_matches.push_back(netlist_match);
            
            //Record the covered blocks so any smaller patterns which include these
            //blocks are rejected
            covered_blocks.insert(internal_blocks.begin(), internal_blocks.end());
        }
    }

    return filtered_matches;
}

void print_match(const NetlistPatternMatch& match, const AtomNetlist& netlist) {
    vtr::printf("Netlist Match to %s:\n", match.pattern_name.c_str());
    for (auto& edge : match.netlist_edges) {
        AtomBlockId from_blk = netlist.pin_block(edge.from_pin);
        AtomBlockId to_blk = netlist.pin_block(edge.to_pin);

        bool from_external = (std::find(match.external_blocks.begin(), match.external_blocks.end(), from_blk) != match.external_blocks.end());
        bool to_external = (std::find(match.external_blocks.begin(), match.external_blocks.end(), to_blk) != match.external_blocks.end());

        vtr::printf("  %s -> %s (%s%s -> %s%s) (pattern edge #%d.%d)\n", 
                netlist.pin_name(edge.from_pin).c_str(),
                netlist.pin_name(edge.to_pin).c_str(),
                netlist.block_model(netlist.pin_block(edge.from_pin))->name,
                (from_external) ? "*" : "",
                netlist.block_model(netlist.pin_block(edge.to_pin))->name,
                (to_external) ? "*" : "",
                edge.pattern_edge_id,
                edge.pattern_edge_sink);
    }

    for (auto blk : match.internal_blocks) {
        vtr::printf("\tInternal match block: %s (%zu)\n", netlist.block_name(blk).c_str(), size_t(blk));
    }

    for (auto blk : match.external_blocks) {
        vtr::printf("\tExternal match block: %s (%zu)\n", netlist.block_name(blk).c_str(), size_t(blk));
    }
}

void write_arch_pack_pattern_dot(std::ostream& os, const t_arch_pack_pattern& arch_pattern) {
    os << "#Dot file of architecture pack pattern graph\n";
    os << "digraph " << arch_pattern.name << " {\n";

    for (size_t inode = 0; inode < arch_pattern.nodes.size(); ++inode) {
        os << "N" << inode;
        
        os << " [";

        os << "label=\"" << describe_pb_graph_pin_hierarchy(arch_pattern.nodes[inode].pb_graph_pin) << " (#" << inode << ")\"";

        os << "];\n";
    }

    for (size_t iedge = 0; iedge < arch_pattern.edges.size(); ++iedge) {
        os << "N" << arch_pattern.edges[iedge].from_node_id << " -> N" << arch_pattern.edges[iedge].to_node_id;
        os << " [";

        os << "label=\"" ;
        os << "(#" << iedge << ")";
        os << "\"";

        os << "];\n";
    }

    os << "}\n";
}

void write_netlist_pack_pattern_dot(std::ostream& os, const t_netlist_pack_pattern& netlist_pattern) {
    os << "#Dot file of netlist pack pattern graph\n";
    os << "digraph " << netlist_pattern.name << " {\n";

    //Nodes
    for (size_t inode = 0; inode < netlist_pattern.nodes.size(); ++inode) {
        os << "N" << inode;
        
        os << " [";

        os << "label=\"";
        if (netlist_pattern.nodes[inode].model_type) {
            os << netlist_pattern.nodes[inode].model_type->name;
        } else {
            os << "*";
        }
        os << " (#" << inode << ")";
        os << "\"";

        if (netlist_pattern.nodes[inode].is_external()) {
            os << " shape=invhouse";
        }

        os << "];\n";
    }

    //Edges
    for (size_t iedge = 0; iedge < netlist_pattern.edges.size(); ++iedge) {
        const auto& edge = netlist_pattern.edges[iedge];

        const auto& from_pin = edge.from_pin;
        for (size_t isink = 0; isink < edge.to_pins.size(); ++isink) {
            const auto& to_pin = edge.to_pins[isink];

            os << "N" << edge.from_pin.node_id << " -> N" << edge.to_pins[isink].node_id;
            os << " [";

            os << "label=\"" ;

            os << "(#" << iedge << "." << isink << ")\\n";
            if (is_wildcard_pin(from_pin)) {
                os << "*";
            } else {
                os << from_pin.model_port->name << "[" << from_pin.port_pin << "]";
            }
            os << "\\n -> ";
            if (is_wildcard_pin(to_pin)) {
                os << "*";
            } else {
                os << to_pin.model_port->name << "[" << to_pin.port_pin << "]";
            }
            os << "\"";

            if (to_pin.required) {
                os << " style=solid";
            } else {
                os << " style=dashed";
            }

            os << "];\n";
        }
    }

    os << "}\n";
}
/*
 * Local Function Implementations
 */
static std::map<std::string,std::vector<t_arch_pack_pattern_connection>> collect_type_pack_pattern_connections(t_type_ptr type) {
    PbGraphPackPatternCollector collector;

    collector.walk(type->pb_graph_head);

    return collector.pack_pattern_connections;
}

static t_arch_pack_pattern build_pack_pattern(std::string pattern_name, const std::vector<t_arch_pack_pattern_connection>& connections) {
    t_arch_pack_pattern pack_pattern;
    pack_pattern.name = pattern_name;

    for (auto conn : connections) {
        vtr::printf("pack_pattern '%s' conn: %s -> %s\n", 
                    conn.pattern_name.c_str(),
                    describe_pb_graph_pin_hierarchy(conn.from_pin).c_str(),
                    describe_pb_graph_pin_hierarchy(conn.to_pin).c_str());
    }

    //Create the nodes
    std::map<t_pb_graph_pin*, int> pb_graph_pin_to_pattern_node_id;
    for (auto conn : connections) {
        VTR_ASSERT(conn.pattern_name == pattern_name);

        t_pb_graph_pin* from_pin = conn.from_pin;
        t_pb_graph_pin* to_pin = conn.to_pin;

        if (from_pin) {
            if (!pb_graph_pin_to_pattern_node_id.count(from_pin)) {
                //Create
                int pattern_node_id = pack_pattern.nodes.size();
                pack_pattern.nodes.emplace_back(from_pin);

                //Save ID
                pb_graph_pin_to_pattern_node_id[from_pin] = pattern_node_id;
            }
        }

        if (to_pin) {
            if (!pb_graph_pin_to_pattern_node_id.count(to_pin)) {
                //Create
                int pattern_node_id = pack_pattern.nodes.size();
                pack_pattern.nodes.emplace_back(to_pin);

                //Save ID
                pb_graph_pin_to_pattern_node_id[to_pin] = pattern_node_id;
            }
        }
    }

    //Create the edges
    std::map<std::pair<t_pb_graph_pin*,t_pb_graph_pin*>, int> conn_to_pattern_edge;
    for (auto conn : connections) {
        auto key = std::make_pair(conn.from_pin, conn.to_pin);
        if (!conn_to_pattern_edge.count(key)) {
            //Find connected nodes
            VTR_ASSERT(pb_graph_pin_to_pattern_node_id.count(conn.from_pin));
            VTR_ASSERT(pb_graph_pin_to_pattern_node_id.count(conn.to_pin));
            int from_node_id = pb_graph_pin_to_pattern_node_id[conn.from_pin];
            int to_node_id = pb_graph_pin_to_pattern_node_id[conn.to_pin];

            //Create edge
            int edge_id = pack_pattern.edges.size();
            pack_pattern.edges.emplace_back(); 

            pack_pattern.edges[edge_id].from_node_id = from_node_id;
            pack_pattern.edges[edge_id].to_node_id = to_node_id;

            //Save ID
            conn_to_pattern_edge[key] = edge_id;


            //Add edge references to connected nodes
            pack_pattern.nodes[from_node_id].out_edge_ids.push_back(edge_id);
            pack_pattern.nodes[to_node_id].in_edge_ids.push_back(edge_id);
        }
    }

    //Record roots
    for (size_t inode = 0; inode < pack_pattern.nodes.size(); ++inode) {
        if (pack_pattern.nodes[inode].in_edge_ids.empty()) {
            pack_pattern.root_node_ids.push_back(inode);
        }
    }

    //Calculate the base cost of the pattern based on the nodes involved
    std::set<t_pb_graph_node*> seen_pb_graph_nodes;
    for (size_t inode = 0; inode < pack_pattern.nodes.size(); ++inode) {
        t_pb_graph_node* pb_graph_node = pack_pattern.nodes[inode].pb_graph_pin->parent_node;

        if (seen_pb_graph_nodes.count(pb_graph_node)) continue;

        pack_pattern.base_cost += compute_primitive_base_cost(pb_graph_node);
    }

    return pack_pattern;
}


static t_netlist_pack_pattern abstract_arch_pack_pattern(const t_arch_pack_pattern& arch_pattern) {
    t_netlist_pack_pattern netlist_pattern;
    netlist_pattern.name = arch_pattern.name;
    netlist_pattern.base_cost = arch_pattern.base_cost;

    struct EdgeInfo {
        EdgeInfo(int from_node, int from_edge, int via_edge, int curr_node)
            : from_arch_node(from_node)
            , from_arch_edge(from_edge)
            , via_arch_edge(via_edge)
            , curr_arch_node(curr_node) {}

        int from_arch_node = OPEN; //Last primitive node we came from
        int from_arch_edge = OPEN; //Last primitive edge we came from
        int via_arch_edge = OPEN; //Edge to current node
        int curr_arch_node = OPEN; //Current node

    };


    //Breadth-First Traversal of arch pack pattern graph, 
    //to record which primitives are reachable from each other.
    //
    //The resulting edges are abstracted from the 
    //pb_graph hierarchy and contain only primitives (no intermediate
    //hiearchy)
    //
    //Note that to get the full set of possible edges we perform a BFT
    //from each primitive/top-level node
    std::vector<EdgeInfo> arch_abstract_edges;
    for (int root_node : arch_pattern.root_node_ids) {
        std::set<int> visited;
        std::queue<EdgeInfo> q;

        vtr::printf("BFT from root %d\n", root_node);

        q.emplace(OPEN, OPEN, OPEN, root_node); //Starting at root

        while (!q.empty()) {
            EdgeInfo info = q.front();
            q.pop();

            int curr_node = info.curr_arch_node;

            if (visited.count(curr_node)) continue; //Don't revisit to avoid loops
            visited.insert(curr_node);
            
            bool is_primitive = arch_pattern.nodes[curr_node].pb_graph_pin->parent_node->is_primitive();
            bool is_top = arch_pattern.nodes[curr_node].pb_graph_pin->parent_node->is_top_level();
            vtr::printf(" Visiting %d via %d (is_prim: %d)\n", curr_node, info.via_arch_edge, is_primitive);

            if ((is_primitive || is_top) && info.from_arch_edge != OPEN && info.via_arch_edge != OPEN) {
                //Record the primitive-to-primitive edge used to reach this node
                arch_abstract_edges.push_back(info);
                vtr::printf("  Recording Abstract Edge from node %d (via edge %d) -> node %d (via edge %d)\n", info.from_arch_node, info.from_arch_edge, curr_node, info.via_arch_edge);
            }

            //Expand out edges
            for (auto out_edge : arch_pattern.nodes[curr_node].out_edge_ids) {

                int next_node = arch_pattern.edges[out_edge].to_node_id;

                if (is_primitive || curr_node == root_node) {
                    //Expanding from a primitive, use the current node as from
                    q.emplace(curr_node, out_edge, out_edge, next_node);
                } else {
                    //Expanding from a non-primitive, re-use the previous primtiive as from
                    q.emplace(info.from_arch_node, info.from_arch_edge, out_edge, next_node);
                }
            }
        }
    }


    //
    //Build the netlist pattern from the edges collected above
    //
    std::map<t_pb_graph_pin*,int> pb_graph_pin_to_netlist_pattern_node;
    std::map<t_pb_graph_node*,int> pb_graph_node_to_netlist_pattern_node;

    std::map<t_pb_graph_pin*,int> pb_graph_pin_to_netlist_pattern_edge;

    for (auto arch_abstract_edge : arch_abstract_edges) {

        //Find or create from node
        int netlist_from_node = OPEN;
        t_pb_graph_pin* from_pin = arch_pattern.nodes[arch_abstract_edge.from_arch_node].pb_graph_pin;
        if (pb_graph_pin_to_netlist_pattern_node.count(from_pin)) {
            //Existing
            netlist_from_node = pb_graph_pin_to_netlist_pattern_node[from_pin];
        } else if (pb_graph_node_to_netlist_pattern_node.count(from_pin->parent_node)) {
            //Existing
            netlist_from_node = pb_graph_node_to_netlist_pattern_node[from_pin->parent_node];
        } else if (from_pin->parent_node->is_top_level()) {
            //Create
            netlist_from_node = netlist_pattern.create_node(true);

            //Default initialization

            //Save Id
            pb_graph_pin_to_netlist_pattern_node[from_pin] = netlist_from_node;
        } else {
            //Create
            netlist_from_node = netlist_pattern.create_node(false);

            //Initialize
            netlist_pattern.nodes[netlist_from_node].model_type = from_pin->parent_node->pb_type->model;

            //Save Id
            pb_graph_node_to_netlist_pattern_node[from_pin->parent_node] = netlist_from_node;
        }
        VTR_ASSERT(netlist_from_node != OPEN);

        //Find or create to node
        int netlist_to_node = OPEN;
        t_pb_graph_pin* to_pin = arch_pattern.nodes[arch_abstract_edge.curr_arch_node].pb_graph_pin;
        if (pb_graph_pin_to_netlist_pattern_node.count(to_pin)) {
            //Existing
            netlist_to_node = pb_graph_pin_to_netlist_pattern_node[to_pin];
        } else if (pb_graph_node_to_netlist_pattern_node.count(to_pin->parent_node)) {
            //Existing
            netlist_to_node = pb_graph_node_to_netlist_pattern_node[to_pin->parent_node];
        } else if (to_pin->parent_node->is_top_level()) {
            //Create
            netlist_to_node = netlist_pattern.create_node(true);

            //Default initialization

            //Save Id
            pb_graph_pin_to_netlist_pattern_node[to_pin] = netlist_to_node;
        } else {
            //Create
            netlist_to_node = netlist_pattern.create_node(false);

            //Initialize
            netlist_pattern.nodes[netlist_to_node].model_type = to_pin->parent_node->pb_type->model;

            //Save Id
            pb_graph_node_to_netlist_pattern_node[to_pin->parent_node] = netlist_to_node;
        }
        VTR_ASSERT(netlist_to_node != OPEN);

        //Create edge

        int netlist_edge = OPEN;
        if (pb_graph_pin_to_netlist_pattern_edge.count(from_pin)) {
            //Existing
            netlist_edge = pb_graph_pin_to_netlist_pattern_edge[from_pin];
        } else {
            //create
            netlist_edge = netlist_pattern.create_edge();

            //Initialize
            netlist_pattern.edges[netlist_edge].from_pin.node_id = netlist_from_node;
            if (!pb_graph_pin_to_netlist_pattern_node.count(from_pin)) {
                //Only initialze these fields for non source/sink
                netlist_pattern.edges[netlist_edge].from_pin.model = from_pin->parent_node->pb_type->model;
                netlist_pattern.edges[netlist_edge].from_pin.model_port = from_pin->port->model_port;
                netlist_pattern.edges[netlist_edge].from_pin.port_pin = from_pin->pin_number;
            }

            //Update node ref
            netlist_pattern.nodes[netlist_from_node].out_edge_ids.push_back(netlist_edge);

            //Save Id
            pb_graph_pin_to_netlist_pattern_edge[from_pin] = netlist_edge;
        }

        //Add sink
        t_netlist_pack_pattern_pin sink_pin;
        sink_pin.node_id = netlist_to_node;
        if (!pb_graph_pin_to_netlist_pattern_node.count(to_pin)) {
            sink_pin.model = to_pin->parent_node->pb_type->model;
            sink_pin.model_port = to_pin->port->model_port;
            sink_pin.port_pin = to_pin->pin_number;
        }

        netlist_pattern.edges[netlist_edge].to_pins.push_back(sink_pin);

        //Update node refs
        netlist_pattern.nodes[netlist_to_node].in_edge_ids.push_back(netlist_edge);
    }

    //Record the roots
    std::vector<int> roots;
    for (size_t inode = 0; inode < netlist_pattern.nodes.size(); ++inode) {
        if (netlist_pattern.nodes[inode].is_root()) {
            roots.push_back(inode);
        }
    }

    //Should have a single root
    if (roots.size() == 1) {
        VTR_ASSERT(netlist_pattern.root_node == OPEN);
        netlist_pattern.root_node = roots[0];
    } else {
        std::string msg = vtr::string_fmt("Pack pattern '%s' has multiple roots (pack pattern "
                                          "connection(s) are likely missing causing the pattern "
                                          "to be disconnected)\n",
                                          netlist_pattern.name.c_str());

        msg += vtr::string_fmt("  Involved nodes:\n");
        for (auto root : roots) {
            if (netlist_pattern.nodes[root].model_type) {
                msg += vtr::string_fmt("%s\n", netlist_pattern.nodes[root].model_type->name);
            }
        }

        VPR_THROW(VPR_ERROR_ARCH, msg.c_str());
    }
    VTR_ASSERT(netlist_pattern.root_node != OPEN);

    return netlist_pattern;
}

static NetlistPatternMatch match_largest(AtomBlockId blk, const t_netlist_pack_pattern& pattern, const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {
    NetlistPatternMatch match;
    if (match_largest_recur(match, blk, pattern.root_node, pattern, excluded_blocks, netlist)) {
        match.pattern_name = pattern.name;
        match.base_cost = pattern.base_cost;
        return match; 
    }
    return NetlistPatternMatch(); //No match, return empty
}

static bool match_largest_recur(NetlistPatternMatch& match, const AtomBlockId blk, 
                                int pattern_node_id, const t_netlist_pack_pattern& pattern,
                                const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {

    if (excluded_blocks.count(blk) && pattern.nodes[pattern_node_id].is_internal()) {
        //Block is already part of another match, and is matching an internal node of the pattern
        return false;
    }

    if (!matches_pattern_node(blk, pattern.nodes[pattern_node_id], netlist)) {
        return false; 
    }

    if (pattern.nodes[pattern_node_id].is_internal()) {
        match.internal_blocks.push_back(blk);
    } else {
        VTR_ASSERT(pattern.nodes[pattern_node_id].is_external());
        match.external_blocks.push_back(blk);
    }

    for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) {
        const auto& pattern_edge = pattern.edges[pattern_edge_id];
        const auto& from_pattern_pin = pattern_edge.from_pin;

        AtomPinId from_pin = find_matching_pin(netlist.block_output_pins(blk), from_pattern_pin, excluded_blocks, netlist);
        if (!from_pin) {
            //No matching driver pin
            if (from_pattern_pin.required) {
                //Required: give-up
                return false;
            } else {
                //Optional: try next edge
                continue;
            }
        }

        AtomNetId net = netlist.pin_net(from_pin);
        auto net_sinks = netlist.net_sinks(net);

        for (size_t isink = 0; isink < pattern_edge.to_pins.size(); ++isink) {
            const auto& to_pattern_pin = pattern_edge.to_pins[isink];
            const auto& to_pattern_node = pattern.nodes[to_pattern_pin.node_id];

            AtomPinId to_pin = find_matching_pin(net_sinks, to_pattern_pin, excluded_blocks, netlist);
            if (!to_pin || (excluded_blocks.count(netlist.pin_block(to_pin)) && to_pattern_node.is_internal())) {
                //No valid to_pin (either doesn't match pattern, or is on an excluded block)

                if (to_pattern_pin.required) {
                    //Required: give-up
                    return false;
                } else {
                    //Optional: try next pattern pin
                    continue;
                }
            }

            //Valid match between netlist from_pin/to_pin and from_pattern_pin/to_pattern_pin
            //Add it to the match
            match.netlist_edges.emplace_back(from_pin, to_pin, pattern_edge_id, isink);

            //Collect any downstream matches recursively
            AtomBlockId to_blk = netlist.pin_block(to_pin);
            bool subtree_matched = match_largest_recur(match, to_blk, to_pattern_pin.node_id, pattern, excluded_blocks, netlist);

            if (!subtree_matched) {
                return false;
            }
        }
    }

    return true;
}

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const AtomNetlist& netlist) {
    return find_matching_pin(pin_range, pattern_pin, {}, netlist);
}

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {

    for (AtomPinId pin : pin_range) {
        if (matches_pattern_pin(pin, pattern_pin, netlist)) {

            if (excluded_blocks.count(netlist.pin_block(pin))) {
                continue;
            } else {
                return pin;
            }
        }
    }

    return AtomPinId::INVALID(); //No match
}

//Returns a parent block of blk, if it is also a valid root for pattern
static AtomBlockId find_parent_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    int pattern_node_id = pattern.root_node;

    VTR_ASSERT_SAFE(matches_pattern_root(blk, pattern, netlist));

    //Find an upstream block which is also a valid root
    for (auto pins : {netlist.block_input_pins(blk), netlist.block_clock_pins(blk)}) { //Current blocks inputs
        for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) { //Root out edges
            for (const auto& pattern_pin : pattern.edges[pattern_edge_id].to_pins) { //Edge pins

                //Do the inputs of the current block match the output edges of the root pattern?
                AtomPinId to_pin = find_matching_pin(pins, pattern_pin, netlist);
                if (!to_pin) continue;

                AtomNetId in_net = netlist.pin_net(to_pin);
                AtomBlockId from_blk = netlist.net_driver_block(in_net);

                if (!matches_pattern_root(from_blk, pattern, netlist)) continue;

                return from_blk;
            }
        }
    }

    return AtomBlockId::INVALID();
}

//Returns true if matches the pattern root node (and it's out-going edges)
static bool matches_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    int pattern_node_id = pattern.root_node;

    if (!matches_pattern_node(blk, pattern.nodes[pattern_node_id], netlist)) {
        return false; 
    }

    for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) {
        const auto& pattern_edge = pattern.edges[pattern_edge_id];

        AtomPinId from_pin = find_matching_pin(netlist.block_output_pins(blk), pattern_edge.from_pin, netlist);
        if (!from_pin) {
            //No matching driver pin
            return false;
        }

        AtomNetId net = netlist.pin_net(from_pin);
        auto net_sinks = netlist.net_sinks(net);

        for (size_t isink = 0; isink < pattern_edge.to_pins.size(); ++isink) {
            const auto& to_pattern_pin = pattern_edge.to_pins[isink];

            AtomPinId to_pin = find_matching_pin(net_sinks, to_pattern_pin, netlist);
            if (!to_pin) {

                if (to_pattern_pin.required) {
                    //Required: give-up
                    return false;
                } else {
                    //Optional: try next pattern pin
                    continue;
                }
            }
        }
    }
    return true;
}

static bool matches_pattern_node(const AtomBlockId blk, const t_netlist_pack_pattern_node& pattern_node, const AtomNetlist& netlist) {
    if (is_wildcard_node(pattern_node)) {
        return true; //Wildcard
    }
    return netlist.block_model(blk) == pattern_node.model_type;
}

static bool is_wildcard_node(const t_netlist_pack_pattern_node& pattern_node) {
    return pattern_node.model_type == nullptr;
}

static bool is_wildcard_pin(const t_netlist_pack_pattern_pin& pattern_pin) {
    return (pattern_pin.model == nullptr
            && pattern_pin.model_port == nullptr
            && pattern_pin.port_pin == OPEN);
}

static bool matches_pattern_pin(const AtomPinId from_pin, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist) {

    if (is_wildcard_pin(pattern_pin)) {
        return true; //Wildcard
    }

    if (netlist.block_model(netlist.pin_block(from_pin)) != pattern_pin.model
        || netlist.port_model(netlist.pin_port(from_pin)) != pattern_pin.model_port
        || netlist.pin_port_bit(from_pin) != (BitIndex) pattern_pin.port_pin) {
        return false;
    }

    return true;
}

