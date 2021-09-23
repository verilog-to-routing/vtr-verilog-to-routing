//============================================================================================
//				INCLUDES
//============================================================================================
#include "cleanup.h"


//============================================================================================
//			INTERNAL FUNCTION DECLARATIONS
//============================================================================================
//Cleanup Functions
void build_netlist (t_module* module, busvec* buses, s_hash** hash_table);
void init_nets (t_pin_def** pins, int num_pins, busvec* buses, struct s_hash** hash_table);
void set_net_assigns (t_assign** assignments, int num_assigns, busvec* buses, struct s_hash** hash_table);
void add_subckts (t_node** nodes, int num_nodes, busvec* buses, struct s_hash** hash_table);
void remove_one_lut_nodes ( busvec* buses, struct s_hash** hash_table, t_module* module );
void clean_netlist ( busvec* buses, struct s_hash** hash_table, t_node** nodes, int num_nodes );
void reassign_net_source (t_net* net);
void print_to_module ( t_module* module, busvec* buses, struct s_hash** hash_table );

netvec* get_bus_from_hash (struct s_hash** hash_table, char* temp_name, busvec* buses);

void verify_netlist ( t_node** nodes, int num_nodes, busvec* buses, struct s_hash** hash_table);
void print_all_nets ( busvec* buses, const char* filename );

bool is_feeder_onelut ( t_node* node );

//============================================================================================
//============================================================================================

void netlist_cleanup (t_module* module){

	struct s_hash** hash_table = alloc_hash_table();

	busvec buses;

	cout << "\t>> Building netlist...\n" ;

    //Creates:
    // 1) Fills in the busvec 'buses'
    // 2) Fills in  hash table mapping I/O pins and wires to buses index number
	build_netlist (module, &buses, hash_table);

	cout << "\t>> VQM Netlist contains " << buffer_count << " buffers.\n" ;
	cout << "\t>> VQM Netlist contains " << invert_count << " invertors.\n" ;
	cout << "\t>> VQM Netlist contains " << onelut_count << " one-LUTs.\n" ;

    //Verify that the initial netlist is ok
	verify_netlist ( module->array_of_nodes, module->number_of_nodes, &buses, hash_table );

	cout << "\t>> Removing One-LUTs" << "...\n";

	remove_one_lut_nodes ( &buses, hash_table, module );

	cout << "\t>> Removing buffered nets" << ((clean_mode == CL_BUFF)? "":" and inverted subckt inputs") << "...\n";

	clean_netlist ( &buses, hash_table, module->array_of_nodes, module->number_of_nodes );

	cout << "\t>> Removed " << buffers_elim << " buffers of " << buffer_count << ".\n" ;
	cout << "\t>> Removed " << inverts_elim << " invertors of " << invert_count << ".\n" ;
	cout << "\t>> Removed " << oneluts_elim << " one-LUTs of " << onelut_count << ".\n" ;

    //Verify that the final modified netlist is ok
	verify_netlist ( module->array_of_nodes, module->number_of_nodes, &buses, hash_table );

	print_to_module ( module, &buses, hash_table );

	free_hash_table(hash_table);

}

//============================================================================================
//============================================================================================

void build_netlist (t_module* module, busvec* buses, s_hash** hash_table){

	//Initialize all nets
	init_nets(module->array_of_pins, module->number_of_pins, buses, hash_table);

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to init.out\n" ;
	print_all_nets( buses, "init.out" );
#endif

	//Interpret the assignment array by making connections between the nets
	set_net_assigns (module->array_of_assignments, module->number_of_assignments, buses, hash_table);

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to assigned.out\n" ;
	print_all_nets( buses, "assigned.out" );
#endif

	//Introduce subcircuit (blackbox) connections to the nets
	add_subckts ( module->array_of_nodes, module->number_of_nodes, buses, hash_table );

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to subckts.out\n" ;
	print_all_nets( buses, "subckts.out" );
#endif

	cout << "\t\t>> Netlist initialization complete.\n" ;

}

//============================================================================================
//============================================================================================

void init_nets (t_pin_def** pins, int num_pins, busvec* buses, struct s_hash** hash_table){

	t_net temp_net;
	netvec temp_bus;

	for (int i = 0; i < num_pins; i++){
		temp_bus.clear();	//reset the temporary bus 

		insert_in_hash_table(hash_table, pins[i]->name, i);	//place the net in the hash table

		temp_net.pin = pins[i];	//each bus corresponds to a pin

		if (temp_net.pin->type == PIN_INPUT){	//set inpad drivers at initialization step
			temp_net.driver = CONST;
		} else {
			temp_net.driver = NODRIVE;
		}
		temp_net.source = NULL;
		temp_net.block_src = NULL;
		temp_net.num_children = 0;
		temp_net.bus_index = i;

		for (int j = 0; j < (max(temp_net.pin->left, temp_net.pin->right) + 1); j++){
			//iterate through and flatten the buses into separate nets
			temp_net.wire_index = j;	
			temp_bus.push_back(temp_net);
		}
			
		buses->push_back(temp_bus);
	}
}

//============================================================================================
//============================================================================================

void set_net_assigns (t_assign** assignments, int num_assigns, busvec* buses, struct s_hash** hash_table){

	t_net* temp_net;
	netvec* temp_bus;

	t_assign* temp_assign;

	for (int i = 0; i < num_assigns; i++) {
		temp_assign = assignments[i];

		temp_bus = get_bus_from_hash (hash_table, temp_assign->target->name, buses);

		if ((temp_assign->target_index >= 0)&&(temp_assign->target->indexed)){
			VTR_ASSERT(temp_bus->size() > (unsigned int)temp_assign->target_index);
			temp_net = &(temp_bus->at(temp_assign->target_index));
		} else {

#ifndef VQM_BUSES
			VTR_ASSERT(temp_bus->size() == 1);	//VQM Bus assignments don't appear; must be 1-bit wide
#endif
			temp_net = &(temp_bus->at(0));
		}

		if (temp_assign->source == NULL){
			temp_net->driver = CONST;
			temp_net->source = NULL;
		} else {

			temp_bus = get_bus_from_hash(hash_table, temp_assign->source->name, buses);

			if ((temp_assign->source_index >= 0)&&(temp_assign->source->indexed)){
				VTR_ASSERT(temp_bus->size() > (unsigned int)temp_assign->source_index);
				temp_net->source = &(temp_bus->at(temp_assign->source_index));
			} else {
				VTR_ASSERT(temp_bus->size() == 1);
				temp_net->source = &(temp_bus->at(0));
			}

			if (temp_assign->inversion){
				temp_net->driver = INVERT;
				invert_count++;
			} else {
				temp_net->driver = BUFFER;
				buffer_count++;
			}

			temp_net = (t_net*)temp_net->source;
			temp_net->num_children++;	//increase the child count of the source
		}
	}


}

//============================================================================================
//============================================================================================

void add_subckts (t_node** nodes, int num_nodes, busvec* buses, struct s_hash** hash_table){

	t_net* temp_net;
	netvec* temp_bus;

	t_node* temp_node;
	t_node_port_association* temp_port;

	onelut_count = 0;

	for (int i = 0; i < num_nodes; i++){
		temp_node = nodes[i];
		if(is_feeder_onelut(temp_node)){
			onelut_count++;
		}
		for (int j = 0; j < temp_node->number_of_ports; j++){
			temp_port = temp_node->array_of_ports[j];

			temp_bus = get_bus_from_hash (hash_table, temp_port->associated_net->name, buses);

			VTR_ASSERT( !temp_bus->empty() );
			VTR_ASSERT( temp_port->wire_index >= 0 );

			temp_net = &(temp_bus->at(temp_port->wire_index));

			if ((temp_net->source == NULL)&&(temp_net->driver != CONST)){
				//this port must be the source
				temp_net->source = temp_port;
				temp_net->block_src = temp_node;
				temp_net->driver = BLACKBOX;
			} else {
				//this port must be a sink
				temp_net->num_children++;
			}
		}
	}
}

//============================================================================================
//============================================================================================

void remove_one_lut_nodes ( busvec* buses, struct s_hash** hash_table, t_module* module ){
/*
        Quartus fitter may have introduced some one-LUTs in the post-fit netlist that makes it harder for VPR to place and route.
        Generally, these one-LUTs are inserted by the Quartus router in order to pass a signal through a LUT to the FF in the same
        BLE. For Stratix IV, the names of these one-LUTs all end with the substring "feeder". This function serves to remove the
        feeder one LUTs from the netlist, if they exist, before converting it into the BLIF file format.

	Go through all nodes, if a node's source net is driven by a one-LUT-type node and if this source net only has one
        child (the node itself):

	  1. If the one-LUT has an input and an output, the one-LUT acts as either a buffer LUT or as an inverter, but we do not
             check as we only care about structure in this converter, not logical functionality.
             Re-associate the node's input port with the source net of the one-LUT, then remove the one-LUT and the node's original
             source net.

             -----                -------                 -----
             | X |---> net m ---> | LUT | ---> net n ---> | Y |
             -----                -------                 -----

             becomes

             -----                -----
             | X |---> net m ---> | Y |
             -----                -----

	  2. If the one-LUT has just an output (feeds VCC downstream):
             Re-associate the node with the VCC net, then remove the one-LUT and the node's original source net.

             -------                 -----
             | LUT | ---> net m ---> | Y |
             -------                 -----

             becomes

             -------                    -----
             | VCC |---> net v -------> | Y |
             -------            |       -----
                                ---->
                                |
                                ---->
	Change Log:
		- Srivatsan Srinivasan, September 2021:
			- removed the function parameters "original_num_nodes" and "nodes". These values can be from the "module" parameter and are now assigned internally within this function.
		- Srivatsan Srinivasan, August 2021:
			- Moved the incrementing of the "oneluts_elim" variable to this fuction from the "remove_node" function. The purpose of this change was to localise any vairable attached to removing one lut nodes within this function. Additionally, now the "remove_node" function is generalized and is not limited to be only used by "remove_one_lut_nodes".
*/
	oneluts_elim = 0;

	// parameters related to the module
	int original_num_nodes = module->number_of_nodes;
	t_node** nodes = module->array_of_nodes;

	t_node* temp_node;
	t_node_port_association* temp_port;
	netvec* temp_bus;
	t_net* temp_net;

	t_node* source_node;
	t_node_port_association* source_port;
	t_node_port_association* prev_port;
	netvec* prev_bus;
	t_net* prev_net;

	netvec* vcc_bus = get_bus_from_hash (hash_table, const_cast<char*>("vcc"), buses);
	VTR_ASSERT(vcc_bus != NULL);
	t_net* vcc_net = &(vcc_bus->at(0));   //Find any VCC net

	for (int i = 0; i < original_num_nodes; i++){
		temp_node = nodes[i];
		if (temp_node == NULL) {   //Node was deleted during a previous iteration
			continue;
		}
		for (int j = 0; j < temp_node->number_of_ports; j++){
			temp_port = temp_node->array_of_ports[j];
			temp_bus = get_bus_from_hash (hash_table, temp_port->associated_net->name, buses);
			VTR_ASSERT((unsigned int)temp_port->wire_index < temp_bus->size());
			temp_net = &(temp_bus->at(temp_port->wire_index));

			if (temp_port != (t_node_port_association*)temp_net->source){
				//Must be an input port
				if (temp_net->driver == BLACKBOX && is_feeder_onelut(temp_net->block_src) && temp_net->num_children == 1){
					source_port = (t_node_port_association*)temp_net->source;   //The output port of the one-LUT
					source_node = temp_net->block_src;   //The one-LUT

					//Re-associate temp_port with the appropriate net
					if(source_node->number_of_ports == 2){
						//For one-LUT with an input and an output, find the net before the one_LUT and associate temp_port with that net instead
						VTR_ASSERT(source_node->number_of_ports == 2);
						for (int k = 0; k < source_node->number_of_ports; k++){
							prev_port = source_node->array_of_ports[k];
							if(prev_port != source_port) {
								//The input port of the one-LUT
								prev_bus = get_bus_from_hash (hash_table, prev_port->associated_net->name, buses);
								VTR_ASSERT((unsigned int)prev_port->wire_index < prev_bus->size());
								prev_net = &(prev_bus->at(prev_port->wire_index)); //Net associated with the input port
							}
						}
						temp_port->associated_net = prev_net->pin;
						temp_port->wire_index = prev_net->wire_index;
					} else {
						//For one-LUT with just an output, associate temp_port with VCC instead
						VTR_ASSERT(source_node->number_of_ports == 1); //If is_feeder_onelut==true, there are only 1 or 2 ports
						VTR_ASSERT(vcc_net != NULL); //Should have a VCC
						temp_port->associated_net = vcc_net->pin;
						temp_port->wire_index = vcc_net->wire_index;
						vcc_net->num_children++;
					}

					//Remove temp_net
					temp_net->num_children--;
					temp_net->source = NULL;
					temp_net->driver = NODRIVE;

					//Free the LUT
					remove_node(source_node, nodes, original_num_nodes);
					
					// increment the count of the number of one lut nodes we eliminiated
					oneluts_elim++;

				}
			}
		}
	}

	// fix the module nodes list (node array) by filling in the gaps where the one-lut nodes have been removed above 
	reorganize_module_node_list(module);

    //Reduce run-time by only verifying at the end
	//verify_netlist (nodes, module->number_of_nodes, buses, hash_table);

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to all_buff.out\n" ;
	print_all_nets(buses, "all_buff.out");
#endif
}

//============================================================================================
//============================================================================================

void clean_netlist ( busvec* buses, struct s_hash** hash_table, t_node** nodes, int num_nodes ){
	netvec* temp_bus;

	t_net* temp_net;
	t_net* root_net;

	t_node_port_association* temp_port;
	t_node* temp_node;
	
	buffers_elim = 0;
	inverts_elim = 0;
	
	t_pin_def* ref_pin;

	int out_sink_elim = 0;

	if (buffd_outs == T_FALSE){
		//Step 1: Reassign output pin sinks 
		for (int i = 0; (unsigned int)i < buses->size(); i++){
			temp_bus = &(buses->at(i));
			VTR_ASSERT(temp_bus->size() > 0);

			ref_pin = temp_bus->at(0).pin;

			if (ref_pin->type != PIN_OUTPUT){
				continue;
			} else {
				for (int j = 0; (unsigned int)j < temp_bus->size(); j++){
					temp_net = &(temp_bus->at(j));
					reassign_net_source (temp_net);
				}
			}
		}

		out_sink_elim = (buffers_elim+inverts_elim);
		cout << "\t\t>> Removed " << out_sink_elim << " nets from output sinks.\n" ;

        //Reduce run time by only verifying at the end
		//verify_netlist (nodes, num_nodes, buses, hash_table);
	}

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to out_buff.out\n" ;
	print_all_nets(buses, "out_buff.out");
#endif

	//Reassign blackbox (node) sinks
	for (int i = 0; i < num_nodes; i++){
		temp_node = nodes[i];
		for (int j = 0; j < temp_node->number_of_ports; j++){
			temp_port = temp_node->array_of_ports[j];
			temp_bus = get_bus_from_hash (hash_table, temp_port->associated_net->name, buses);
			VTR_ASSERT((unsigned int)temp_port->wire_index < temp_bus->size());
			temp_net = &(temp_bus->at(temp_port->wire_index));

			if (temp_port != (t_node_port_association*)temp_net->source){
				reassign_net_source (temp_net);
				if ((temp_net->driver == BUFFER)
					||( (temp_net->driver == INVERT) && (clean_mode == CL_ALL) && 
						( (lut_mode == VQM)||( !is_lut(temp_node) ) ) )){

					root_net = (t_net*)temp_net->source;
					temp_port->associated_net = root_net->pin;
					temp_port->wire_index = root_net->wire_index;
					temp_net->num_children--;

					if (temp_net->num_children > 0){
						root_net->num_children++;
					} else {
						if (temp_net->driver == BUFFER){
							buffers_elim++;
						} else {
							inverts_elim++;
						}
						temp_net->source = NULL;
						temp_net->driver = NODRIVE;
					}
				}
			}
		}
	}

	if (buffd_outs == T_FALSE){
		cout << "\t\t>> Removed " << ((buffers_elim+inverts_elim) - out_sink_elim) << " nets from subcircuit sinks.\n" ;
	} else {
		cout << "\t\t>> Removed " << (buffers_elim + inverts_elim) << " nets from subcircuit sinks.\n";
	}

    //Reduce run-time by only verifying at the end
	//verify_netlist (nodes, num_nodes, buses, hash_table);

#ifdef CLEAN_DEBUG
	cout << "\t\t>> Dumping to all_buff.out\n" ;
	print_all_nets(buses, "all_buff.out");
#endif
}

//============================================================================================
//============================================================================================

void reassign_net_source (t_net* net){
	t_net* net_source;
	t_node_port_association* port_source;

	if ((net->driver == BUFFER)||(net->driver == INVERT)){
		net_source = (t_net*)net->source;
		reassign_net_source(net_source);
		switch (net_source->driver){
			case INVERT: 
				if (net->driver == INVERT){
					net->driver = BUFFER;
				} else {
					net->driver = INVERT;
				}
                //fallthrough
			case BUFFER:
				net->source = net_source->source;
				net_source->num_children--;
				if (net_source->num_children > 0){
					((t_net*)net->source)->num_children++;
				} else {
					net_source->source = NULL;
					net_source->driver = NODRIVE;
					buffers_elim++;
				}
				break;
			case BLACKBOX:
				if ((net_source->num_children == 1)&&(net->driver == BUFFER)){
					//nets driven by blackboxes must have no fanout to be reduced. Otherwise, they are kept.
					port_source = (t_node_port_association*)net_source->source;

					port_source->associated_net = net->pin;
					port_source->wire_index = net->wire_index;

					net->source = net_source->source;	//transfer the port
					net->block_src = net_source->block_src;	//transfer the block
					net->driver = BLACKBOX;	//reset the driver
					
					net_source->num_children--;
					net_source->source = NULL;
					net_source->driver = NODRIVE;
					buffers_elim++;
				}
				break;
			default:
				break;
		}
	}
}

//============================================================================================
//============================================================================================

void print_to_module ( t_module* module, busvec* buses, struct s_hash** hash_table ){

	t_net* target_net;
	t_net* source_net;
	netvec* temp_bus;

	VTR_ASSERT(buses->size() == (unsigned int)module->number_of_pins);

	t_assign* temp_assign;
	//Adjust assignments, freeing ones that target dead nets
	for (int i = 0; i < module->number_of_assignments; i++){
		temp_assign = module->array_of_assignments[i];
		temp_bus = get_bus_from_hash (hash_table, temp_assign->target->name, buses);

		if (temp_assign->target_index >= 0){
			VTR_ASSERT(temp_bus->size() > (unsigned int)temp_assign->target_index);
			target_net = &(temp_bus->at(temp_assign->target_index));
		} else {
			VTR_ASSERT(temp_bus->size() == 1);
			target_net = &(temp_bus->at(0));
		}

		if (temp_assign->source == NULL){
			VTR_ASSERT(target_net->driver == CONST);
			continue;
		}

		if (		  ((target_net->driver == BUFFER)||(target_net->driver == INVERT))
				&&((target_net->pin->type == PIN_OUTPUT)||(target_net->num_children > 0))
				&&(target_net->source != NULL)){
			//target is not a dead net
			temp_bus = get_bus_from_hash (hash_table, temp_assign->source->name, buses);
			if (temp_assign->source_index >= 0){
				VTR_ASSERT(temp_bus->size() > (unsigned int)temp_assign->source_index);
				source_net = &(temp_bus->at(temp_assign->source_index));
			} else {
				VTR_ASSERT(temp_bus->size() == 1);
				source_net = &(temp_bus->at(0));
			}
			if (source_net != (t_net*)target_net->source){
				//target_net has been reassigned
				source_net = (t_net*)target_net->source;
				temp_assign->source = source_net->pin;

				if (temp_assign->source->indexed){
					temp_assign->source_index = source_net->wire_index;
				} else {
					VTR_ASSERT(temp_assign->source->left == temp_assign->source->right);
					temp_assign->source_index = -1;
				}

				if (target_net->driver == INVERT){
					temp_assign->inversion = T_TRUE;
				} else if (target_net->driver == BUFFER){
					temp_assign->inversion = T_FALSE;
				} else {
					//should never get here
					cout << "ERROR: Illegal assignment change.\n" ;
					exit(1);
				}
			}
		} else {
			//target is a dead net
			free(module->array_of_assignments[i]);
			module->array_of_assignments[i] = NULL;
		}
	}

	/*NOTE: 
		module->array_of_nodes has already been altered; the t_node_port_associations have been changed
		to reflect new subcircuit port connectivity.

		module->array_of_pins doesn't need to be altered; inputs and outputs should not have been touched
		in the cleanup process. Wires still exist in the array_of_pins that correspond to dead nets but they're
		not printed in BLIF explicitly. Wires would only be printed as a result of being present in an 
		assignment or port association; both of which have been fixed. 
	*/

}

//============================================================================================
//============================================================================================

netvec* get_bus_from_hash (struct s_hash** hash_table, char* bus_name, busvec* buses){
	s_hash* hash_entry;

	hash_entry = get_hash_entry (hash_table, bus_name);
	VTR_ASSERT(hash_entry != NULL);
	VTR_ASSERT((unsigned int)hash_entry->index < buses->size());

	return &(buses->at(hash_entry->index));
}

//============================================================================================
//============================================================================================

void verify_netlist ( t_node** nodes, int num_nodes, busvec* buses, struct s_hash** hash_table){

	netvec* temp_bus;

	t_net* temp_net;

	t_net* net_source;
	t_node_port_association* port_source;
	t_node* temp_node;
	
	t_pin_def* ref_pin;

	s_hash* hash_entry;

	cout << "\t>> Verifying netlist...\n" ;

	//Step 0: Construct child_count "matrix" corresponding to the net indeces. 
	//	The children of each net will be counted as the netlist is verified, 
	//	then compared against the number of children stored in the net.
	vector < vector <int> > child_count;
	child_count.resize(buses->size());
	for (unsigned int i = 0; i < buses->size(); i++){
		child_count[i].resize(buses->at(i).size());
	}

	//Step 1: Verify each source is valid.
	for (int i = 0; (unsigned int)i < buses->size(); i++){
		temp_bus = &(buses->at(i));
		VTR_ASSERT(temp_bus->size() > 0);

		ref_pin = temp_bus->at(0).pin;

		hash_entry = get_hash_entry (hash_table, ref_pin->name);
		VTR_ASSERT(hash_entry != NULL);
		VTR_ASSERT(hash_entry->index == i);
		
		for (int j = 0; (unsigned int)j < temp_bus->size(); j++){
			temp_net = &(temp_bus->at(j));

			VTR_ASSERT((temp_net->bus_index == i)&&(temp_net->wire_index == j));	//indeces must line up
			VTR_ASSERT(ref_pin == temp_net->pin);	//all nets in a common bus share a pin

			if (temp_net->driver == CONST){
				continue; //no way to discern incorrect connectivity from a constant net
			} else if (temp_net->source == NULL){
				VTR_ASSERT((temp_net->num_children == 0)&&(temp_net->driver == NODRIVE));
				//NOTE: These nets will be removed later for having no children.
			} else if (temp_net->driver != BLACKBOX){
				//being driven by a BUFFER or INVERT. (Both t_net* structs.)
				net_source = (t_net*)temp_net->source;

                //PIN_INOUT should have been removed earlier
				VTR_ASSERT((net_source->pin->type == PIN_INPUT)||(net_source->pin->type == PIN_WIRE));

				VTR_ASSERT((unsigned int)net_source->bus_index < child_count.size());
				VTR_ASSERT((unsigned int)net_source->wire_index < child_count[net_source->bus_index].size());
				child_count[net_source->bus_index][net_source->wire_index]++;
			} else {
				port_source = (t_node_port_association*)temp_net->source;
				VTR_ASSERT(temp_net->block_src != NULL);
				VTR_ASSERT(port_source->associated_net == ref_pin);
				VTR_ASSERT(port_source->wire_index == temp_net->wire_index);
			}
		}
	}

	//Step 2: Increase the child count for nets going into subcircuits.
	for (int i = 0; i < num_nodes; i++){
		temp_node = nodes[i];
		for (int j = 0; j < temp_node->number_of_ports; j++){
			port_source = temp_node->array_of_ports[j];

			VTR_ASSERT(port_source != NULL);
			VTR_ASSERT(port_source->associated_net != NULL);
	
			//Find the net connected to the port
			temp_bus = get_bus_from_hash(hash_table, port_source->associated_net->name, buses);

			if (port_source->wire_index >= 0){
				VTR_ASSERT((unsigned int)port_source->wire_index < temp_bus->size());
				temp_net = &(temp_bus->at(port_source->wire_index));
			} else {
				VTR_ASSERT(temp_bus->size() == 1);
				temp_net = &(temp_bus->at(0));
			}			

			if ((t_node_port_association*)temp_net->source == port_source){
				//if the port is the source, make sure the block_src is correct
				VTR_ASSERT(temp_net->block_src == temp_node);
			} else {
				//if this isn't the source, it must be a sink, so increase its child_count
				child_count[temp_net->bus_index][temp_net->wire_index]++;
			}
		}
	}

	//Step 3: Verify child_counts across all nets
	for (int i = 0; (unsigned int)i < buses->size(); i++){
		temp_bus = &(buses->at(i));
		VTR_ASSERT(temp_bus->size() > 0);
		for (int j = 0; (unsigned int)j < temp_bus->size(); j++){
			temp_net = &(temp_bus->at(j));
			VTR_ASSERT((temp_net->bus_index == i)&&(temp_net->wire_index == j));	//indeces must line up
			VTR_ASSERT(child_count[i][j] == temp_net->num_children);
		}
	}
}

//============================================================================================
//============================================================================================

void print_all_nets ( busvec* buses, const char* filename ){
	
	netvec* temp_bus;
	t_net* temp_net;

	t_net* net_source;
	t_node_port_association* port_source;

	ofstream outfile ( filename );

	VTR_ASSERT( outfile.is_open() );

	outfile << "Number of buses: " << buses->size() << "\n\n" ;

	for ( unsigned int i = 0; i < buses->size(); i++ ){
		
		temp_bus = &(buses->at(i));
		outfile << "Bus " << i << endl;
		outfile << "\tNumber of Nets: " << temp_bus->size() << endl;
			
		for (unsigned int j = 0; j < temp_bus->size(); j++){
			temp_net = &(temp_bus->at(j));
			outfile << "\tNet " << j << ": " << temp_net->pin->name << "\n\t\tWire Index: " << temp_net->wire_index << endl;
			outfile << "\t\tNumber of children: " << temp_net->num_children << endl;
			outfile << "\t\tSource: ";
			if (temp_net->driver == CONST){
				outfile << "Constant " << ((temp_net->pin->type == PIN_INPUT)? "(inpad), ":", ") << ((temp_net->source == NULL)?"NULL\n":"Assigned\n");
			} else if (temp_net->source == NULL){
				outfile << "NULL " << "\n\t\tDriver Type: " << temp_net->driver << endl ;
			} else if ((temp_net->driver == BUFFER)||(temp_net->driver == INVERT)){
				net_source = (t_net*)temp_net->source;
				outfile << "Net (" << temp_net->driver << "): " << net_source->pin->name << "[" << net_source->wire_index << "]\n" ;
				outfile << "\t\t\tInverted: " << (temp_net->driver == INVERT) << endl;
			} else {
				VTR_ASSERT((temp_net->driver == BLACKBOX)&&(temp_net->block_src != NULL));
				port_source = (t_node_port_association*)temp_net->source;
				outfile << "Block\t" << temp_net->block_src->name << endl;
				outfile << "\t\t\tType: " << temp_net->block_src->type << " Port: " << port_source->port_name << "[" << port_source->port_index << "]\n" ;
			}
		}
	}

	outfile.close();
}

//============================================================================================
//============================================================================================

bool is_feeder_onelut ( t_node* node ) {
	if(node == NULL) return false;
	
	//Hardcoded for Stratix IV
	string node_name = node->name;
	string node_name_ending;
	if (node_name.length() >= 8){
		node_name_ending = node_name.substr(node_name.length()-8);
	} else {
		node_name_ending = node_name;
	}

#ifdef CLEAN_DEBUG
	cout << "\t\t Node Type: " << node->type << "\t" << "Node Name Ending: " << node_name_ending << "\t" << "Num of Ports: " << node->number_of_ports <<"\n";
#endif

	//Only LUTs with 1 port (1 output port) or 2 ports (1 input and 1 output) are considered one-luts
	if (node->number_of_ports == 1 || node->number_of_ports == 2){
		//Only stratixiv_lcell_comb one-LUTs that end in "feeder" can be removed at this stage
		if (node->type == string("stratixiv_lcell_comb") && node_name_ending == string("feeder_I")) {
			return true;
		}
	}

    return false;
}

//============================================================================================
//============================================================================================

void remove_node ( t_node* node, t_node** nodes, int original_num_nodes ) {
	//Free node and assign it to NULL on the spot
	//Array will be re-organized to fill in the gaps later
	/*
		Change Log:
		- Srivatsan Srinivasan, August 2021:
			- Orirignally, the "oneluts_elim" variable (which is used to keep track of the number of one lut nodes removed from the node list) was incremented here. This incrementation has been moved to the "remove_one_lut_nodes" function.
			- This purpose of this change is that now this function is not limited to just be used by "remove_one_lut_nodes".
	*/

	VTR_ASSERT(node != NULL);
	VTR_ASSERT(nodes != NULL);

#ifdef CLEAN_DEBUG
	cout << "\t\t\t Removing " << node->name << "\n";
#endif
	bool found = false;

	for (int i = 0; i < original_num_nodes; i++){
		if(nodes[i] == node){
			free_node( (void*)nodes[i] );
			nodes[i] = NULL;
			found = true;
			break;
		}
	}
	
	VTR_ASSERT(found);
}

//============================================================================================
//============================================================================================

void reorganize_module_node_list(t_module* module)
{
	/*
        The purpose of this function is to regorganize the module node list (node array) by filling in gaps with the last available elements in the array to save CPU time.

		The module provided to this function is expected to have nodes deleted within its nodes list (array). This essentially creates gaps in the array. So we fill in the gaps of this array and ensure it is a continuous list of nodes.

		Please refer to the example below:

		Inital Node array:
             ------      ------                  ------      ------
            |LUT 1| --> |LUT 2| -->         --> |LUT 3| --> |LUT 4|
            ------      ------                  ------      ------

		After reorganization and filling gaps:

             ------      ------      ------      ------
            |LUT 1| --> |LUT 2| --> |LUT 4| --> |LUT 3| -->
            ------      ------      ------      ------

		The function fills in the gaps with the last available node in the list (as shown in the example above).

	Change Log:
		- Srivatsan Srinivasan, August 2021:
			- created this function to reorganize node arrays with gaps inside of them.
			- Initially the feature provided by this function was embedded indide the "remove_one_lut_nodes" function. By creating a seperate function, we are now not restricted to only removing one-lut nodes. 
			- Now we can remove any types of nodes and then run this function to reorganize the node array.  
*/
	// assign module related parameters
	int original_number_of_nodes = module->number_of_nodes;
	t_node** module_node_list = module->array_of_nodes;

	int curr_node_index = 0;
	int replacement_node_index = original_number_of_nodes - 1;
	while (curr_node_index <= replacement_node_index) {
		if (module_node_list[curr_node_index] == NULL) {
			if (module_node_list[replacement_node_index] != NULL) {
				//Replace gap with node
				module_node_list[curr_node_index] = module_node_list[replacement_node_index];
				module_node_list[replacement_node_index] = NULL;
				curr_node_index++;
			}
			replacement_node_index--;
		} else {
			curr_node_index++;
		}
	}
	
	//Update array bounds
	// curr_node_index keeps track of the number of non-removed nodes within the node array. 
	// So we can use it directly to indicate the new size of the array.
	module -> number_of_nodes = curr_node_index;

	return;
}

//============================================================================================
//============================================================================================