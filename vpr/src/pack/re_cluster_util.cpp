#include "re_cluster_util.h"

#include "vpr_context.h"
#include "clustered_netlist_utils.h"
#include "cluster_util.h"
#include "cluster_router.h"
#include "cluster_placement.h"
#include "place_macro.h"
#include "initial_placement.h"
#include "read_netlist.h"
#include <cstring>

//The name suffix of the new block (if exists)
const char* name_suffix = "_m";

/******************* Static Functions ********************/
//static void set_atom_pin_mapping(const ClusteredNetlist& clb_nlist, const AtomBlockId atom_blk, const AtomPortId atom_port, const t_pb_graph_pin* gpin);
static void load_atom_index_for_pb_pin(t_pb_routes& pb_route, int ipin);
static void load_internal_to_block_net_nums(const t_logical_block_type_ptr type, t_pb_routes& pb_route);
static bool count_children_pbs(const t_pb* pb);
static void fix_atom_pin_mapping(const AtomBlockId blk);

static void fix_cluster_pins_after_moving(const ClusterBlockId clb_index);
static void check_net_absorbtion(const AtomNetId atom_net_id,
                                 const ClusterBlockId new_clb,
                                 const ClusterBlockId old_clb,
                                 ClusterPinId& cluster_pin_id,
                                 bool& previously_absorbed,
                                 bool& now_abosrbed);

static void fix_cluster_port_after_moving(const ClusterBlockId clb_index);

static void fix_cluster_net_after_moving(const AtomBlockId& atom_id,
                                         const ClusterBlockId& old_clb,
                                         const ClusterBlockId& new_clb);

ClusterBlockId atom_to_cluster(const AtomBlockId& atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    return (atom_ctx.lookup.atom_clb(atom));
}

std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster) {
    ClusterAtomsLookup cluster_lookup;
    return (cluster_lookup.atoms_in_cluster(cluster));
}

bool remove_atom_from_cluster(const AtomBlockId& atom_id,
                              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                              ClusterBlockId& old_clb,
                              t_clustering_data& clustering_data,
                              int& imacro,
                              bool during_packing) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    //Determine the cluster ID
    old_clb = atom_to_cluster(atom_id);

    //re-build router_data structure for this cluster
    t_lb_router_data* router_data = lb_load_router_data(lb_type_rr_graphs, old_clb);

    //remove atom from router_data
    remove_atom_from_target(router_data, atom_id);

    //check cluster legality
    bool is_cluster_legal = check_cluster_legality(0, E_DETAILED_ROUTE_AT_END_ONLY, router_data);

    if (is_cluster_legal) {
        t_pb* temp = const_cast<t_pb*>(atom_ctx.lookup.atom_pb(atom_id));
        t_pb* next = temp->parent_pb;
        //char* atom_name = vtr::strdup(temp->name);
        bool has_more_children;

        revert_place_atom_block(atom_id, router_data);
        //delete atom pb
        cleanup_pb(temp);

        has_more_children = count_children_pbs(next);
        //keep deleting the parent pbs if they were created only for the removed atom
        while (!has_more_children) {
            temp = next;
            next = next->parent_pb;
            cleanup_pb(temp);
            has_more_children = count_children_pbs(next);
        }

        //if the parents' names are the same as the removed atom names,
        //update the name to prevent double the name when creating a new cluster for
        // the removed atom
        /*
         * while(next != nullptr && *(next->name) == *atom_name) {
         * next->name = vtr::strdup(child_name);
         * if(next->parent_pb == nullptr)
         * next = next->parent_pb;
         * }
         */

        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route.clear();
        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route = alloc_and_load_pb_route(router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(old_clb)->pb_graph_node);

        if (during_packing) {
            clustering_data.intra_lb_routing[old_clb] = router_data->saved_lb_nets;
            router_data->saved_lb_nets = nullptr;
        }

        else
            get_imacro_from_iblk(&imacro, old_clb, g_vpr_ctx.placement().pl_macros);
    } else {
        VTR_LOG("re-cluster: Cluster is illegal after removing an atom\n");
    }

    free_router_data(router_data);
    router_data = nullptr;

    //return true if succeeded
    return (is_cluster_legal);
}

t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs, const ClusterBlockId& clb_index) {
    //build data structures used by intra-logic block router
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto block_type = cluster_ctx.clb_nlist.block_type(clb_index);
    t_lb_router_data* router_data = alloc_and_load_router_data(&lb_type_rr_graphs[block_type->index], block_type);

    //iterate over atoms of the current cluster and add them to router data
    for (auto atom_id : cluster_to_atoms(clb_index)) {
        add_atom_as_target(router_data, atom_id);
    }
    return (router_data);
}

bool start_new_cluster_for_atom(const AtomBlockId atom_id,
                                const enum e_pad_loc_type& pad_loc_type,
                                const t_logical_block_type_ptr& type,
                                const int mode,
                                const int feasible_block_array_size,
                                int& imacro,
                                bool enable_pin_feasibility_filter,
                                ClusterBlockId clb_index,
                                t_lb_router_data** router_data,
                                std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                PartitionRegion& temp_cluster_pr,
                                t_clustering_data& clustering_data,
                                bool during_packing) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& helper_ctx = g_vpr_ctx.mutable_helper();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    t_pack_molecule* molecule = atom_ctx.atom_molecules.find(atom_id)->second;
    int verbosity = 0;

    /*Cluster's PartitionRegion is empty initially, meaning it has no floorplanning constraints*/
    PartitionRegion empty_pr;
    floorplanning_ctx.cluster_constraints.push_back(empty_pr);

    /* Allocate a dummy initial cluster and load a atom block as a seed and check if it is legal */
    AtomBlockId root_atom = molecule->atom_block_ids[molecule->root];
    const std::string& root_atom_name = atom_ctx.nlist.block_name(root_atom);
    //const t_model* root_model = atom_ctx.nlist.block_model(root_atom);

    t_pb* pb = new t_pb;
    pb->pb_graph_node = type->pb_graph_head;
    alloc_and_load_pb_stats(pb, feasible_block_array_size);
    pb->parent_pb = nullptr;

    *router_data = alloc_and_load_router_data(&lb_type_rr_graphs[type->index], type);

    e_block_pack_status pack_result = BLK_STATUS_UNDEFINED;
    pb->mode = mode;
    reset_cluster_placement_stats(&(helper_ctx.cluster_placement_stats[type->index]));
    set_mode_cluster_placement_stats(pb->pb_graph_node, mode);

    pack_result = try_pack_molecule(&(helper_ctx.cluster_placement_stats[type->index]),
                                    molecule,
                                    helper_ctx.primitives_list,
                                    pb,
                                    helper_ctx.num_models,
                                    helper_ctx.max_cluster_size,
                                    clb_index,
                                    E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                    *router_data,
                                    0,
                                    enable_pin_feasibility_filter,
                                    0,
                                    FULL_EXTERNAL_PIN_UTIL,
                                    temp_cluster_pr);

    // If clustering succeeds, add it to the clb netlist
    if (pack_result == BLK_PASSED) {
        VTR_LOGV(verbosity > 2, "\tPASSED_SEED: Block Type %s\n", type->name);
        //Once clustering succeeds, add it to the clb netlist
        if (pb->name != nullptr) {
            free(pb->name);
        }
        std::string new_name = root_atom_name + name_suffix;
        pb->name = vtr::strdup(new_name.c_str());
        clb_index = cluster_ctx.clb_nlist.create_block(new_name.c_str(), pb, type);
        helper_ctx.total_clb_num++;

        if (during_packing) {
            clustering_data.intra_lb_routing.push_back((*router_data)->saved_lb_nets);
            (*router_data)->saved_lb_nets = nullptr;
        } else {
            cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route = alloc_and_load_pb_route((*router_data)->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(clb_index)->pb_graph_node);
            g_vpr_ctx.mutable_placement().block_locs.resize(g_vpr_ctx.placement().block_locs.size() + 1);
            set_imacro_for_iblk(&imacro, clb_index);
            place_one_block(clb_index, pad_loc_type);
        }
    } else {
        free_pb(pb);
        delete pb;
    }

    //Free failed clustering
    free_router_data(*router_data);
    *router_data = nullptr;

    return (pack_result == BLK_PASSED);
}

void fix_clustered_netlist(const AtomBlockId& atom_id,
                           const ClusterBlockId& old_clb,
                           const ClusterBlockId& new_clb) {
    fix_cluster_port_after_moving(new_clb);
    fix_cluster_net_after_moving(atom_id, old_clb, new_clb);
}

/*******************************************/
/************ static functions *************/
/*******************************************/

static void fix_cluster_net_after_moving(const AtomBlockId& atom_id,
                                         const ClusterBlockId& old_clb,
                                         const ClusterBlockId& new_clb) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    AtomNetId atom_net_id;
    ClusterPinId cluster_pin;
    bool previously_absorbed, now_abosrbed;

    //remove all old cluster pin from their nets
    ClusterNetId cur_clb_net;
    for (auto& old_clb_pin : cluster_ctx.clb_nlist.block_pins(old_clb)) {
        cur_clb_net = cluster_ctx.clb_nlist.pin_net(old_clb_pin);
        cluster_ctx.clb_nlist.remove_net_pin(cur_clb_net, old_clb_pin);
    }

    //delete cluster nets that are no longer used
    for (auto atom_pin : atom_ctx.nlist.block_pins(atom_id)) {
        atom_net_id = atom_ctx.nlist.pin_net(atom_pin);
        check_net_absorbtion(atom_net_id, new_clb, old_clb, cluster_pin, previously_absorbed, now_abosrbed);

        if (!previously_absorbed && now_abosrbed) {
            cur_clb_net = cluster_ctx.clb_nlist.pin_net(cluster_pin);
            cluster_ctx.clb_nlist.remove_net(cur_clb_net);
        }
    }

    //Fix cluster pin for old and new clbs
    fix_cluster_pins_after_moving(old_clb);
    fix_cluster_pins_after_moving(new_clb);

    for (auto& atom_blk : cluster_to_atoms(old_clb))
        fix_atom_pin_mapping(atom_blk);

    for (auto& atom_blk : cluster_to_atoms(new_clb))
        fix_atom_pin_mapping(atom_blk);

    cluster_ctx.clb_nlist.remove_and_compress();
    load_internal_to_block_net_nums(cluster_ctx.clb_nlist.block_type(old_clb), cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route);
    load_internal_to_block_net_nums(cluster_ctx.clb_nlist.block_type(new_clb), cluster_ctx.clb_nlist.block_pb(new_clb)->pb_route);
}

static void fix_cluster_port_after_moving(const ClusterBlockId clb_index) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    const t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb_index);

    while (!pb->is_root()) {
        pb = pb->parent_pb;
    }

    size_t num_old_ports = cluster_ctx.clb_nlist.block_ports(clb_index).size();
    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;

    for (size_t port = num_old_ports; port < (unsigned)pb_type->num_ports; port++) {
        if (pb_type->ports[port].is_clock && pb_type->ports[port].type == IN_PORT) {
            cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::CLOCK);
        } else if (!pb_type->ports[port].is_clock && pb_type->ports[port].type == IN_PORT) {
            cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::INPUT);
        } else {
            VTR_ASSERT(pb_type->ports[port].type == OUT_PORT);
            cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::OUTPUT);
        }
    }

    num_old_ports = cluster_ctx.clb_nlist.block_ports(clb_index).size();
}

static void fix_cluster_pins_after_moving(const ClusterBlockId clb_index) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    const t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb_index);
    t_pb_graph_pin* pb_graph_pin;
    AtomNetId atom_net_id;
    ClusterNetId clb_net_id;

    t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(clb_index);

    int num_input_ports = pb->pb_graph_node->num_input_ports;
    int num_output_ports = pb->pb_graph_node->num_output_ports;
    int num_clock_ports = pb->pb_graph_node->num_clock_ports;

    int j, k, ipin, rr_node_index;

    ipin = 0;
    for (j = 0; j < num_input_ports; j++) {
        ClusterPortId input_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[j].name);
        for (k = 0; k < pb->pb_graph_node->num_input_pins[j]; k++) {
            pb_graph_pin = &pb->pb_graph_node->input_pins[j][k];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(input_port_id, (BitIndex)k);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(input_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
                    else
                        cluster_ctx.clb_nlist.set_pin_net(cur_pin_id, PinType::SINK, clb_net_id);
                }
                cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route[rr_node_index].pb_graph_pin = pb_graph_pin;
            }
            ipin++;
        }
    }

    for (j = 0; j < num_output_ports; j++) {
        ClusterPortId output_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports + j].name);
        for (k = 0; k < pb->pb_graph_node->num_output_pins[j]; k++) {
            pb_graph_pin = &pb->pb_graph_node->output_pins[j][k];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(output_port_id, (BitIndex)k);
                    AtomPinId atom_net_driver = atom_ctx.nlist.net_driver(atom_net_id);
                    bool driver_is_constant = atom_ctx.nlist.pin_is_constant(atom_net_driver);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(output_port_id, (BitIndex)k, clb_net_id, PinType::DRIVER, ipin, driver_is_constant);
                    else {
                        cluster_ctx.clb_nlist.set_pin_net(cur_pin_id, PinType::DRIVER, clb_net_id);
                        cluster_ctx.clb_nlist.set_pin_is_constant(cur_pin_id, driver_is_constant);
                    }
                    VTR_ASSERT(cluster_ctx.clb_nlist.net_is_constant(clb_net_id) == driver_is_constant);
                }
                cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route[rr_node_index].pb_graph_pin = pb_graph_pin;
            }
            ipin++;
        }
    }

    for (j = 0; j < num_clock_ports; j++) {
        ClusterPortId clock_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports + num_output_ports + j].name);
        for (k = 0; k < pb->pb_graph_node->num_clock_pins[j]; k++) {
            pb_graph_pin = &pb->pb_graph_node->clock_pins[j][k];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(clock_port_id, (BitIndex)k);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(clock_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
                    else
                        cluster_ctx.clb_nlist.set_pin_net(cur_pin_id, PinType::SINK, clb_net_id);
                }
                cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route[rr_node_index].pb_graph_pin = pb_graph_pin;
            }
            ipin++;
        }
    }
}

static void check_net_absorbtion(const AtomNetId atom_net_id,
                                 const ClusterBlockId new_clb,
                                 const ClusterBlockId old_clb,
                                 ClusterPinId& cluster_pin_id,
                                 bool& previously_absorbed,
                                 bool& now_abosrbed) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    AtomBlockId atom_block_id;
    ClusterBlockId clb_index;

    ClusterNetId clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);

    if (clb_net_id == ClusterNetId::INVALID())
        previously_absorbed = true;
    else {
        previously_absorbed = false;
        for (auto& cluster_pin : cluster_ctx.clb_nlist.net_pins(clb_net_id)) {
            if (cluster_pin && cluster_ctx.clb_nlist.pin_block(cluster_pin) == old_clb) {
                cluster_pin_id = cluster_pin;
                break;
            }
        }
    }

    //iterate over net pins and check their cluster
    now_abosrbed = true;
    for (auto& net_pin : atom_ctx.nlist.net_pins(atom_net_id)) {
        atom_block_id = atom_ctx.nlist.pin_block(net_pin);
        clb_index = atom_ctx.lookup.atom_clb(atom_block_id);

        if (clb_index != new_clb) {
            now_abosrbed = false;
            break;
        }
    }
}

static void fix_atom_pin_mapping(const AtomBlockId blk) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_pb* pb = atom_ctx.lookup.atom_pb(blk);
    VTR_ASSERT_MSG(pb, "Atom block must have a matching PB");

    const t_pb_graph_node* gnode = pb->pb_graph_node;
    VTR_ASSERT_MSG(gnode->pb_type->model == atom_ctx.nlist.block_model(blk),
                   "Atom block PB must match BLIF model");

    for (int iport = 0; iport < gnode->num_input_ports; ++iport) {
        if (gnode->num_input_pins[iport] <= 0) continue;

        const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->input_pins[iport][0].port->model_port);
        if (!port) continue;

        for (int ipin = 0; ipin < gnode->num_input_pins[iport]; ++ipin) {
            const t_pb_graph_pin* gpin = &gnode->input_pins[iport][ipin];
            VTR_ASSERT(gpin);

            set_atom_pin_mapping(cluster_ctx.clb_nlist, blk, port, gpin);
        }
    }

    for (int iport = 0; iport < gnode->num_output_ports; ++iport) {
        if (gnode->num_output_pins[iport] <= 0) continue;

        const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->output_pins[iport][0].port->model_port);
        if (!port) continue;

        for (int ipin = 0; ipin < gnode->num_output_pins[iport]; ++ipin) {
            const t_pb_graph_pin* gpin = &gnode->output_pins[iport][ipin];
            VTR_ASSERT(gpin);

            set_atom_pin_mapping(cluster_ctx.clb_nlist, blk, port, gpin);
        }
    }

    for (int iport = 0; iport < gnode->num_clock_ports; ++iport) {
        if (gnode->num_clock_pins[iport] <= 0) continue;

        const AtomPortId port = atom_ctx.nlist.find_atom_port(blk, gnode->clock_pins[iport][0].port->model_port);
        if (!port) continue;

        for (int ipin = 0; ipin < gnode->num_clock_pins[iport]; ++ipin) {
            const t_pb_graph_pin* gpin = &gnode->clock_pins[iport][ipin];
            VTR_ASSERT(gpin);

            set_atom_pin_mapping(cluster_ctx.clb_nlist, blk, port, gpin);
        }
    }
}

static void load_internal_to_block_net_nums(const t_logical_block_type_ptr type, t_pb_routes& pb_route) {
    int num_pins = type->pb_graph_head->total_pb_pins;

    for (int i = 0; i < num_pins; i++) {
        if (!pb_route.count(i)) continue;

        //if (pb_route[i].driver_pb_pin_id != OPEN && !pb_route[i].atom_net_id) {
        if (pb_route[i].driver_pb_pin_id != OPEN) {
            load_atom_index_for_pb_pin(pb_route, i);
        }
    }
}

static void load_atom_index_for_pb_pin(t_pb_routes& pb_route, int ipin) {
    int driver = pb_route[ipin].driver_pb_pin_id;

    VTR_ASSERT(driver != OPEN);
    //VTR_ASSERT(!pb_route[ipin].atom_net_id);

    if (!pb_route[driver].atom_net_id) {
        load_atom_index_for_pb_pin(pb_route, driver);
    }

    //Store the net coming from the driver
    pb_route[ipin].atom_net_id = pb_route[driver].atom_net_id;

    //Store ourselves with the driver
    pb_route[driver].sink_pb_pin_ids.push_back(ipin);
}

static bool count_children_pbs(const t_pb* pb) {
    if (pb == nullptr)
        return 0;

    for (int i = 0; i < pb->get_num_child_types(); i++) {
        for (int j = 0; j < pb->get_num_children_of_type(i); j++) {
            if (pb->child_pbs[i] != nullptr && pb->child_pbs[i][j].name != nullptr) {
                return true;
            }
        }
    }
    return false;
}