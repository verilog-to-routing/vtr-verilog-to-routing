#include "re_cluster_util.h"

#include "vpr_context.h"
#include "clustered_netlist_utils.h"
#include "cluster_util.h"
#include "cluster_router.h"
#include "cluster_placement.h"

std::unordered_set<ClusterPinId> free_pins;

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
							  ClusterBlockId& old_clb) {
    
	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

	//Determine the cluster ID
	old_clb = atom_to_cluster(atom_id);

	//re-build router_data structure for this cluster
	t_lb_router_data* router_data = lb_load_router_data(lb_type_rr_graphs, old_clb);

	//remove atom from router_data
	remove_atom_from_target(router_data, atom_id);
	
	//check cluster legality
	bool is_cluster_legal = check_cluster_legality(0, E_DETAILED_ROUTE_AT_END_ONLY, router_data);

	if(is_cluster_legal) {
		revert_place_atom_block(atom_id, router_data);
		cluster_ctx.clb_nlist.block_pb(old_clb)->pb_route = alloc_and_load_pb_route(router_data->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(old_clb)->pb_graph_node);
	}
	else
        VTR_LOG("re-cluster: Cluster is illegal after removing an atom\n");
   

    cleanup_pb(cluster_ctx.clb_nlist.block_pb(old_clb));

	//free router_data memory
	free_router_data(router_data);
	router_data = nullptr;

	//return true if succeeded
	return(is_cluster_legal);
}


t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs, const ClusterBlockId& clb_index) {
	//build data structures used by intra-logic block router
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto block_type = cluster_ctx.clb_nlist.block_type(clb_index);
	t_lb_router_data* router_data = alloc_and_load_router_data(&lb_type_rr_graphs[block_type->index], block_type);

	//iterate over atoms of the current cluster and add them to router data
	for(auto atom_id : cluster_to_atoms(clb_index)) {
		add_atom_as_target(router_data, atom_id);
	}
    return(router_data);
}

bool start_new_cluster_for_atom(const AtomBlockId atom_id,
					   const t_logical_block_type_ptr& type,
					   const int mode,
					   const int feasible_block_array_size,
					   bool enable_pin_feasibility_filter,
					   ClusterBlockId clb_index,
					   t_lb_router_data** router_data,
					   std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
					   PartitionRegion& temp_cluster_pr) {
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
    if(pack_result == BLK_PASSED) {
    	VTR_LOGV(verbosity > 2, "\tPASSED_SEED: Block Type %s\n", type->name);
        //Once clustering succeeds, add it to the clb netlist
        if (pb->name != nullptr) {
            free(pb->name);
        }
        pb->name = vtr::strdup(root_atom_name.c_str());
        clb_index = cluster_ctx.clb_nlist.create_block(root_atom_name.c_str(), pb, type);
        helper_ctx.total_clb_num++;
/*
        for(auto& temp_net : *((**router_data).saved_lb_nets)) {
        	VTR_LOG("ZZZ %zu\n", temp_net.atom_net_id);
        	for(auto& temp_pin : temp_net.atom_pins) {
        		VTR_LOG("ZZZZ %zu\n", temp_pin);
        	}
        }
*/

        cluster_ctx.clb_nlist.block_pb(clb_index)->pb_route = alloc_and_load_pb_route((*router_data)->saved_lb_nets, cluster_ctx.clb_nlist.block_pb(clb_index)->pb_graph_node);
    } else {
        free_pb(pb);
        delete pb;
    }
    
    //Free failed clustering
    free_router_data(*router_data);
    *router_data = nullptr;

    return (pack_result == BLK_PASSED);
}



void fix_cluster_net_after_moving(const AtomBlockId& atom_id, 
								  const ClusterBlockId& old_clb,
								  const ClusterBlockId& new_clb) {

	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
	auto& atom_ctx = g_vpr_ctx.mutable_atom();
	
	AtomNetId atom_net_id;
	ClusterPinId cluster_pin;
	bool previously_absorbed, now_abosrbed;

	for(auto& atom_pin : atom_ctx.nlist.block_pins(atom_id)) {
		
		atom_net_id = atom_ctx.nlist.pin_net(atom_pin);
		check_net_absorbtion(atom_net_id, new_clb, old_clb, cluster_pin, previously_absorbed, now_abosrbed);

		if(!previously_absorbed) {
			cluster_ctx.clb_nlist.remove_net_pin(atom_ctx.lookup.clb_net(atom_net_id), cluster_pin);
			free_pins.insert(cluster_pin);
		}
	}

	for(auto& atom_pin : atom_ctx.)

		if(!now_abosrbed)
			create_cluster_net_for_atom_net(atom_net_id);

		/*
		if(previously_absorbed && now_abosrbed)
			continue;
		else if(previously_absorbed && !now_abosrbed) {
			create_cluster_net_for_atom_net(atom_net_id);
		} else if(!previously_absorbed && now_abosrbed) {
			delete_cluster_net_of_atom_net(atom_net_id);
		} else {
			VTR_ASSERT(!previously_absorbed && !now_abosrbed);
			continue;
		}
		*/
	}
	cluster_ctx.clb_nlist.compress();
}


void fix_cluster_port_after_moving(const ClusterBlockId clb_index) {
	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
	const t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb_index);
	
	while(!pb->is_root()) {
		pb = pb->parent_pb;
	}

	size_t num_old_ports = cluster_ctx.clb_nlist.block_ports(clb_index).size();
	const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
	//VTR_LOG("BBB: %zu, %zu\n", num_old_ports, pb_type->num_ports);


	for(size_t port = num_old_ports; port < (unsigned)pb_type->num_ports; port++) {
		if(pb_type->ports[port].is_clock && pb_type->ports[port].type == IN_PORT) {
			cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::CLOCK);
		} else if(!pb_type->ports[port].is_clock && pb_type->ports[port].type == IN_PORT) {
			cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::INPUT);
		} else {
			VTR_ASSERT(pb_type->ports[port].type == OUT_PORT);
			cluster_ctx.clb_nlist.create_port(clb_index, pb_type->ports[port].name, pb_type->ports[port].num_pins, PortType::OUTPUT);
		}
	}

	num_old_ports = cluster_ctx.clb_nlist.block_ports(clb_index).size();
}

/*
void fix_cluster_pin_after_moving(const ClusterBlockId clb_index) {
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

	int j,k, ipin;

	ipin = 0;

	for (j = 0; j < num_input_ports; j++) {
		ClusterPortId input_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[j].name);
		for (k = 0; k < pb->pb_graph_node->num_input_pins[j]; k++) {
			pb_graph_pin = &pb->pb_graph_node->input_pins[j][k];
			VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
			if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
				VTR_LOG("BBB\n");
				atom_net_id = pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id;
				if(atom_net_id) {
					clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
					atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
					//clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
					cluster_ctx.clb_nlist.create_pin(input_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
				}
			}
			ipin++;
		}

	}

	for (j = 0; j < num_output_ports; j++) {
		ClusterPortId output_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports+j].name);
		size_t temp = cluster_ctx.clb_nlist.port_pins(output_port_id).size();
		for (k = 0; k < pb->pb_graph_node->num_output_pins[j]; k++) {
			pb_graph_pin = &pb->pb_graph_node->output_pins[j][k];
			VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
			if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
				VTR_LOG("BBBB\n");
				atom_net_id = pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id;
				if (atom_net_id) {
					clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
					atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
					//clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
					AtomPinId atom_net_driver = atom_ctx.nlist.net_driver(atom_net_id);
					bool driver_is_constant = atom_ctx.nlist.pin_is_constant(atom_net_driver);
					//PortType temp_type  = cluster_ctx.clb_nlist.port_type(ClusterPortId(12));
					cluster_ctx.clb_nlist.create_pin(output_port_id, (BitIndex)k, clb_net_id, PinType::DRIVER, ipin, driver_is_constant);
					VTR_ASSERT(cluster_ctx.clb_nlist.net_is_constant(clb_net_id) == driver_is_constant);
				}
			}
			ipin++;
		}

	}

	for (j = 0; j < num_clock_ports; j++) {
		ClusterPortId clock_port_id = cluster_ctx.clb_nlist.find_port(clb_index, block_type->pb_type->ports[num_input_ports+num_output_ports+j].name);
		for (k = 0; k < pb->pb_graph_node->num_clock_pins[j]; k++) {
			pb_graph_pin = &pb->pb_graph_node->clock_pins[j][k];
			VTR_ASSERT(pb_graph_pin->pin_count_in_cluster == ipin);
			if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster)) {
				VTR_LOG("BBBBB\n");
				if (atom_net_id) {
					clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
					atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
					//clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
					cluster_ctx.clb_nlist.create_pin(clock_port_id, (BitIndex)k, clb_net_id, PinType::SINK, ipin);
				}
			}
			ipin++;
		}

	}
	cluster_ctx.clb_nlist.verify();
}
*/

void check_net_absorbtion(const AtomNetId atom_net_id,
								const ClusterBlockId new_clb,
								const ClusterBlockId old_clb,
								ClusterPinId& cluster_pin_id,
								bool& previously_absorbed,
								bool& now_abosrbed) {

	auto& atom_ctx = g_vpr_ctx.atom();

	AtomBlockId atom_block_id;
	ClusterBlockId clb_index;

	ClusterNetId clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);

	if(clb_net_id == ClusterNetId::INVALID())
		previously_absorbed = true;
	else {
		previously_absorbed = false;
		for(auto& cluster_pin : cluster_ctx.clb_nlist.net_pins(clb_net_id)) {
			if(cluster_ctx.clb_nlist.pin_block(cluster_pin) == old_clb) {
				cluster_pin_id = cluster_pin;
				break;
			}
		}
	}

	//iterate over net pins and check their cluster
	now_abosrbed = true;
	for(auto& net_pin: atom_ctx.nlist.net_pins(atom_net_id)) {
		atom_block_id = atom_ctx.nlist.pin_block(net_pin);
		clb_index = atom_ctx.lookup.atom_clb(atom_block_id);

		if(clb_index != new_clb) {
			now_abosrbed = false;
			break;
		}
	}
}

void create_cluster_net_for_atom_net(const AtomNetId& atom_net_id) {
	auto& atom_ctx = g_vpr_ctx.mutable_atom();
	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

	AtomBlockId atom_block_id;

	ClusterNetId clb_net_id = cluster_ctx.clb_nlist.create_net(atom_ctx.nlist.net_name(atom_net_id));
	atom_ctx.lookup.set_atom_clb_net(atom_net_id, clb_net_id);
	//iterating over the different pins of the atom net
	for(auto& atom_pin : atom_ctx.nlist.net_pins(atom_net_id)) {
		create_cluster_pin_for_atom_pin(atom_pin);
	}
}


void create_cluster_pin_for_atom_pin(const AtomPinId& atom_pin_id) {
	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
	auto& atom_ctx = g_vpr_ctx.atom();

	AtomNetId atom_net_id = atom_ctx.nlist.pin_net(atom_pin_id);
	ClusterNetId clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
	AtomBlockId atom_block_id = atom_ctx.nlist.pin_block(atom_pin_id);
	ClusterBlockId cluster_block_id = atom_ctx.lookup.atom_clb(atom_block_id);
	const t_pb* pb = cluster_ctx.clb_nlist.block_pb(cluster_block_id);
	t_logical_block_type_ptr block_type = cluster_ctx.clb_nlist.block_type(cluster_block_id);

	int num_input_ports = pb->pb_graph_node->num_input_ports;
	int num_output_ports = pb->pb_graph_node->num_output_ports;
	int num_clock_ports = pb->pb_graph_node->num_clock_ports;

	int j,k;
	t_pb_graph_pin* pb_graph_pin;

	if(atom_ctx.nlist.pin_type(atom_pin_id) == PinType::SINK) {
		//iterate over input pins
		for (j = 0; j < num_input_ports; j++) {
			ClusterPortId input_port_id = cluster_ctx.clb_nlist.find_port(cluster_block_id, block_type->pb_type->ports[j].name);
			for (k = 0; k < pb->pb_graph_node->num_input_pins[j]; k++) {
				pb_graph_pin = &pb->pb_graph_node->input_pins[j][k];
				if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster) && atom_net_id == pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id) {
					cluster_ctx.clb_nlist.create_pin(input_port_id, (BitIndex)k, clb_net_id, PinType::SINK, pb_graph_pin->pin_count_in_cluster);
					return;
				}
			}
		}

		for (j = 0; j < num_clock_ports; j++) {
			ClusterPortId clock_port_id = cluster_ctx.clb_nlist.find_port(cluster_block_id, block_type->pb_type->ports[num_input_ports+num_output_ports+j].name);
			for (k = 0; k < pb->pb_graph_node->num_clock_pins[j]; k++) {
				pb_graph_pin = &pb->pb_graph_node->clock_pins[j][k];
				if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster) && atom_net_id == pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id) {
					cluster_ctx.clb_nlist.create_pin(clock_port_id, (BitIndex)k, clb_net_id, PinType::SINK, pb_graph_pin->pin_count_in_cluster);
					return;
				}
			}
		}

	} else {
		VTR_ASSERT(atom_ctx.nlist.pin_type(atom_pin_id) == PinType::DRIVER);
		for (j = 0; j < num_output_ports; j++) {
			ClusterPortId output_port_id = cluster_ctx.clb_nlist.find_port(cluster_block_id, block_type->pb_type->ports[num_input_ports+j].name);
			for (k = 0; k < pb->pb_graph_node->num_output_pins[j]; k++) {
				pb_graph_pin = &pb->pb_graph_node->output_pins[j][k];
				if (pb->pb_route.count(pb_graph_pin->pin_count_in_cluster) && atom_net_id == pb->pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_id) {
					clb_net_id = atom_ctx.lookup.clb_net(atom_net_id);
					AtomPinId atom_net_driver = atom_ctx.nlist.net_driver(atom_net_id);
					bool driver_is_constant = atom_ctx.nlist.pin_is_constant(atom_net_driver);
					cluster_ctx.clb_nlist.create_pin(output_port_id, (BitIndex)k, clb_net_id, PinType::DRIVER, pb_graph_pin->pin_count_in_cluster, driver_is_constant);
					VTR_ASSERT(cluster_ctx.clb_nlist.net_is_constant(clb_net_id) == driver_is_constant);
					return;
				}
			}
		}
	}
	VTR_ASSERT(false);
}

void delete_cluster_net_of_atom_net(const AtomNetId& ) {
	return;
}