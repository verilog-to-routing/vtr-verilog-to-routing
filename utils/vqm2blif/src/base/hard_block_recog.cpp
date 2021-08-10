// add higher level comments here



#include "hard_block_recog.h"


void filter_and_create_hard_blocks(t_module* main_module, t_arch* main_arch, std::vector<std::string>* hard_block_type_names, std::string arch_file_name, std::string vqm_file_name)
{
    t_hard_block_recog hard_block_node_refs_and_info;

    try
    {
        initialize_hard_block_models(main_arch, hard_block_type_names, &hard_block_node_refs_and_info);
    }
    catch(const vtr::VtrError& error)
    {
        throw vtr::VtrError((std::string)error.what() + "The FPGA architecture is described in " + arch_file_name + ".");
    }






    // need to delete all the dynamic memory used to store 
    // all the hard block port information
    delete_hard_block_port_info(&(hard_block_node_refs_and_info.hard_block_type_name_to_port_info));
    
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
            throw vtr::VtrError("The provided hard block model '" + *hard_block_type_name_traverser + "' was not found within the corresponding FPGA architecture.");
        }
        else
        { 
            single_hard_block_init_result =  create_and_initialize_all_hard_block_ports(hard_block_model, storage_of_hard_block_info);

            if (!single_hard_block_init_result)
            {
                throw vtr::VtrError("Hard block model '" + *hard_block_type_name_traverser + "' found in the architecture has no input/output ports.");
            }
        }

    }

    return;

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

        store_hard_block_port_info(storage_of_hard_block_info, curr_hard_block_type_name, curr_hard_block_model_port->name, curr_hard_block_model_port->dir, &equivalent_hard_block_node_port_array, &port_index);

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
    port_array = (t_array_ref*)vtr::malloc(sizeof(t_array_ref));
    
    port_array->pointer = NULL;
    port_array->array_size = 0;
    port_array->allocated_size = 0;

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
void store_hard_block_port_info(t_hard_block_recog* storage_of_hard_block_port_info, std::string curr_hard_block_type_name,std::string curr_port_name, PORTS current_port_dir,t_array_ref** curr_port_array, int* port_index)
{
       
    std::unordered_map<std::string,t_hard_block_port_info>::iterator curr_port_info = ((storage_of_hard_block_port_info->hard_block_type_name_to_port_info).find(curr_hard_block_type_name));

    copy_array_ref(*curr_port_array, &(curr_port_info->second).hard_block_ports);
    
    // insert the port name to the current index
    (curr_port_info->second).port_name_to_port_start_index.insert({curr_port_name, *port_index});

    //insert the port direction of the current port
    (curr_port_info->second).port_name_to_port_dir.insert({curr_port_name, current_port_dir});

    *port_index += (*curr_port_array)->array_size;

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

void identify_hard_block_port_name_and_index (t_parsed_hard_block_component_info* curr_hard_block_component, std::string curr_node_name_component)
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

        // store the extarcted port name and index to be used externally
        curr_hard_block_component->curr_hard_block_port_name.assign(port_info[PORT_NAME]);
        
        curr_hard_block_component->curr_hard_block_port_index = std::stoi(port_info[PORT_INDEX]);
    }
    else if (std::regex_match(curr_node_name_component, port_info, port_is_not_a_bus, std::regex_constants::match_default))
    {
        // if we are here then the port was not a bus

        // just store the extracted port name and default poert index to be used externally
        curr_hard_block_component->curr_hard_block_port_name.assign(port_info[PORT_NAME]);
        
        curr_hard_block_component->curr_hard_block_port_index = DEFAULT_PORT_INDEX;
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