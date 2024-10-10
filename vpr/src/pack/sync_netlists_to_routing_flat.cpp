/**
* @file sync_netlists_to_routing_flat.cpp
*
* @brief Implementation for \see sync_netlists_to_routing_flat().
*/

#include "clustered_netlist_fwd.h"
#include "clustered_netlist_utils.h"
#include "logic_types.h"
#include "netlist_fwd.h"
#include "physical_types.h"
#include "vtr_time.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "annotate_routing.h"
#include "globals.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "rr_graph2.h"

#include "sync_netlists_to_routing_flat.h"

/* Static function decls (file-scope) */

/** Get intra-cluster connections from a given RouteTree. Output <source, sink> pairs to \p out_connections . */
static void get_intra_cluster_connections(const RouteTree& tree, std::vector<std::pair<RRNodeId, RRNodeId>>& out_connections);

/** Rudimentary intra-cluster router between two pb_graph pins.
 * This is needed because the flat router compresses the RRG reducing singular paths into nodes.
 * We need to unpack it to get valid packing results, which is the purpose of this simple BFS router.
 * Outputs the path to the pb_routes field of \p out_pb . */
static void route_intra_cluster_conn(const t_pb_graph_pin* source_pin, const t_pb_graph_pin* sink_pin, AtomNetId net_id, t_pb* out_pb);

/** Rebuild the pb.pb_routes struct for each cluster block from flat routing results.
 * The pb_routes struct holds all intra-cluster routing. */
static void sync_pb_routes_to_routing(void);

/** Rebuild ClusteredNetlist from flat routing results, since some nets can move in/out of a block after routing. */
static void sync_clustered_netlist_to_routing(void);

/** Rebuild atom_lookup.atom_pin_pb_graph_pin and pb.atom_pin_bit_index from flat routing results.
 * These contain mappings between the AtomNetlist and the physical pins, which are invalidated after flat routing due to changed pin rotations.
 * (i.e. the primitive has equivalent input pins and flat routing used a different pin) */
static void fixup_atom_pb_graph_pin_mapping(void);


/* Function definitions */

/** Is the clock net found in the routing results?
 * (If not, clock_modeling is probably ideal and we should preserve clock routing while rebuilding.) */
inline bool is_clock_net_routed(void){
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& route_ctx = g_vpr_ctx.routing();

    for(auto net_id: atom_ctx.nlist.nets()){
        auto& tree = route_ctx.route_trees[net_id];
        if(!tree)
            continue;
        if(route_ctx.is_clock_net[net_id]) /* Clock net has routing */
            return true;
    }

    return false;
}

/** Get the ClusterBlockId for a given RRNodeId. */
inline ClusterBlockId get_cluster_block_from_rr_node(RRNodeId inode){
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& rr_graph = device_ctx.rr_graph;

    auto physical_tile = device_ctx.grid.get_physical_type({
        rr_graph.node_xlow(inode),
        rr_graph.node_ylow(inode),
        rr_graph.node_layer(inode)
    });

    int source_pin = rr_graph.node_pin_num(inode);

    auto [_, subtile] = get_sub_tile_from_pin_physical_num(physical_tile, source_pin);

    ClusterBlockId clb = place_ctx.grid_blocks().block_at_location({
        rr_graph.node_xlow(inode),
        rr_graph.node_ylow(inode),
        subtile,
        rr_graph.node_layer(inode)
    });

    return clb;
}

static void get_intra_cluster_connections(const RouteTree& tree, std::vector<std::pair<RRNodeId, RRNodeId>>& out_connections){
    auto& rr_graph = g_vpr_ctx.device().rr_graph;

    for(auto& node: tree.all_nodes()){
        const auto& parent = node.parent();
        if(!parent) /* Root */
            continue;

        /* Find the case where both nodes are IPIN/OPINs and on the same block */
        auto type = rr_graph.node_type(node.inode);
        auto parent_type = rr_graph.node_type(parent->inode);

        if((type == IPIN || type == OPIN) && (parent_type == IPIN || parent_type == OPIN)){
            auto clb = get_cluster_block_from_rr_node(node.inode);
            auto parent_clb = get_cluster_block_from_rr_node(parent->inode);
            if(clb == parent_clb)
                out_connections.push_back({parent->inode, node.inode});
        }
    }
}

static void route_intra_cluster_conn(const t_pb_graph_pin* source_pin, const t_pb_graph_pin* sink_pin, AtomNetId net_id, t_pb* out_pb){
    std::unordered_set<const t_pb_graph_pin*> visited;
    std::deque<const t_pb_graph_pin*> queue;
    std::unordered_map<const t_pb_graph_pin*, const t_pb_graph_pin*> prev;

    auto& out_pb_routes = out_pb->pb_route;

    queue.push_back(source_pin);
    prev[source_pin] = NULL;

    while(!queue.empty()){
        const t_pb_graph_pin* cur_pin = queue.front();
        queue.pop_front();
        if(visited.count(cur_pin))
            continue;
        visited.insert(cur_pin);

        /* Backtrack and return */
        if(cur_pin == sink_pin){
            break;
        }

        for(auto& edge: cur_pin->output_edges){
            VTR_ASSERT(edge->num_output_pins == 1);
            queue.push_back(edge->output_pins[0]);
            prev[edge->output_pins[0]] = cur_pin;
        }
    }
 
    VTR_ASSERT_MSG(visited.count(sink_pin), "Couldn't find sink pin");

    /* Collect path: we need to build pb_routes from source to sink */
    std::vector<const t_pb_graph_pin*> path;
    const t_pb_graph_pin* cur_pin = sink_pin;
    while(cur_pin != source_pin){
        path.push_back(cur_pin);
        cur_pin = prev[cur_pin];
    }
    path.push_back(source_pin);

    /* Output the path into out_pb, starting from source. This is where the pb_route is updated */
    int prev_pin_id = -1;
    for(auto it = path.rbegin(); it != path.rend(); ++it){
        cur_pin = *it;
        int cur_pin_id = cur_pin->pin_count_in_cluster;
        t_pb_route* cur_pb_route;

        if(out_pb_routes.count(cur_pin_id))
            cur_pb_route = &out_pb_routes[cur_pin_id];
        else {
            t_pb_route pb_route = {
                net_id,
                -1,
                {},
                cur_pin
            };
            out_pb_routes.insert(std::make_pair<>(cur_pin_id, pb_route));
            cur_pb_route = &out_pb_routes[cur_pin_id];
        }

        if(prev_pin_id != -1){
            t_pb_route& prev_pb_route = out_pb_routes[prev_pin_id];
            prev_pb_route.sink_pb_pin_ids.push_back(cur_pin_id);
            cur_pb_route->driver_pb_pin_id = prev_pb_route.pb_graph_pin->pin_count_in_cluster;
        }

        prev_pin_id = cur_pin_id;
    }
}

static void sync_pb_routes_to_routing(void){
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& route_ctx = g_vpr_ctx.routing();
    auto& rr_graph = device_ctx.rr_graph;

    /* Was the clock net routed? */
    bool clock_net_is_routed = is_clock_net_routed();

    /* Clear out existing pb_routes: they were made by the intra cluster router and are invalid now */
    for (ClusterBlockId clb_blk_id : cluster_ctx.clb_nlist.blocks()) {
        /* If we don't have routing for the clock net, don't erase entries associated with a clock net.
         * Otherwise we won't have data to rebuild them */
        std::vector<int> pins_to_erase;
        auto& pb_routes = cluster_ctx.clb_nlist.block_pb(clb_blk_id)->pb_route;
        for(auto& [pin, pb_route]: pb_routes){
            if(clock_net_is_routed || !route_ctx.is_clock_net[pb_route.atom_net_id])
                pins_to_erase.push_back(pin);
        }

        for(int pin: pins_to_erase){
            pb_routes.erase(pin);
        }
    }

    /* Go through each route tree and rebuild the pb_routes */
    for(ParentNetId net_id: atom_ctx.nlist.nets()){
        auto& tree = route_ctx.route_trees[net_id];
        if(!tree)
            continue; /* No routing at this ParentNetId */

        /* Get all intrablock connections */
        std::vector<std::pair<RRNodeId, RRNodeId>> conns_to_restore; /* (source, sink) */
        get_intra_cluster_connections(tree.value(), conns_to_restore);

        /* Restore the connections */
        for(auto [source_inode, sink_inode]: conns_to_restore){
            auto physical_tile = device_ctx.grid.get_physical_type({
                rr_graph.node_xlow(source_inode),
                rr_graph.node_ylow(source_inode),
                rr_graph.node_layer(source_inode)
            });
            int source_pin = rr_graph.node_pin_num(source_inode);
            int sink_pin = rr_graph.node_pin_num(sink_inode);

            auto [_, subtile] = get_sub_tile_from_pin_physical_num(physical_tile, source_pin);

            ClusterBlockId clb = place_ctx.grid_blocks().block_at_location({
                rr_graph.node_xlow(source_inode),
                rr_graph.node_ylow(source_inode),
                subtile,
                rr_graph.node_layer(source_inode)
            });

            /* Look up pb graph pins from pb type if pin is not on tile, look up from block otherwise */
            const t_pb_graph_pin* source_pb_graph_pin, *sink_pb_graph_pin;
            if(is_pin_on_tile(physical_tile, sink_pin)){
                sink_pb_graph_pin = get_pb_graph_node_pin_from_block_pin(clb, sink_pin);
            }else{
                sink_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, sink_pin);
            }
            if(is_pin_on_tile(physical_tile, source_pin)){
                source_pb_graph_pin = get_pb_graph_node_pin_from_block_pin(clb, source_pin);
            }else{
                source_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, source_pin);
            }

            t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb);

            /* Route between the pins */
            route_intra_cluster_conn(source_pb_graph_pin, sink_pb_graph_pin, convert_to_atom_net_id(net_id), pb);
        }
    }
}

/** Rebuild the ClusterNetId <-> AtomNetId lookup after compressing the ClusterNetlist.
 * Needs the old ClusterNetIds in atom_ctx.lookup. Won't work after calling compress() twice,
 * since we won't have access to the old IDs in the IdRemapper anywhere. */
inline void rebuild_atom_nets_lookup(ClusteredNetlist::IdRemapper& remapped){
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& atom_lookup = atom_ctx.lookup;

    for(auto parent_net_id: atom_ctx.nlist.nets()){
        auto atom_net_id = convert_to_atom_net_id(parent_net_id);
        auto old_clb_nets_opt = atom_lookup.clb_nets(atom_net_id);
        if(!old_clb_nets_opt)
            continue;
        std::vector<ClusterNetId> old_clb_nets = old_clb_nets_opt.value();
        atom_lookup.remove_atom_net(atom_net_id);
        for(auto old_clb_net: old_clb_nets){
            ClusterNetId new_clb_net = remapped.new_net_id(old_clb_net);
            atom_lookup.add_atom_clb_net(atom_net_id, new_clb_net);
        }
    }
}

/** Regenerate clustered netlist nets from routing results */
static void sync_clustered_netlist_to_routing(void){
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& route_ctx = g_vpr_ctx.routing();
    auto& clb_netlist = cluster_ctx.clb_nlist;
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& atom_lookup = atom_ctx.lookup;

    bool clock_net_is_routed = is_clock_net_routed();

    /* 1. Remove all nets, pins and ports from the clustered netlist.
     * If the clock net is not routed, don't remove entries for the clock net
     * otherwise we won't have data to rebuild them. */
    std::vector<ClusterNetId> nets_to_remove;
    std::vector<ClusterPinId> pins_to_remove;
    std::vector<ClusterPortId> ports_to_remove;

    for(auto net_id: clb_netlist.nets()){
        auto atom_net_id = atom_lookup.atom_net(net_id);
        if(!clock_net_is_routed && route_ctx.is_clock_net[atom_net_id])
            continue;

        nets_to_remove.push_back(net_id);
    }
    for(auto pin_id: clb_netlist.pins()){
        ClusterNetId clb_net_id = clb_netlist.pin_net(pin_id);
        auto atom_net_id = atom_lookup.atom_net(clb_net_id);
        if(!clock_net_is_routed && atom_net_id && route_ctx.is_clock_net[atom_net_id])
            continue;

        pins_to_remove.push_back(pin_id);
    }
    for(auto port_id: clb_netlist.ports()){
        ClusterNetId clb_net_id = clb_netlist.port_net(port_id, 0);
        auto atom_net_id = atom_lookup.atom_net(clb_net_id);
        if(!clock_net_is_routed && atom_net_id && route_ctx.is_clock_net[atom_net_id])
            continue;

        ports_to_remove.push_back(port_id);
    }

    /* ClusteredNetlist's iterators rely on internal lookups, so we mark for removal
     * while iterating, then remove in bulk */
    for(auto net_id: nets_to_remove){
        clb_netlist.remove_net(net_id);
        atom_lookup.remove_clb_net(net_id);
    }
    for(auto pin_id: pins_to_remove){
        clb_netlist.remove_pin(pin_id);
    }
    for(auto port_id: ports_to_remove){
        clb_netlist.remove_port(port_id);
    }

    /* 2. Reset all internal lookups for netlist */
    auto remapped = clb_netlist.compress();
    rebuild_atom_nets_lookup(remapped);

    /* 3. Walk each routing in the atom netlist. If a node is on the tile, add a ClusterPinId for it.
     * Add the associated net and port too if they don't exist */
    for(auto parent_net_id: atom_ctx.nlist.nets()){
        auto& tree = route_ctx.route_trees[parent_net_id];
        AtomNetId atom_net_id = convert_to_atom_net_id(parent_net_id);

        ClusterNetId clb_net_id;
        int clb_nets_so_far = 0;
        for(auto& rt_node: tree->all_nodes()){
            auto node_type = rr_graph.node_type(rt_node.inode);
            if(node_type != IPIN && node_type != OPIN)
                continue;

            auto physical_tile = device_ctx.grid.get_physical_type({
                rr_graph.node_xlow(rt_node.inode),
                rr_graph.node_ylow(rt_node.inode),
                rr_graph.node_layer(rt_node.inode)
            });

            int pin_index = rr_graph.node_pin_num(rt_node.inode);

            auto [_, subtile] = get_sub_tile_from_pin_physical_num(physical_tile, pin_index);

            ClusterBlockId clb = place_ctx.grid_blocks().block_at_location({
                rr_graph.node_xlow(rt_node.inode),
                rr_graph.node_ylow(rt_node.inode),
                subtile,
                rr_graph.node_layer(rt_node.inode)
            });

            if(!is_pin_on_tile(physical_tile, pin_index))
                continue;

            /* OPIN on the tile: create a new clb_net_id and add all ports & pins into here
             * Due to how the route tree is traversed, all nodes until the next OPIN on the tile will
             * be under this OPIN, so this is valid (we don't need to get the branch explicitly) */
            if(node_type == OPIN){
                std::string net_name;
                if(clb_nets_so_far == 0)
                    net_name = atom_ctx.nlist.net_name(parent_net_id);
                else
                    net_name = atom_ctx.nlist.net_name(parent_net_id) + "_" + std::to_string(clb_nets_so_far);
                clb_net_id = clb_netlist.create_net(net_name);
                atom_lookup.add_atom_clb_net(atom_net_id, clb_net_id);
                clb_nets_so_far++;
            }

            t_pb_graph_pin* pb_graph_pin = get_pb_graph_node_pin_from_block_pin(clb, pin_index);

            ClusterPortId port_id = clb_netlist.find_port(clb, pb_graph_pin->port->name);
            if(!port_id){
                PortType port_type;
                if(pb_graph_pin->port->is_clock)
                    port_type = PortType::CLOCK;
                else if(pb_graph_pin->port->type == IN_PORT)
                    port_type = PortType::INPUT;
                else if(pb_graph_pin->port->type == OUT_PORT)
                    port_type = PortType::OUTPUT;
                else
                    VTR_ASSERT_MSG(false, "Unsupported port type");
                port_id = clb_netlist.create_port(clb, pb_graph_pin->port->name, pb_graph_pin->port->num_pins, port_type);
            }
            PinType pin_type = node_type == OPIN ? PinType::DRIVER : PinType::SINK;

            ClusterPinId new_pin = clb_netlist.create_pin(port_id, pb_graph_pin->pin_number, clb_net_id, pin_type, pb_graph_pin->pin_count_in_cluster);
            clb_netlist.set_pin_net(new_pin, pin_type, clb_net_id);
        }
    }
    /* 4. Rebuild internal cluster netlist lookups */
    remapped = clb_netlist.compress();
    rebuild_atom_nets_lookup(remapped);
    /* 5. Rebuild place_ctx.physical_pins lookup
     * TODO: maybe we don't need this fn and pin_index is enough? */
    auto& blk_loc_registry = place_ctx.mutable_blk_loc_registry();
    auto& physical_pins = place_ctx.mutable_physical_pins();
    physical_pins.clear();
    for(auto clb: clb_netlist.blocks()){
        blk_loc_registry.place_sync_external_block_connections(clb);
    }
}

static void fixup_atom_pb_graph_pin_mapping(void){
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    for(ClusterBlockId clb: cluster_ctx.clb_nlist.blocks()){    
        /* Collect all innermost pb routes */
        std::vector<int> sink_pb_route_ids;
        t_pb* clb_pb = cluster_ctx.clb_nlist.block_pb(clb);
        for(auto [pb_route_id, pb_route]: clb_pb->pb_route){
            if(pb_route.sink_pb_pin_ids.empty())
                sink_pb_route_ids.push_back(pb_route_id);
        }

        for(int sink_pb_route_id: sink_pb_route_ids){
            t_pb_route& pb_route = clb_pb->pb_route.at(sink_pb_route_id);

            const t_pb_graph_pin* atom_pbg_pin = pb_route.pb_graph_pin;
            t_pb* atom_pb = clb_pb->find_mutable_pb(atom_pbg_pin->parent_node);
            AtomBlockId atb = atom_ctx.lookup.pb_atom(atom_pb);
            if(!atb)
                continue;

            /* Find atom port from pbg pin's model port */
            AtomPortId atom_port = atom_ctx.nlist.find_atom_port(atb, atom_pbg_pin->port->model_port);
            for(AtomPinId atom_pin: atom_ctx.nlist.port_pins(atom_port)){
                /* Match net IDs from pb_route and atom netlist and connect in lookup */
                if(pb_route.atom_net_id == atom_ctx.nlist.pin_net(atom_pin)){
                    atom_ctx.lookup.set_atom_pin_pb_graph_pin(atom_pin, atom_pbg_pin);
                    atom_pb->set_atom_pin_bit_index(atom_pbg_pin, atom_ctx.nlist.pin_port_bit(atom_pin));
                }
            }
        }
    }
}

/**
 * Regenerate intra-cluster routing in the packer ctx from flat routing results.
 * This function SHOULD be run ONLY when routing is finished!!!
 */
void sync_netlists_to_routing_flat(void) {
    vtr::ScopedStartFinishTimer timer("Synchronize the packed netlist to routing optimization");

    sync_clustered_netlist_to_routing();
    sync_pb_routes_to_routing();
    fixup_atom_pb_graph_pin_mapping();
}
