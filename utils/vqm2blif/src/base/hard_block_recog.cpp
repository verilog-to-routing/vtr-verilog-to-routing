// add higher level comments here



#include "hard_block_recog.h"


void add_hard_blocks_to_netlist(t_module* main_module, t_arch* main_arch, std::vector<std::string>* list_hard_block_type_names, std::string arch_file_name, std::string vqm_file_name)
{
    t_hard_block_recog module_hard_block_node_refs_and_info;

    try
    {
        initialize_hard_block_models(main_arch, list_hard_block_type_names, &module_hard_block_node_refs_and_info);
    }
    catch(const vtr::VtrError& error)
    {
        throw vtr::VtrError((std::string)error.what() + "The FPGA architecture is described in " + arch_file_name + ".");
    }

    try
    {
        process_module_nodes_and_create_hard_blocks(main_module, list_hard_block_type_names, &module_hard_block_node_refs_and_info);
    }
    catch(const vtr::VtrError& error)
    {
        throw vtr::VtrError((std::string)error.what() + "The original netlist is described in " + vqm_file_name + ".");
    }




    // need to delete all the dynamic memory used to store 
    // all the hard block port information
    delete_hard_block_port_info(&(module_hard_block_node_refs_and_info.hard_block_type_name_to_port_info));
    
    return;
}

void initialize_hard_block_models(t_arch* main_arch, std::vector<std::string>* hard_block_type_names, t_hard_block_recog* storage_of_hard_block_info)
{
    t_model* hard_block_model = NULL;
    std::vector<std::string>::iterator hard_block_type_name_traverser;
    bool single_hard_block_init_result = false;

    for (hard_block_type_name_traverser = hard_block_type_names->begin(); hard_block_type_name_traverser != hard_block_type_names->end(); hard_block_type_name_traverser++)
    {
        // function already made
        hard_block_model = find_arch_model_by_name(*hard_block_type_name_traverser, main_arch->models);

        if (hard_block_model == NULL)
        {         
            throw vtr::VtrError("The provided hard block model: '" + *hard_block_type_name_traverser + "' was not found within the corresponding FPGA architecture.");
        }
        else
        { 
            single_hard_block_init_result =  create_and_initialize_all_hard_block_ports(hard_block_model, storage_of_hard_block_info);

            if (!single_hard_block_init_result)
            {
                throw vtr::VtrError("Hard block model: '" + *hard_block_type_name_traverser + "' found in the architecture has no input/output ports.");
            }
        }

    }

    return;

}

void process_module_nodes_and_create_hard_blocks(t_module* main_module, std::vector<std::string>* hard_block_type_name_list, t_hard_block_recog* module_hard_block_node_refs_and_info)
{   
    // represents a block in the netlist
    // refer to 'vqm_dll.h' for more info on t_node
    t_node* curr_module_node = NULL;

    // create a new t_array_ref (refer to 'vqm_dll.h') structure so that we can append new hard block instances we create to the original node array
    t_array_ref* node_list_with_hard_blocks = create_t_array_ref_from_array((void**)(main_module->array_of_nodes), main_module->number_of_nodes);

    std::string curr_module_node_type = "";
    std::string curr_node_name = "";
    
    int number_of_module_nodes = main_module->number_of_nodes; // before any hard blocks are added

    t_parsed_hard_block_port_info curr_module_node_info;

    int curr_hard_block_instance_index = 0;

    /* iterate through every node in the module and create a new node to represent each and every hard block we identify within the netlist.*/
    for (int i = 0; i < number_of_module_nodes; i++)
    {

        curr_module_node = (t_node*)node_list_with_hard_blocks->pointer[i];
        curr_module_node_type.assign(curr_module_node->type);

        // hard block ports are only represented in nodes that are either a LUT or flip flop block
        if ((curr_module_node_type.compare(LUT_TYPE) == 0) || (curr_module_node_type.compare(DFF_TYPE) == 0))
        {

            curr_module_node_info = extract_hard_block_port_info_from_module_node(curr_module_node, hard_block_type_name_list);
            
            // check to see that the current node represents a hard block port
            // a hard block instance name would not have bee found if it wasn't a hard block port
            if (!curr_module_node_info.hard_block_name.empty())
            {
                
                // if we are here, the current node is a LUT or DFF node that represents a hard block instance port //

                /* referring to the diagram at the top, if the node is a lut that represents an output port, its connected net is an internal connection, which is not a legal netlist connection, so we cannot process the node any further. Therefore we only process nodes that are LUTs representing input ports or DFFs. */
                if (is_hard_block_port_legal(curr_module_node))
                {

                    // get the index to the current hard block instance we need to work with
                    curr_hard_block_instance_index = find_hard_block_instance(module_hard_block_node_refs_and_info, &curr_module_node_info);

                    // check to see whether the hard block instance the current node belongs to exists (remember each node here represents a hard block port)
                    if (curr_hard_block_instance_index == HARD_BLOCK_INSTANCE_DOES_NOT_EXIST)
                    {
                        // we need to create a new hard block instance, since it does not exists for the port the current node represents
                        curr_hard_block_instance_index = create_new_hard_block_instance(node_list_with_hard_blocks, module_hard_block_node_refs_and_info, &curr_module_node_info);
                    }
                    
                    /* the remaining steps below assign the net currently connected to the node being processed (a flip-flop or a LUT) to the hard block instance port the current node represents. The correspponding hard block instance and the specific port were found previously.*/
                    assign_net_to_hard_block_instance_port(curr_module_node, &curr_module_node_info, module_hard_block_node_refs_and_info, curr_hard_block_instance_index);
                }

                /* in this if clause, the current node represents a hard block port, so we cannot have this node be written to the output netlist, so we need to add it to the nodes to delete list */
                module_hard_block_node_refs_and_info->luts_dffeas_nodes_to_remove.push_back(curr_module_node);
                
            }
        }
    }

    
}

bool create_and_initialize_all_hard_block_ports(t_model* hard_block_arch_model, t_hard_block_recog* storage_of_hard_block_info)
{
    int hard_block_port_index = 0;
    std::string hard_block_arch_model_name = hard_block_arch_model->name;
    bool result = true;

    t_model_ports* input_ports = hard_block_arch_model->inputs;
    t_model_ports* output_ports = hard_block_arch_model->outputs;

    //initialize a hard block node port array
    create_hard_block_port_info_structure(storage_of_hard_block_info,hard_block_arch_model_name);

    // handle input ports
    hard_block_port_index += extract_and_store_hard_block_model_ports(storage_of_hard_block_info, input_ports, hard_block_arch_model_name,hard_block_port_index, INPUT_PORTS);

    // handle output ports
    hard_block_port_index += extract_and_store_hard_block_model_ports(storage_of_hard_block_info, output_ports, hard_block_arch_model_name,hard_block_port_index, OUTPUT_PORTS);

    if (hard_block_port_index == HARD_BLOCK_WITH_NO_PORTS)
    {

        result = false;

    }

    return result; 
}

void create_hard_block_port_info_structure(t_hard_block_recog* storage_of_hard_block_info, std::string hard_block_type_name)
{
    t_hard_block_port_info curr_hard_block_port_storage;

    (curr_hard_block_port_storage.hard_block_ports).pointer = NULL;
    (curr_hard_block_port_storage.hard_block_ports).allocated_size = 0;
    (curr_hard_block_port_storage.hard_block_ports).array_size = 0;
 
    storage_of_hard_block_info->hard_block_type_name_to_port_info.insert({hard_block_type_name,curr_hard_block_port_storage});

    return;

}

int extract_and_store_hard_block_model_ports(t_hard_block_recog* storage_of_hard_block_info, t_model_ports* curr_hard_block_model_port, std::string curr_hard_block_type_name,int port_index, std::string port_type)
{
    t_array_ref* equivalent_hard_block_node_port_array = NULL;
    int starting_port_index = port_index;

    while (curr_hard_block_model_port != NULL)
    {
        equivalent_hard_block_node_port_array = convert_hard_block_model_port_to_hard_block_node_port(curr_hard_block_model_port);

        store_hard_block_port_info(storage_of_hard_block_info, curr_hard_block_type_name, curr_hard_block_model_port->name, &equivalent_hard_block_node_port_array, &port_index);

        curr_hard_block_model_port = curr_hard_block_model_port->next;

    }

    if (starting_port_index == port_index)
    {
        VTR_LOG_WARN("Model '%s' found in the architecture file does not have %s ports\n", curr_hard_block_type_name.c_str(), port_type.c_str());
    }

    return port_index;
}

t_array_ref* convert_hard_block_model_port_to_hard_block_node_port(t_model_ports* hard_block_model_port)
{
    t_node_port_association* curr_hard_block_node_port = NULL;
    t_array_ref* port_array = NULL;
    char* curr_hard_block_model_port_name = hard_block_model_port->name;
    int port_size = hard_block_model_port->size;

    //create memory to store the port array
    port_array = create_and_initialize_t_array_ref_struct();
    
    for (int port_index = 0; port_index < port_size; port_index++)
    {
        // hard blocks will not have indexed wire assignments 
        // doesnt do anything different I think
        curr_hard_block_node_port = create_unconnected_node_port_association(curr_hard_block_model_port_name, port_index, WIRE_NOT_INDEXED);

        // store the newly created specific port index within the entire port
        // array
        append_array_element((intptr_t)curr_hard_block_node_port, port_array);
    }

    return port_array;

}

t_node_port_association* create_unconnected_node_port_association(char *port_name, int port_index, int wire_index)
{
    t_node_port_association* curr_hard_block_node_port = NULL;

    curr_hard_block_node_port = (t_node_port_association*)vtr::malloc(sizeof(t_node_port_association));
    
    curr_hard_block_node_port->port_name = (char*)vtr::malloc(sizeof(char) * (strlen(port_name) + 1));
    strcpy(curr_hard_block_node_port->port_name, port_name);

    curr_hard_block_node_port->port_index = port_index;
    curr_hard_block_node_port->associated_net = NULL;
    curr_hard_block_node_port->wire_index = wire_index;

    return curr_hard_block_node_port;
}

// need the hard block name itself
void store_hard_block_port_info(t_hard_block_recog* storage_of_hard_block_port_info, std::string curr_hard_block_type_name,std::string curr_port_name, t_array_ref** curr_port_array, int* port_index)
{
       
    std::unordered_map<std::string,t_hard_block_port_info>::iterator curr_port_info = ((storage_of_hard_block_port_info->hard_block_type_name_to_port_info).find(curr_hard_block_type_name));

    copy_array_ref(*curr_port_array, &(curr_port_info->second).hard_block_ports);
    
    // insert the port name to the current index
    (curr_port_info->second).port_name_to_port_start_index.insert(std::pair<std::string,int>(curr_port_name, *port_index));

    *port_index += (*curr_port_array)->array_size;

    // store the current port size
    (curr_port_info->second).port_name_to_port_size.insert(std::pair<std::string,int>(curr_port_name, (*curr_port_array)->array_size));

    vtr::free((*curr_port_array)->pointer);
    vtr::free(*curr_port_array);

    *curr_port_array = NULL;

    return;
}

void copy_array_ref(t_array_ref* array_ref_orig, t_array_ref* array_ref_copy)
{
    int array_size = array_ref_orig->array_size;

    for (int index = 0; index < array_size; index++)
    {
        append_array_element((intptr_t)array_ref_orig->pointer[index], array_ref_copy);
    }

    return;
}

t_array_ref* create_and_initialize_t_array_ref_struct(void)
{
    t_array_ref* empty_array = NULL;

    empty_array = (t_array_ref*)vtr::malloc(sizeof(t_array_ref));

    empty_array->pointer = NULL;
    empty_array->array_size = 0;
    empty_array->allocated_size = 0;

    return empty_array;

}

int find_hard_block_instance(t_hard_block_recog* module_hard_block_node_refs_and_info, t_parsed_hard_block_port_info* curr_module_node_info)
{
    int hard_block_instance_index = 0;

    std::unordered_map<std::string,int>::iterator curr_hard_block_instance_index_ref;

    std::string curr_hard_block_instance_name = curr_module_node_info->hard_block_name;

    /* if we previously found a module node that represented a different port of the same hard block instance, we would have already created the node to represent the hard block instance.

    Search the current list of already created hard block instances for the hard block instance the current node is a part (remember that the current node represents a port for a hard block).
    We are using hard block names to idetify each instance.*/
    curr_hard_block_instance_index_ref = module_hard_block_node_refs_and_info->hard_block_instance_name_to_index.find(curr_hard_block_instance_name);

    // now check the search result to see whether the hard block instance the current node is a part of was already created (if it exists in our internal list)
    if (curr_hard_block_instance_index_ref == module_hard_block_node_refs_and_info->hard_block_instance_name_to_index.end())
    {
        // if we are here, then the hard block instance the current node belongs to does not exist
        // return unique identifier
        hard_block_instance_index = HARD_BLOCK_INSTANCE_DOES_NOT_EXIST;
        
    }
    else
    {
        // if we are here then the the hard block instance the current node is a port of already exists. 
        //so we store its index to identify it from all the other hard block instaces found in the netlist (all hard block instances are stored in a vector within 't_hard_block_recog')
        hard_block_instance_index = curr_hard_block_instance_index_ref->second;
    }


    return hard_block_instance_index;

}

int create_new_hard_block_instance(t_array_ref* module_node_list, t_hard_block_recog* module_hard_block_node_refs_and_info, t_parsed_hard_block_port_info* curr_module_node_info)
{
    // index to to the new 't_hard_block' struct being created here that is found within the 'hard_block_instances' vector
    int created_hard_block_instance_index = 0;

    // storage for the newly created hard block ports
    t_array_ref* created_hard_block_instance_ports = NULL;

    // storage for the new node within the current module that represents the new hard block instance being created here
    t_node* hard_block_instance_node = NULL;

    //get the port information for the new hard block instances type
   t_hard_block_port_info* port_info_for_curr_hard_block_type = &((module_hard_block_node_refs_and_info->hard_block_type_name_to_port_info.find(curr_module_node_info->hard_block_type))->second);

    // create a new set of ports for the new hard block instance
    created_hard_block_instance_ports = create_unconnected_hard_block_instance_ports(port_info_for_curr_hard_block_type);

    // create a new node for the new hard block instance
    hard_block_instance_node = create_new_hard_block_instance_node(created_hard_block_instance_ports, curr_module_node_info);

    //store the new hard block instance information created above within a 't_hard_block' struct and get the index to where it is stored
    created_hard_block_instance_index = store_new_hard_block_instance_info(module_hard_block_node_refs_and_info, port_info_for_curr_hard_block_type, hard_block_instance_node, curr_module_node_info);

    // append the newly created node that represents the new hard block instance to the node list for the module
    append_array_element((intptr_t)hard_block_instance_node, module_node_list);

    // delete the array ref struct used to store the reference to the newly created hard block instance ports
    vtr::free(created_hard_block_instance_ports);

    return created_hard_block_instance_index;
}

void assign_net_to_hard_block_instance_port(t_node* curr_module_node, t_parsed_hard_block_port_info* curr_module_node_info, t_hard_block_recog* module_hard_block_node_refs_and_info, int curr_hard_block_instance_index)
{
    t_hard_block* curr_hard_block_instance = &(module_hard_block_node_refs_and_info->hard_block_instances[curr_hard_block_instance_index]);

    std::unordered_map<std::string,t_hard_block_port_info>::iterator curr_hard_block_type_port_info = module_hard_block_node_refs_and_info->hard_block_type_name_to_port_info.find(curr_module_node_info->hard_block_type);

    int port_to_assign_index = 0;

    t_pin_def* net_to_assign = get_net_to_assign_to_hard_block_instance_port(curr_module_node);

    port_to_assign_index = identify_port_index_within_hard_block_type_port_array(&(curr_hard_block_type_port_info->second), curr_module_node_info, curr_module_node);

    // assign the net to tocrresponding hard block instance port //
    handle_net_assignment(curr_module_node, curr_hard_block_instance, port_to_assign_index, net_to_assign, curr_module_node_info);

    //curr_hard_block_instance->hard_block_instance_node_reference->array_of_ports[port_to_assign_index]->associated_net = net_to_assign;

    return;
}

t_pin_def* get_net_to_assign_to_hard_block_instance_port(t_node* curr_module_node)
{
    t_pin_def* net_to_assign = NULL;
    t_node_port_association* port_connected_to_net = NULL;

    int number_of_ports_in_node = curr_module_node->number_of_ports;

    std::string curr_module_node_type = curr_module_node->type;
    std::string curr_module_node_name = curr_module_node->name;

    std::string curr_port_name;

    // store what type of port we expect the net to be connected to 
    std::string expected_port_type;

    /* for a detailed error message, we identify what type of port
    the net we want should be connected to for the module node */

    if (!(curr_module_node_type.compare(LUT_TYPE)))
    {

        // we are here if the current node is LUT

        // if the current node is a LUT, then the LUT represents an input port and so the LUT input should be connected to the net
        expected_port_type.assign(INPUT_PORTS);
    }
    else
    {
        // we are here if the current node is a DFF

        // if the current node is a DFF, then the DFF represents an output port and so the DFF output should be connected to the net
        expected_port_type.assign(OUTPUT_PORTS);
    }

    for (int i = 0; i < number_of_ports_in_node; i++)
    {

        curr_port_name.assign(curr_module_node->array_of_ports[i]->port_name);

        if (!(curr_module_node_type.compare(LUT_TYPE)))
        {
            // if the node is a LUT //
            if (curr_port_name.compare(LUT_OUTPUT_PORT))
            {
                // if the port is not the LUT output ie. "combout" port //

                /* if we are here then we know that the LUT the current node
                represents must be an input port for hard block instance it is part of. We also know that this type of LUT only has an input and output port in the vqm netlist. Finally we know that the current port must be an input, so it's connected net is what we want to assign to the hard block instance port. */
                port_connected_to_net = curr_module_node->array_of_ports[i];

                break;
            }

        }
        else
        {
            // if the node is a flip flop //
            if (!(curr_port_name.compare(DFF_OUTPUT_PORT)))
            {
                // if the port is a D flip flop output ie. "q" port //

                /* if we are here then we know that the DFF the current node
                represents mus be an output port for hard block instance it is part of. Finally we know that the current port must be an output, so its connected net is what we want to assign to the hard block instance port. */
                port_connected_to_net = curr_module_node->array_of_ports[i];

                break;
            }
        }
    }

    if (port_connected_to_net != NULL)
    {
        net_to_assign = port_connected_to_net->associated_net;
    }
    else
    {
        /* we did not find any ports that fit the conditions above
        and this means that the current node, which represents a hard block 
        instance port does not have an accompanying net. This should never happen, so throw an error and indicate to the user */
        throw vtr::VtrError("The vqm netlist node '" + curr_module_node_name + "' does not have an " + expected_port_type + " port.");

    }

    return net_to_assign;
}

int identify_port_index_within_hard_block_type_port_array(t_hard_block_port_info* curr_hard_block_type_port_info, t_parsed_hard_block_port_info* curr_module_node_info, t_node* curr_module_node)
{
    int identified_port_index = 0;
    
    int port_end_index = 0;

    // error related identifiers
    std::string curr_module_node_name = curr_module_node->name;
    
    // identifier to store the port size of the current port
    std::unordered_map<std::string,int>::iterator found_port_size;

    /* use the mapping structure within the hard block port info struct
    to find the index the port can be found in within the port array of a given hard block. If the port is vectored, then the index returned is the beginning of the vectored port (ie. vectored_port[0])*/
    std::unordered_map<std::string,int>::iterator found_port_start_index = curr_hard_block_type_port_info->port_name_to_port_start_index.find(curr_module_node_info->hard_block_port_name);

    // check to see whether the port name exists
    if (found_port_start_index == (curr_hard_block_type_port_info->port_name_to_port_start_index.end()))
    {
        // port does not exist, so throw and error and indicate it to the user
        throw vtr::VtrError("The vqm netlist node '" + curr_module_node_name + "' represents a port: '" + curr_module_node_info->hard_block_port_name +"' within hard block model: '" + curr_module_node_info->hard_block_type + "'. But this port does not exist within the given hard block model as described in the architecture file.");
    }


    // at this point the port belongs to the hard block model //

    /* now we assign the found base port index and increment it by the parsed index from the node name (ie hard_block_port_index from t_parsed_hard_block_port_info). 
    
    For example support a port array is arranged as follows:
    index:         0           1           2            3
    port_array: port_1[0]  port_1[1]   port_1[2]    port_1[2]

    Now if we want the index of port_1[1], the base port index would be 0, then we need to increment by 1. This is why the increment is requried.

    If the port is not vectored, the increment value would be 0. So the case is handled
    */
   identified_port_index = found_port_start_index->second;
   identified_port_index += curr_module_node_info->hard_block_port_index;

   // now we check whether the port index is within the port range //

   // finf the port size of the current port (using internal mapping structure found in the hard block port info struct)
   // this should result in a valid value as we already verfied whether the port exists
   found_port_size = curr_hard_block_type_port_info->port_name_to_port_size.find(curr_module_node_info->hard_block_port_name);

   // calculate port end index
   port_end_index = found_port_start_index->second + (found_port_size->second - 1);

   // verify if the current port index is out of ranged by chekcing if it is larger than the maximum index for the port
   if (identified_port_index > port_end_index)
   {
       // port index is out of range, so throw and error and indicate it to the user
        throw vtr::VtrError("The vqm netlist node '" + curr_module_node_name + "' represents a port: '" + curr_module_node_info->hard_block_port_name +"' at index: " + std::to_string(curr_module_node_info->hard_block_port_index) + ", within hard block model: '" + curr_module_node_info->hard_block_type + "'. But this port index is out of range in the given hard block model as described in the architecture file.");
   }

   return identified_port_index;

}

void handle_net_assignment(t_node* curr_module_node, t_hard_block* curr_hard_block_instance, int port_to_assign_index, t_pin_def* net_to_assign, t_parsed_hard_block_port_info* curr_module_node_info)
{
    std::string curr_module_node_name = curr_module_node->name;

    t_node_port_association* port_to_assign = curr_hard_block_instance->hard_block_instance_node_reference->array_of_ports[port_to_assign_index];

    // need to check whether the current port was previouly assigned to a net
    if ((port_to_assign->associated_net) == NULL)
    {
        // port was not assigned to a net previously //

        // assign the net to the port ("connect" them)
        // since we just connected a port, we have one less port to assign, so update the "ports left to assign" tracker
        port_to_assign->associated_net = net_to_assign;
        curr_hard_block_instance->hard_block_ports_not_assigned -= 1;
    }
    else
    {
        // the port was already assigned previouly, this is an error, since we cannot have multiple nets connected to the same port
        // so report an error
        throw vtr::VtrError("The vqm netlist node '" + curr_module_node_name + "' represents a port: '" + curr_module_node_info->hard_block_port_name +"' within hard block model: '" + curr_module_node_info->hard_block_type + "'. But this port was already represented previously, therefore the current netlist node is a duplication of the port. A port can only have one netlist node representing it.");
    }

    return;
}

bool is_hard_block_port_legal(t_node* curr_module_node)
{

    bool result = false;

    std::string curr_module_node_type = curr_module_node->type;

    // now we check the direction of the port as input or whether the node is a DFF
    if ((curr_module_node->number_of_ports != LUT_OUTPUT_PORT_SIZE) || (!(curr_module_node_type.compare(DFF_TYPE))))
    {
        result = true;
    }

    return result;

}

t_array_ref* create_unconnected_hard_block_instance_ports(t_hard_block_port_info* curr_hard_block_type_port_info)
{
    t_array_ref* template_for_hard_block_ports = &(curr_hard_block_type_port_info->hard_block_ports);
    t_array_ref* hard_block_instance_port_array = NULL;
    int number_of_ports = template_for_hard_block_ports->array_size;

    // temporarily store single port parameters
    char* port_name = NULL;
    int port_index = 0;
    int port_wire_index = 0;

    // store the newly created port
    t_node_port_association* temp_port = NULL;

    // convert the template hard block ports into a port format
    t_node_port_association** template_port_array = (t_node_port_association**)template_for_hard_block_ports->pointer;

    // create memory to store the ports for the newly created hard block instance
    hard_block_instance_port_array = create_and_initialize_t_array_ref_struct();

    // go through the template ports, copy their parameters to a new set of ports that will be for the new hard block instance we are creating
    for (int i = 0; i < number_of_ports; i++)
    {
        port_name = template_port_array[i]->port_name;
        port_index = template_port_array[i]->port_index;
        port_wire_index = template_port_array[i]->wire_index;

        temp_port = create_unconnected_node_port_association(port_name, port_index, port_wire_index);

        append_array_element((intptr_t)temp_port, hard_block_instance_port_array);
    }

    return hard_block_instance_port_array;

}

t_node* create_new_hard_block_instance_node(t_array_ref* curr_hard_block_instance_ports, t_parsed_hard_block_port_info* curr_hard_block_instance_info)
{
    t_node* new_hard_block_instance = NULL;

    char* hard_block_instance_name = NULL;
    char* hard_block_instance_type = NULL;

    int hard_block_instance_name_size = strlen((curr_hard_block_instance_info->hard_block_name).c_str()) + 1;
    int hard_block_instance_type_size = strlen((curr_hard_block_instance_info->hard_block_type).c_str()) + 1;

    // create the node for the new hard block instance
    new_hard_block_instance = (t_node*)vtr::malloc(sizeof(t_node));

    // assign the ports and their count for the new hard block
    new_hard_block_instance->array_of_ports = (t_node_port_association**)curr_hard_block_instance_ports->pointer;

    new_hard_block_instance->number_of_ports = curr_hard_block_instance_ports->array_size;

    // hard blocks will not have any parameters so indicate that
    new_hard_block_instance->number_of_params = 0;
    new_hard_block_instance->array_of_params = NULL;

    // create storage for the name and type of the current hard block and store their values //
    
    hard_block_instance_name = (char*)vtr::malloc(sizeof(char)*hard_block_instance_name_size);
    hard_block_instance_type = (char*)vtr::malloc(sizeof(char)*hard_block_instance_type_size);

    strcpy(hard_block_instance_name, (curr_hard_block_instance_info->hard_block_name).c_str());
    strcpy(hard_block_instance_type, (curr_hard_block_instance_info->hard_block_type).c_str());

    new_hard_block_instance->name = hard_block_instance_name;
    new_hard_block_instance->type = hard_block_instance_type;

    return new_hard_block_instance;

}

int store_new_hard_block_instance_info(t_hard_block_recog* module_hard_block_node_refs_and_info, t_hard_block_port_info* curr_hard_block_type_port_info, t_node* new_hard_block_instance_node, t_parsed_hard_block_port_info* curr_module_node_info)
{
    int new_hard_block_instance_index = 0;

    t_hard_block new_hard_block_instance_info;

    // store information regarding the new hard block instance being added to the module node list to a new t_hard_block struct //

    new_hard_block_instance_info.hard_block_instance_node_reference = new_hard_block_instance_node;
    
    // initially all ports for the newly created hard block instance are unassigned 
    new_hard_block_instance_info.hard_block_ports_not_assigned = curr_hard_block_type_port_info->hard_block_ports.array_size;

    // insert the hard block (t_hard_block struct) to the list of all hard block instances
    module_hard_block_node_refs_and_info->hard_block_instances.push_back(new_hard_block_instance_info);

    // since we added the new hard block instance (t_hard_block struct) at the end of vector, the index will be the last position with the list
    new_hard_block_instance_index = module_hard_block_node_refs_and_info->hard_block_instances.size() - 1;

    /* now create a mapping between the new hard block instance name and the index it is located in with the list of all hard block instances, so we can find it quicly using just the hard block instance name*/
    module_hard_block_node_refs_and_info->hard_block_instance_name_to_index.insert(std::pair<std::string,int>(curr_module_node_info->hard_block_name,new_hard_block_instance_index));

    return new_hard_block_instance_index;
}

t_array_ref* create_t_array_ref_from_array(void** array_to_store, int array_size)
{   
    t_array_ref* array_reference = NULL;
    int array_allocated_size = 0;

    array_reference = (t_array_ref*)vtr::malloc(sizeof(t_array_ref));

    array_allocated_size = calculate_array_size_using_bounds(array_size);

    // assign the array and its corresponding information to the array ref struct
    array_reference->allocated_size = array_allocated_size;
    array_reference->array_size = array_size;
    array_reference->pointer = array_to_store;

    return array_reference;

}

t_parsed_hard_block_port_info extract_hard_block_port_info_from_module_node(t_node* curr_module_node, std::vector<std::string>* hard_block_type_name_list)
{
    std::string curr_module_node_name = curr_module_node->name;

    // container to hold all the names of the different hierachy levels found for the current node in the netlist(refer to 'split_node_name function' for more info)
    std::vector<std::string> components_of_module_node_name;

    int index_of_node_name_component_with_hard_block_type_info = 0;
    int index_of_node_name_component_with_hard_block_port_info = 0;

    // data structure to hold all the extract port information
    t_parsed_hard_block_port_info stored_port_info;

    split_node_name(curr_module_node_name, &components_of_module_node_name, VQM_NODE_NAME_DELIMITER);

    // if the node name does not have atleast two hierarhcy levels, then it cannot be a hard block port so we cannot extract any more information.
    // a hard block port in the vqm netlist must have atleast the port information and the next top level block name it is connected to. As shown below:
    // For example, \router:test_noc_router|payload[8]~QIC_DANGLING_PORT_I is a hard block payload port connected to a router block.
    // \Add0~9_I is not a hard block port
    if ((components_of_module_node_name.size()) >= 2)
    {
        // looking at the comment above, regardless of how many hierarchy levels an node has, the lowest level (last index) will contain the port info and the second lowest level (second last index) will contain the hard block type.
        // so we store those indices below
        index_of_node_name_component_with_hard_block_type_info = components_of_module_node_name.size() - 2;
        index_of_node_name_component_with_hard_block_port_info = components_of_module_node_name.size() - 1;

        stored_port_info.hard_block_type.assign(identify_hard_block_type(hard_block_type_name_list, components_of_module_node_name[index_of_node_name_component_with_hard_block_type_info]));

        // if the hard block type was empty, then the current node does not represent a hard block port, therefore we cannot extract any more info
        /* For example, sha256_pc:\sha256_gen:10:sha256_pc_gen:sha256_2|altshift_taps:q_w_rtl_2|shift_taps_b8v:auto_generated|altsyncram_6aa1:altsyncram5|ram_block6a17550~I meets all the conditions to get here, but it is a ram block */
        if (!(stored_port_info.hard_block_type.empty()))
        {
            // if the hard block type was verified then we can go ahead and store its name and also its parsed port name and index

            stored_port_info.hard_block_name.assign(construct_hard_block_name(&components_of_module_node_name, VQM_NODE_NAME_DELIMITER));

            identify_hard_block_port_name_and_index(&stored_port_info, components_of_module_node_name[index_of_node_name_component_with_hard_block_port_info]);
        }
    }

    return stored_port_info;

}

void split_node_name(std::string original_node_name, std::vector<std::string>* node_name_components, std::string delimiter)
{

    // positional trackers to determine the beginning and end position of each hierarchy level (component of the node name) of the current node within the design. The positions are updated as we go through the node name and identify every level of hierarchy.
    size_t start_of_current_node_name_component = 0;
    size_t end_of_current_node_name_component = 0;

    // go through the node name, then identify and store each hierarchy level of the node (represented as a component of the original node name), found using the delimiter
    while ((end_of_current_node_name_component = original_node_name.find(delimiter, start_of_current_node_name_component)) != std::string::npos)
    {
        // store the current node name component (current hierarchy level)
        node_name_components->push_back(original_node_name.substr(start_of_current_node_name_component, end_of_current_node_name_component - start_of_current_node_name_component));

        // update position for the next node name component (next hierarchy level)
        start_of_current_node_name_component = end_of_current_node_name_component + delimiter.length();

    }

    // since the last component (the port info is not follwed by a delimiter we need to handle it here and store it)
    node_name_components->push_back(original_node_name.substr(start_of_current_node_name_component, end_of_current_node_name_component - start_of_current_node_name_component));

    return;
}

std::string identify_hard_block_type(std::vector<std::string>* hard_block_type_name_list, std::string curr_node_name_component)
{
    std::vector<std::string>::iterator hard_block_type_name_traverser;
    
    std::string hard_block_type_name_to_find;
    std::string hard_block_type = "";
    
    size_t match_result = 0;

    for (hard_block_type_name_traverser = hard_block_type_name_list->begin(); hard_block_type_name_traverser != hard_block_type_name_list->end(); hard_block_type_name_traverser++)
    {
        hard_block_type_name_to_find.assign(*hard_block_type_name_traverser + HARD_BLOCK_TYPE_NAME_SEPERATOR);

        match_result = curr_node_name_component.find(hard_block_type_name_to_find);

        if (match_result != std::string::npos)
        {
            hard_block_type.assign(*hard_block_type_name_traverser);
            break;
        }

    }

    return hard_block_type;
}

std::string construct_hard_block_name(std::vector<std::string>*node_name_components, std::string delimiter)
{
    // stores the full name of the hard block the current node is part of, the current node represents a port of a hard block
    std::string curr_hard_block_name = "";

    /* the hard block name should not include the specific port the current node represents (so we reduce the size by 1 to remove it when constructing the hard block name)
    the last index of the node name components vector contains the port information, so by reducing the size of the vector by one we can ignore the port info when constructing the hard block name */
    int number_of_node_name_components = node_name_components->size() - 1;

    // go through the node name components and combine them together to form the hard block name. 
    for (int i = 0; i < number_of_node_name_components; i++)
    {
        curr_hard_block_name += (*node_name_components)[i] + delimiter;
    }

    return curr_hard_block_name;

}

void identify_hard_block_port_name_and_index (t_parsed_hard_block_port_info* curr_hard_block_port, std::string curr_node_name_component)
{   
    // identifer to check whether the port defined in the current node name is a bus (ex. payload[1]~QIC_DANGLING_PORT_I)
    std::regex port_is_a_bus ("(.*)[[]([0-9]*)\]~(?:.*)");

    // identifier to check whether the current port defined in the current node name isn't a bus (ex. value~9490_I)
    std::regex port_is_not_a_bus ("(.*)~(?:.*)");

    // when we compare the given node name component with the previous identifiers, the port name and port index are extracted and they are stored in the variable below
    std::smatch port_info;

    // handle either the case where the current port defined in the provided node name is either a bus or not
    // we should never not match with either case below
    if (std::regex_match(curr_node_name_component, port_info, port_is_a_bus, std::regex_constants::match_default))
    {
        // if we are here, then the port is a bus

        // store the extracted port name and index to be used externally
        curr_hard_block_port->hard_block_port_name.assign(port_info[PORT_NAME]);
        
        curr_hard_block_port->hard_block_port_index = std::stoi(port_info[PORT_INDEX]);
    }
    else if (std::regex_match(curr_node_name_component, port_info, port_is_not_a_bus, std::regex_constants::match_default))
    {
        // if we are here then the port was not a bus
        // not need to update port index as the default value represents a port that is not a bus

        // store the extracted port name
        curr_hard_block_port->hard_block_port_name.assign(port_info[PORT_NAME]);
    
    }

    return;
}

void delete_hard_block_port_info(std::unordered_map<std::string, t_hard_block_port_info>* hard_block_type_name_to_port_info_map)
{
    std::unordered_map<std::string, t_hard_block_port_info>::iterator curr_hard_block_port_info = hard_block_type_name_to_port_info_map->begin();

    while (curr_hard_block_port_info != (hard_block_type_name_to_port_info_map->end()))
    {

        int number_of_ports = (curr_hard_block_port_info->second).hard_block_ports.array_size;
    
        uintptr_t* ports_to_delete = (uintptr_t*)((curr_hard_block_port_info->second).hard_block_ports.pointer);

        deallocate_array(ports_to_delete, number_of_ports, free_port_association);

        curr_hard_block_port_info++;

    }

    return;  
}