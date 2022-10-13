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
//static bool count_children_pbs(const t_pb* pb);
static void fix_atom_pin_mapping(const AtomBlockId blk);

static void fix_cluster_pins_after_moving(const ClusterBlockId clb_index);
static void check_net_absorbtion(const AtomNetId atom_net_id,
                                 const ClusterBlockId new_clb,
                                 const ClusterBlockId old_clb,
                                 ClusterPinId& cluster_pin_id,
                                 bool& previously_absorbed,
                                 bool& now_abosrbed);

static void fix_cluster_port_after_moving(const ClusterBlockId clb_index);

static void fix_cluster_net_after_moving(const t_pack_molecule* molecule,
                                         int molecule_size,
                                         const ClusterBlockId& old_clb,
                                         const ClusterBlockId& new_clb);

static void rebuild_cluster_placemet_stats(const ClusterBlockId& clb_index,
                                           const std::vector<AtomBlockId>& clb_atoms,
                                           int type_idx,
                                           int mode);

/*****************  API functions ***********************/
ClusterBlockId atom_to_cluster(const AtomBlockId& atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    return (atom_ctx.lookup.atom_clb(atom));
}

std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster) {
    ClusterAtomsLookup cluster_lookup;
    return (cluster_lookup.atoms_in_cluster(cluster));
}

void remove_mol_from_cluster(const t_pack_molecule* molecule,
                             int molecule_size,
                             ClusterBlockId& old_clb,
                             std::vector<AtomBlockId>& old_clb_atoms,
                             t_lb_router_data*& router_data) {
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();

    //re-build router_data structure for this cluster
    router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, old_clb, old_clb_atoms);

    //remove atom from router_data
    for (int i_atom = 0; i_atom < molecule_size; i_atom++) {
        if (molecule->atom_block_ids[i_atom]) {
            remove_atom_from_target(router_data, molecule->atom_block_ids[i_atom]);
            old_clb_atoms.erase(std::remove(old_clb_atoms.begin(), old_clb_atoms.end(), molecule->atom_block_ids[i_atom]));
        }
    }
}

void commit_mol_move(const ClusterBlockId& old_clb,
                     const ClusterBlockId& new_clb,
                     bool during_packing,
                     bool new_clb_created) {
    auto& device_ctx = g_vpr_ctx.device();

    //Place the new cluster if this function called during placement (after the initial placement is done)
    if (!during_packing && new_clb_created) {
        int imacro;
        g_vpr_ctx.mutable_placement().block_locs.resize(g_vpr_ctx.placement().block_locs.size() + 1);
        get_imacro_from_iblk(&imacro, old_clb, g_vpr_ctx.placement().pl_macros);
        set_imacro_for_iblk(&imacro, new_clb);
        place_one_block(new_clb, device_ctx.pad_loc_type, NULL);
    }
}

t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs, const ClusterBlockId& clb_index, const std::vector<AtomBlockId>& clb_atoms) {
    //build data structures used by intra-logic block router
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto block_type = cluster_ctx.clb_nlist.block_type(clb_index);
    t_lb_router_data* router_data = alloc_and_load_router_data(&lb_type_rr_graphs[block_type->index], block_type);

    //iterate over atoms of the current cluster and add them to router data
    for (auto atom_id : clb_atoms) {
        add_atom_as_target(router_data, atom_id);
    }
    return (router_data);
}

bool start_new_cluster_for_mol(t_pack_molecule* molecule,
                               const t_logical_block_type_ptr& type,
                               const int mode,
                               const int feasible_block_array_size,
                               bool enable_pin_feasibility_filter,
                               ClusterBlockId clb_index,
                               bool during_packing,
                               int verbosity,
                               t_clustering_data& clustering_data,
                               t_lb_router_data** router_data,
                               PartitionRegion& temp_cluster_pr) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    /* Cluster's PartitionRegion is empty initially, meaning it has no floorplanning constraints */
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

    *router_data = alloc_and_load_router_data(&(helper_ctx.lb_type_rr_graphs[type->index]), type);

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
        VTR_LOGV(verbosity > 4, "\tPASSED_SEED: Block Type %s\n", type->name);
        //Once clustering succeeds, add it to the clb netlist
        if (pb->name != nullptr) {
            free(pb->name);
        }
        std::string new_name = root_atom_name + name_suffix;
        pb->name = vtr::strdup(new_name.c_str());
        clb_index = cluster_ctx.clb_nlist.create_block(new_name.c_str(), pb, type);
        helper_ctx.total_clb_num++;

        //If you are still in packing, update the clustering data. Otherwise, update the clustered netlist.
        if (during_packing) {
            clustering_data.intra_lb_routing.push_back((*router_data)->saved_lb_nets);
            (*router_data)->saved_lb_nets = nullptr;
        } else {
            cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route = alloc_and_load_pb_route((*router_data)->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(clb_index)->pb_graph_node);
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

bool pack_mol_in_existing_cluster(t_pack_molecule* molecule,
                                  const ClusterBlockId new_clb,
                                  const std::vector<AtomBlockId>& new_clb_atoms,
                                  bool during_packing,
                                  bool is_swap,
                                  t_clustering_data& clustering_data,
                                  t_lb_router_data*& router_data) {
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    PartitionRegion temp_cluster_pr;
    e_block_pack_status pack_result = BLK_STATUS_UNDEFINED;
    t_ext_pin_util target_ext_pin_util = helper_ctx.target_external_pin_util.get_pin_util(cluster_ctx.clb_nlist.block_type(new_clb)->name);
    t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(new_clb);
    t_pb* temp_pb = cluster_ctx.clb_nlist.block_pb(new_clb);

    //re-build cluster placement stats
    rebuild_cluster_placemet_stats(new_clb, new_clb_atoms, cluster_ctx.clb_nlist.block_type(new_clb)->index, cluster_ctx.clb_nlist.block_pb(new_clb)->mode);

    //re-build router_data structure for this cluster
    if (!is_swap)
        router_data = lb_load_router_data(helper_ctx.lb_type_rr_graphs, new_clb, new_clb_atoms);

    pack_result = try_pack_molecule(&(helper_ctx.cluster_placement_stats[block_type->index]),
                                    molecule,
                                    helper_ctx.primitives_list,
                                    temp_pb,
                                    helper_ctx.num_models,
                                    helper_ctx.max_cluster_size,
                                    new_clb,
                                    E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                    router_data,
                                    0,
                                    //helper_ctx.enable_pin_feasibility_filter,
                                    false,
                                    helper_ctx.feasible_block_array_size,
                                    target_ext_pin_util,
                                    temp_cluster_pr);

    // If clustering succeeds, add it to the clb netlist
    if (pack_result == BLK_PASSED) {
        //If you are still in packing, update the clustering data. Otherwise, update the clustered netlist.
        if (during_packing) {
            free_intra_lb_nets(clustering_data.intra_lb_routing[new_clb]);
            clustering_data.intra_lb_routing[new_clb] = router_data->saved_lb_nets;
            router_data->saved_lb_nets = nullptr;
        } else {
            cluster_ctx.clb_nlist.block_pb(new_clb)->pb_route.clear();
            cluster_ctx.clb_nlist.block_pb(new_clb)->pb_route = alloc_and_load_pb_route(router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(new_clb)->pb_graph_node);
        }
    }

    if (pack_result == BLK_PASSED || !is_swap) {
        //Free clustering router data
        free_router_data(router_data);
        router_data = nullptr;
    }

    return (pack_result == BLK_PASSED);
}

void fix_clustered_netlist(t_pack_molecule* molecule,
                           int molecule_size,
                           const ClusterBlockId& old_clb,
                           const ClusterBlockId& new_clb) {
    fix_cluster_port_after_moving(new_clb);
    fix_cluster_net_after_moving(molecule, molecule_size, old_clb, new_clb);
}

void revert_mol_move(const ClusterBlockId& old_clb,
                     t_pack_molecule* molecule,
                     t_lb_router_data*& old_router_data,
                     bool during_packing,
                     t_clustering_data& clustering_data) {
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    PartitionRegion temp_cluster_pr_original;
    e_block_pack_status pack_result = try_pack_molecule(&(helper_ctx.cluster_placement_stats[cluster_ctx.clb_nlist.block_type(old_clb)->index]),
                                                        molecule,
                                                        helper_ctx.primitives_list,
                                                        cluster_ctx.clb_nlist.block_pb(old_clb),
                                                        helper_ctx.num_models,
                                                        helper_ctx.max_cluster_size,
                                                        old_clb,
                                                        E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                                        old_router_data,
                                                        0,
                                                        helper_ctx.enable_pin_feasibility_filter,
                                                        helper_ctx.feasible_block_array_size,
                                                        helper_ctx.target_external_pin_util.get_pin_util(cluster_ctx.clb_nlist.block_type(old_clb)->name),
                                                        temp_cluster_pr_original);

    VTR_ASSERT(pack_result == BLK_PASSED);
    //If you are still in packing, update the clustering data. Otherwise, update the clustered netlist.
    if (during_packing) {
        free_intra_lb_nets(clustering_data.intra_lb_routing[old_clb]);
        clustering_data.intra_lb_routing[old_clb] = old_router_data->saved_lb_nets;
        old_router_data->saved_lb_nets = nullptr;
    } else {
        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route.clear();
        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route = alloc_and_load_pb_route(old_router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(old_clb)->pb_graph_node);
    }

    free_router_data(old_router_data);
    old_router_data = nullptr;
}
/*******************************************/
/************ static functions *************/
/*******************************************/

static void fix_cluster_net_after_moving(const t_pack_molecule* molecule,
                                         int molecule_size,
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
    for (int i_atom = 0; i_atom < molecule_size; i_atom++) {
        if (molecule->atom_block_ids[i_atom]) {
            for (auto atom_pin : atom_ctx.nlist.block_pins(molecule->atom_block_ids[i_atom])) {
                atom_net_id = atom_ctx.nlist.pin_net(atom_pin);
                check_net_absorbtion(atom_net_id, new_clb, old_clb, cluster_pin, previously_absorbed, now_abosrbed);

                if (!previously_absorbed && now_abosrbed) {
                    cur_clb_net = cluster_ctx.clb_nlist.pin_net(cluster_pin);
                    cluster_ctx.clb_nlist.remove_net(cur_clb_net);
                }
            }
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

    int iport, ipb_pin, ipin, rr_node_index;

    ipin = 0;
    // iterating over input ports
    for (iport = 0; iport < num_input_ports; iport++) {
        ClusterPortId input_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[iport].name);
        // iterating over physical block pins of each input port
        for (ipb_pin = 0; ipb_pin < pb->pb_graph_node->num_input_pins[iport]; ipb_pin++) {
            pb_graph_pin = &pb->pb_graph_node->input_pins[iport][ipb_pin];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(input_port_id, (BitIndex)ipb_pin);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(input_port_id, (BitIndex)ipb_pin, clb_net_id, PinType::SINK, ipin);
                    else
                        cluster_ctx.clb_nlist.set_pin_net(cur_pin_id, PinType::SINK, clb_net_id);
                }
                cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route[rr_node_index].pb_graph_pin = pb_graph_pin;
            }
            ipin++;
        }
    }

    // iterating over output ports
    for (iport = 0; iport < num_output_ports; iport++) {
        ClusterPortId output_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports + iport].name);
        // iterating over physical block pins of each output port
        for (ipb_pin = 0; ipb_pin < pb->pb_graph_node->num_output_pins[iport]; ipb_pin++) {
            pb_graph_pin = &pb->pb_graph_node->output_pins[iport][ipb_pin];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(output_port_id, (BitIndex)ipb_pin);
                    AtomPinId atom_net_driver = atom_ctx.nlist.net_driver(atom_net_id);
                    bool driver_is_constant = atom_ctx.nlist.pin_is_constant(atom_net_driver);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(output_port_id, (BitIndex)ipb_pin, clb_net_id, PinType::DRIVER, ipin, driver_is_constant);
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

    // iterating over clock ports
    for (iport = 0; iport < num_clock_ports; iport++) {
        ClusterPortId clock_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports + num_output_ports + iport].name);
        // iterating over physical block pins of each clock port
        for (ipb_pin = 0; ipb_pin < pb->pb_graph_node->num_clock_pins[iport]; ipb_pin++) {
            pb_graph_pin = &pb->pb_graph_node->clock_pins[iport][ipb_pin];
            rr_node_index = pb_graph_pin->pin_count_in_cluster;

            VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
            if (pb->pb_route.count(rr_node_index)) {
                atom_net_id = pb->pb_route[rr_node_index].atom_net_id;
                if (atom_net_id) {
                    clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
                    atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
                    ClusterPinId cur_pin_id = cluster_ctx.clb_nlist.find_pin(clock_port_id, (BitIndex)ipb_pin);
                    if (!cur_pin_id)
                        cluster_ctx.clb_nlist.create_pin(clock_port_id, (BitIndex)ipb_pin, clb_net_id, PinType::SINK, ipin);
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

    for (int ipb_pin = 0; ipb_pin < num_pins; ipb_pin++) {
        if (!pb_route.count(ipb_pin)) continue;

        if (pb_route[ipb_pin].driver_pb_pin_id != OPEN) {
            load_atom_index_for_pb_pin(pb_route, ipb_pin);
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

#if 0
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
#endif

static void rebuild_cluster_placemet_stats(const ClusterBlockId& clb_index,
                                           const std::vector<AtomBlockId>& clb_atoms,
                                           int type_idx,
                                           int mode) {
    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    t_cluster_placement_stats* cluster_placement_stats = &(helper_ctx.cluster_placement_stats[type_idx]);

    reset_cluster_placement_stats(cluster_placement_stats);
    set_mode_cluster_placement_stats(cluster_ctx.clb_nlist.block_pb(clb_index)->pb_graph_node, mode);

    std::set<t_pb_graph_node*> list_of_pb_atom_nodes;
    t_cluster_placement_primitive* cur_cluster_placement_primitive;

    for (auto& atom_blk : clb_atoms) {
        list_of_pb_atom_nodes.insert(atom_ctx.lookup.atom_pb(atom_blk)->pb_graph_node);
    }

    for (int i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        cur_cluster_placement_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
        while (cur_cluster_placement_primitive != nullptr) {
            if (list_of_pb_atom_nodes.find(cur_cluster_placement_primitive->pb_graph_node) != list_of_pb_atom_nodes.end()) {
                cur_cluster_placement_primitive->valid = false;
            }
            cur_cluster_placement_primitive = cur_cluster_placement_primitive->next_primitive;
        }
    }
}

bool is_cluster_legal(t_lb_router_data*& router_data) {
    return (check_cluster_legality(0, E_DETAILED_ROUTE_AT_END_ONLY, router_data));
}

void commit_mol_removal(const t_pack_molecule* molecule,
                        const int& molecule_size,
                        const ClusterBlockId& old_clb,
                        bool during_packing,
                        t_lb_router_data*& router_data,
                        t_clustering_data& clustering_data) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    for (int i_atom = 0; i_atom < molecule_size; i_atom++) {
        if (molecule->atom_block_ids[i_atom]) {
            revert_place_atom_block(molecule->atom_block_ids[i_atom], router_data);
        }
    }

    cleanup_pb(cluster_ctx.clb_nlist.block_pb(old_clb));

    //If you are still in packing, update the clustering data. Otherwise, update the clustered netlist.
    if (during_packing) {
        free_intra_lb_nets(clustering_data.intra_lb_routing[old_clb]);
        clustering_data.intra_lb_routing[old_clb] = router_data->saved_lb_nets;
        router_data->saved_lb_nets = nullptr;
    } else {
        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route.clear();
        cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route = alloc_and_load_pb_route(router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(old_clb)->pb_graph_node);
    }
}

bool check_type_and_mode_compitability(const ClusterBlockId& old_clb,
                                       const ClusterBlockId& new_clb,
                                       int verbosity) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Check that the old and new clusters are the same type
    if (cluster_ctx.clb_nlist.block_type(old_clb) != cluster_ctx.clb_nlist.block_type(new_clb)) {
        VTR_LOGV(verbosity > 4, "Move aborted. New and old cluster blocks are not of the same type");
        return false;
    }

    //Check that the old and new clusters are the mode
    if (cluster_ctx.clb_nlist.block_pb(old_clb)->mode != cluster_ctx.clb_nlist.block_pb(new_clb)->mode) {
        VTR_LOGV(verbosity > 4, "Move aborted. New and old cluster blocks are not of the same mode");
        return false;
    }

    return true;
}
