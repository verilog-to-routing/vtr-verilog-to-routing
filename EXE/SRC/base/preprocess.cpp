//============================================================================================
//				INCLUDES
//============================================================================================
#include "../include/preprocess.h"
#include "../include/libvqm/vqm_common.h"


//============================================================================================
//			INTERNAL FUNCTION DECLARATIONS
//============================================================================================

//Functions to identify and decompose inout pins
void decompose_inout_pins(t_module* module, t_arch* arch);

t_model* find_model_in_architecture(t_model* arch_models, t_node* node);

t_model_ports* find_port_in_architecture_model(t_model* arch_model, t_node_port_association* node_port);

char* prefix_string(const char* prefix, const char* base);

t_split_inout_pin create_decomposed_pins(t_module* module, t_pin_def* pin);

int fix_netlist_connectivity_for_inout_pins(t_split_inout_pin* new_input_pin,
                                            t_assign_vec* pin_assignment_sources,
                                            t_assign_vec* pin_assignment_sinks, 
                                            t_port_vec* pin_node_sources,
                                            t_port_vec* pin_node_sinks           );

//Functions to remove global constants
void remove_constant_nets(t_module *module);


//Functions to identify dual-clock RAMs and split them into seperate blocks
void decompose_multiclock_blocks(t_module* module, t_arch* arch, t_type_descriptor* arch_types, int num_types);

void duplicate_and_split_multiclock_blocks(t_module* module, vector<t_node*>& multiclock_blocks);

map<t_node_port_association*, t_node*> map_ports_to_split_blocks(t_node* orig_node, t_node* split_node_a, t_node* split_node_b);

void dump_node_ports(t_node* node);

t_node_parameter* find_node_param(t_node* node, const char* param_name);

t_node* duplicate_node(t_node* orig_node);
t_node_parameter* duplicate_param(t_node_parameter* orig_param);
t_node_port_association* duplicate_port(t_node_port_association* orig_port);

//Functions to identify global nets
void add_global_to_nonglobal_buffers(t_module* module, t_arch* arch, t_type_descriptor* arch_types, int num_types);

t_global_ports identify_primitive_global_pins(t_arch* arch, t_type_descriptor* arch_types, int num_types);

t_global_nets identify_global_nets(t_module* module, t_global_ports global_port_vec);

t_net_driver_map identify_net_drivers(t_module* module, t_arch* arch, t_global_ports global_ports, t_global_nets global_nets);
 
t_assign_vec_pair identify_global_local_assignments(t_module* module, t_global_nets global_nets, t_net_driver_map net_driver_map);

t_node_port_vec_pair identify_global_local_pins(t_module* module, t_arch* arch, t_global_ports global_ports,
                                                t_global_nets global_nets, t_net_driver_map net_driver_map);
    
int insert_fake_global_buffer_cells(t_module* module, t_assign_vec_pair global_local_assign, t_node_port_vec_pair global_local_pins);

boolean is_node_port_global(t_node* node, t_node_port_association* node_port, t_global_ports global_ports);

boolean is_net_global(t_pin_def* net, t_global_nets global_nets);

t_pin_def* add_new_input_buffer(t_module* module, t_pin_def* global_src_net, t_buffer_type buffer_type, int num_buffers_inserted);

char* extract_primitive_name(char* pb_type_blif_model);

t_pin_def* duplicate_pin(t_module* module, const char* name_prefix, const char* name_base, t_pin_def* src_pin, t_pin_def_type pin_type);
t_pin_def* add_pin(t_module* module);
t_node* add_buffer(t_module* module, t_buffer_type buffer_type, int num_buffers_inserted);

void print_map(t_global_ports global_ports);
//============================================================================================
//============================================================================================
void preprocess_netlist(t_module* module, t_arch* arch, t_type_descriptor* arch_types, int num_types, t_boolean fix_global_nets, t_boolean split_multiclock_blocks){
    /*
     * Put all netlist pre-processing function calls here
     */

    //Clean up 'inout' pins by replacing them with
    // seperate 'input' and 'output' pins
    cout << "\t>> Preprocessing Netlist to decompose inout pins\n";
    decompose_inout_pins(module, arch);

    cout << "\n";

    cout << "\t>> Preprocessing Netlist to remove constant nets" << endl;
    remove_constant_nets(module);

    if(split_multiclock_blocks) {
        cout << "\t>> Preprocessing Netlist to decompose dual-clock Blocks\n";
        decompose_multiclock_blocks(module, arch, arch_types, num_types);
    }

    if(fix_global_nets) {
        //Add fake_gbuf cells to allow global signals (e.g. clocks & resets) to be
        // routed on 'global' networks, but still drive non-global signals (e.g. LUT
        // inputs, output pins etc.).
        cout << "\t>> Preprocessing Netlist to insert global to non-global buffers\n";
        add_global_to_nonglobal_buffers(module, arch, arch_types, num_types);
    }
}

//============================================================================================
//============================================================================================
void decompose_inout_pins(t_module* module, t_arch* arch){
    /*
     * Example inout pin connectivity:
     *
     *       --------------------------------
     *       |                              |
     *       |            -------------     |
     *       |            |           |     |
     *   inout_pin ---> child_input   |     |
     *       |  ^         |           |     |
     *       |  |         -------------     |
     *       |  |                           |
     *       |  |         ------------      |
     *       |  |         |          |      |
     *       |  |----- child_output  |      |
     *       |            |          |      |
     *       |            ------------      |
     *       |                              |
     *       --------------------------------
     * 
     * Here the inout pin 'inout_pin' drives the child input pin
     * 'child_input', and is driven by the child output pin
     * 'child_output'
     *
     *
     * Example Final Connectivity:
     *       -----------------------------------
     *       |                                 |
     *       |               -------------     |
     *       |               |           |     |
     * new_input_pin ---> child_input    |     |
     *       |               |           |     |
     *       |               -------------     |
     *       |                                 |
     *       |               ------------      |
     *       |               |          |      |
     * new_output_pin <-- child_output  |      |
     *       |               |          |      |
     *       |               ------------      |
     *       |                                 |
     *       -----------------------------------
     *
     * After processing, 'inout_pin' has been split into two
     * different pins 'new_input_pin' and 'new_output_pin'. 
     * As thier names imply, 'new_input_pin' and 'new_output_pin' 
     * respectively handle the input and output duties of the 
     * original inout pin
     *
     */

    //Counters
    int i,j,k,p;

    //Stats
    int number_of_inout_pins_found = 0;
    int number_of_nets_moved = 0;

    //The number of pins in the module may change as we process the netlist
    // therefore use only the initial number of pins.
    // This is valid since any new pins will be added onto the END of the
    // module->number_of_pins array.
    int initial_number_of_pins =  module->number_of_pins;

    //Look through all pins in the design
    for (i = 0; i < initial_number_of_pins; i++) {
        //The pin of interest
        t_pin_def* pin = module->array_of_pins[i];

        /*  
         *  Declare vectors to track source and sinks of pin
         */
        t_assign_vec pin_assignment_sources;
        t_assign_vec pin_assignment_sinks;
        t_port_vec   pin_node_sources;
        t_port_vec   pin_node_sinks;

        //Find inout pins
        if (pin->type == PIN_INOUT) {
            //Stats
            number_of_inout_pins_found++;
            
            if (verbose_mode) {
                cout << "\t   inout pin : " << pin->name << "\n";
            }

            /*
             *  Need to check two places to find sources and sinks
             *  of this pin:
             *    1) Assign statements
             *    2) Blackbox instantiations (nodes)
             */

            // 1) Check assignments (i.e. verilog assign statments) for driven 
            //    by and driving pins
            for(j = 0; j < module->number_of_assignments; j++) {

                t_assign* assign_stmt = module->array_of_assignments[j];

                if (pin == assign_stmt->target) {
                    //If pin is the target it is being driven by source
                    if (verbose_mode) {
                        cout << "\t    source   : " << assign_stmt->source->name << "\n";
                    }
                    pin_assignment_sources.push_back(assign_stmt);

                } else if (pin == assign_stmt->source) {
                    //If pin is the source it is driving target (target is a sink)
                    if (verbose_mode) {
                        cout << "\t    sink     : " << assign_stmt->target->name << "\n";
                    }
                    pin_assignment_sinks.push_back(assign_stmt);
                }
            }

            // 2) Check nodes (i.e. sub-modules) for driven by and driving pins
            for (k = 0; k < module->number_of_nodes; k++) {
                
                t_node* node = module->array_of_nodes[k];
               
                //Check ports of each node
                for(p = 0; p < node->number_of_ports; p++) {

                    t_node_port_association* node_port = node->array_of_ports[p];

                    //Is the pin attached to this black box node?
                    if (pin == node_port->associated_net) {
                        /*
                         *  Since these VQM nodes are blackboxes
                         *  we do not know whether the node_port
                         *  is an input or an output.
                         *
                         *  We must know this to ensure the port
                         *  is connected to the correct pin once
                         *  the inout pin is split into seperate
                         *  input and output pins.
                         *
                         *  This can be determined by querying the
                         *  architecture file.
                         */

                        //Architecture pointers
                        t_model* arch_model;
                        t_model_ports* arch_model_port;

                        //Find the model
                        arch_model = find_model_in_architecture(arch->models, node);

                        //Find the architecure model port
                        arch_model_port = find_port_in_architecture_model(arch_model, node_port);

                        if (arch_model_port->dir == IN_PORT) {
                            //It is an input, meaning a sink for node_port
                            if (verbose_mode) {
                                cout << "\t    sink: " << node->name << "." << node_port->port_name << " (" << node->type << " inport)\n";
                            }
                            pin_node_sinks.push_back(node_port);

                        } else if (arch_model_port->dir == OUT_PORT) {
                            //It is an output, meaning a source for pin
                            if (verbose_mode) {
                                cout << "\t    source   : " << node->name << "." << node_port->port_name << " (" << node->type << " outport)\n";
                            }
                            pin_node_sources.push_back(node_port);

                        } else {
                            cout << "Error: invalid port type for port'" << node_port->port_name << "' on model '" << node->type << "' in architecture file\n";
                            exit(1);
                        }

                    } //pin matches associated net (BB prim port)
                } //Node port
            } //Node



            /*
             * Create the new input and output pins that replace the inout
             * pin
             */
            t_split_inout_pin split_pin = create_decomposed_pins(module, pin);

            /*
             * Fix the connectivity of modules and assign statements to reflect
             * the new input and output pins
             */
            number_of_nets_moved += fix_netlist_connectivity_for_inout_pins(&split_pin,
                                                                            &pin_assignment_sources,
                                                                            &pin_assignment_sinks, 
                                                                            &pin_node_sources,
                                                                            &pin_node_sinks           );

        } //is PIN_INOUT

    } //module pins

    //Stats
    cout << "\t>> Decomposed " << number_of_inout_pins_found << " 'inout' pin(s), moving " << number_of_nets_moved << " net(s)\n";
}

t_model* find_model_in_architecture(t_model* arch_models, t_node* node) {
    /*
     * Finds the archtecture module corresponding to the node type
     *
     *  arch_models: the head of the linked list of architecture models
     *  node       : the VQM node to match
     *
     * Returns: A pointer to the corresponding model
     */

    //The VQM name may not match the architecture name if the architecture contians elaborated modes
    //  So generate the elaborated mode name for this node
    string elaborated_name = generate_opname(node, arch_models);

    //Find the correct model, by name matching
    t_model* arch_model = arch_models;
    while((arch_model) && (strcmp(elaborated_name.c_str(), arch_model->name) != 0)) {
        //Move to the next model
        arch_model = arch_model->next;
    }

    //The model must be in the arch file
    if (arch_model == NULL) {
        cout << "Error: could not find model in architecture file for '" << node->type << "'\n";
        exit(1);
    }
    assert(arch_model != NULL);

    return arch_model;
}

char* prefix_string(const char* prefix, const char* base) {

    //Number of characters (including terminator)
    size_t new_string_size = (strlen(prefix) + strlen(base) + 1)*sizeof(char);

    char* new_string = (char*) my_malloc(new_string_size);

    //Checks for overflow
    snprintf(new_string, new_string_size, "%s%s", prefix, base);

    return new_string;
}

t_model_ports* find_port_in_architecture_model(t_model* arch_model, t_node_port_association* node_port) {
    /*
     * Finds the module port corresponding to the port name 
     *
     *  arch_model: the architecture model matching node
     *  node_port : the port to match
     *
     * Returns: A pointer to the corresponding port
     */


    /*
     *  First chech the input linked list, if port name is not
     *  there, try the output linked list.  It is an error
     *  if port name is not in either list.
     */

    //Check inputs
    //Find the correct port, by name matching
    t_model_ports* arch_model_port = arch_model->inputs;

    //Until NULL or a matching port name
    while ((arch_model_port) && (strcmp(arch_model_port->name, node_port->port_name) != 0)) {
        //Must be an input if in the inputs linked list
        assert(arch_model_port->dir == IN_PORT); 

        //Move to the next arch_model_port
        arch_model_port = arch_model_port->next;
    }

    //arch_model_port is either null or an input
    if (arch_model_port == NULL) {
        //Not an input should be an output

        //Check outputs
        arch_model_port = arch_model->outputs;

        //Until NULL or a matching port name
        while ((arch_model_port) && (strcmp(arch_model_port->name, node_port->port_name) != 0)) {
            //Must be an output if in the output linked list
            assert(arch_model_port->dir == OUT_PORT); 

            //Move to the next arch_model_port
            arch_model_port = arch_model_port->next;
        }

        //arch_model_port must be either an input or an output, 
        // hence it should never be NULL at this point
        if (arch_model_port == NULL) {
            cout << "Error: could not find port '" << node_port->port_name << "' on model '" << arch_model->name << "' in architecture file\n";
            exit(1);
        }
    }

    assert(arch_model_port != NULL);

    return arch_model_port;
}


t_split_inout_pin create_decomposed_pins(t_module* module, t_pin_def* inout_pin) {
    /*
     *  Declare new pins to replace the inout pin
     *    We will replace the inout pin with it's decomposed 'input'
     *    and create the decomposed 'output' pin at the end of the
     *    module->array_of_pins
     */

    //Only need one additional pin, since we re-use the already
    // allocated 'inout' pin
    module->number_of_pins++;

    /*
     *  OUTPUT PIN
     */

    //Reallocate space
    module->array_of_pins = (t_pin_def**) my_realloc(module->array_of_pins, module->number_of_pins*sizeof(t_pin_def*));

    //The new output pin is at the end of the array, and must be
    // allocated
    module->array_of_pins[module->number_of_pins-1] = (t_pin_def*) my_malloc(sizeof(t_pin_def));

    //Pointer to the new output pin
    t_pin_def* new_output_pin = module->array_of_pins[module->number_of_pins-1];

    //Save the pointer to the original pin name
    char* pin_name_ptr = inout_pin->name;

    //Create the pin name
    new_output_pin->name = prefix_string(INOUT_OUTPUT_PREFIX, inout_pin->name);

    //Set the pin type
    new_output_pin->type = PIN_OUTPUT;

    //Copy other values
    new_output_pin->left = inout_pin->left;
    new_output_pin->right = inout_pin->right;
    new_output_pin->indexed = inout_pin->indexed;

    /*
     *  INPUT PIN
     */

    //The new input pin is the old inout pin
    // The old values of the pin are overwritten
    t_pin_def* new_input_pin = inout_pin;

    //Update the pin name
    new_input_pin->name = prefix_string(INOUT_INPUT_PREFIX, pin_name_ptr);

    //Update the pin type
    // All other attributes of the t_pin_def can remain the same as the
    // original inout pin
    new_input_pin->type = PIN_INPUT;

    //Use a struct to return the two new pin pointers
    t_split_inout_pin split_inout_pin;
    split_inout_pin.in = new_input_pin;
    split_inout_pin.out = new_output_pin;

    //Remove the old name, it is no longer needed
    free(pin_name_ptr);

    return split_inout_pin;
}

int fix_netlist_connectivity_for_inout_pins(t_split_inout_pin* split_inout_pin,
                                            t_assign_vec* pin_assignment_sources,
                                            t_assign_vec* pin_assignment_sinks, 
                                            t_port_vec* pin_node_sources,
                                            t_port_vec* pin_node_sinks           ){
    /*
     *  Correct the netlist connectivity so that the appropriate signals
     *   drive/are driven by the newly split inout pin.
     *
     *   There are four cases to consider:
     *     1) Assignments with the original inout pin as a source
     *     2) Assignments with the original inout pin as a sink
     *     3) Nodes with the original inout pin as a source
     *     4) Nodes with the original inout pin as a sink
     */
    size_t cnt;
    int number_of_nets_moved = 0;

    /* 
     *  Fix the connectivity for the assignments
     */
    //Sources for the original inout pin
    for (cnt = 0; cnt < pin_assignment_sources->size(); cnt++) {
        t_assign* assign_stmt = pin_assignment_sources->at(cnt);

        /*
         * Sources of the original pin should now map to thier targets
         * (outputs) to  the new output pin
         *  e.g.
         *       assign target_inout = source;
         *               
         *               becomes
         * 
         *       assign target_new_out = source;
         */
        
        assign_stmt->target = split_inout_pin->out;
        number_of_nets_moved++;
    }

    //Sinks for the original inout pin
    for (cnt = 0; cnt < pin_assignment_sinks->size(); cnt++) {
        t_assign* assign_stmt = pin_assignment_sinks->at(cnt);

        /*
         * Sinks of the original pin should now map to thier sources
         * (inputs) to the new input pin
         *  e.g.
         *       assign target = source_inout;
         *               
         *               becomes
         * 
         *       assign target = source_new_in;
         */
        
        assign_stmt->source = split_inout_pin->in;
        number_of_nets_moved++;
    }

    /* 
     *  Fix the connectivity for the nodes
     */
    //Sources for the original inout pin
    for (cnt = 0; cnt < pin_node_sources->size(); cnt++) {
        t_node_port_association* node_port = pin_node_sources->at(cnt);

        /*
         * Sources of the original pin should now map to thier outputs
         * to the new output pin
         *  e.g.
         *       inout pin1;
         * 
         *       buffer buf1 (
         *           .o (pin1) //buffer output
         *           .i ()
         *       )
         *               
         *               becomes
         * 
         *       output pin1_new_out;
         * 
         *       buffer buf1 (
         *           .o (pin1_new_out) //buffer output
         *           .i ()
         *       )
         */
        
        node_port->associated_net = split_inout_pin->out;
        number_of_nets_moved++;
    }

    //Sinks for the original inout pin
    for (cnt = 0; cnt < pin_node_sinks->size(); cnt++) {
        t_node_port_association* node_port = pin_node_sinks->at(cnt);

        /*
         * Sinks of the original pin should now map to thier inputs
         * to the new input pin
         *  e.g.
         *       inout pin1;
         * 
         *       buffer buf1 (
         *           .o ()
         *           .i (pin1) //buffer input
         *       )
         *               
         *               becomes
         * 
         *       input pin1_new_in;
         * 
         *       buffer buf1 (
         *           .o () 
         *           .i (pin1_new_in) //buffer input
         *       )
         */
        
        node_port->associated_net = split_inout_pin->in;
        number_of_nets_moved++;
    }

    return number_of_nets_moved;
}


//============================================================================================
//============================================================================================
void remove_constant_nets(t_module *module) {
    size_t num_const_port_connections_removed = 0;

    //Identify and save the constant nets by looking at
    //assignment statements
    vector<t_pin_def*> constant_nets;
    for(int i = 0; i < module->number_of_assignments; i++) {
        t_assign* assignment = module->array_of_assignments[i];

        t_pin_def* source_net = assignment->source;
        t_pin_def* target_net = assignment->target;

        //Constant assignments have no source_net
        if(!source_net) {
            if(verbose_mode) {
                cout << "\t\tConst Assignment: source '";
                cout << assignment->value << "' (const)";
                cout << " target '" << target_net->name << "'" << endl;
            }

            constant_nets.push_back(target_net);
        }
    }

    //Look at all the nodes and remove connections to the constant nets
    for(int i = 0; i < module->number_of_nodes; i++) {
        t_node* node = module->array_of_nodes[i];

        for(int j = 0; j < node->number_of_ports; j++) {
            t_node_port_association* node_port = node->array_of_ports[j];
            
            for(vector<t_pin_def*>::iterator pin_def_iter = constant_nets.begin(); 
                pin_def_iter != constant_nets.end(); pin_def_iter++) {
                t_pin_def* const_net = *pin_def_iter;

                if(node_port->associated_net == const_net) {
                    //cout << "Constant port connection: " << node->name << " (" << node->type << ") " << node_port->port_name << endl;
                    //Delete the connection, by removing the port (unconnected ports are non-existant in VQM)
                    
                    //Copy the last element over the element to be deleted
                    //
                    // Note that we don't reallocate the array size (with realloc), since
                    // it is unneccessary. It would be reallocated if expanded, and we do
                    // not suffer from a memory leak since we always keep the array pointer,
                    // which points to the originally allocated block.
                    if(node->number_of_ports > 1) {
                        node->array_of_ports[j] = node->array_of_ports[node->number_of_ports-1];
                    }

                    //Change the array bounds
                    node->number_of_ports--;

                    //Keep track of how many ports were removed
                    num_const_port_connections_removed++;
                }
            }
        }
    }

    cout << "\t>> Removed " << num_const_port_connections_removed << " connections to constant nets" << endl;
    
}

//============================================================================================
//============================================================================================
void decompose_multiclock_blocks(t_module* module, t_arch* arch, t_type_descriptor* arch_types, int num_types) {
    //Identify netlist multi-clock primitives
    vector<t_node*> multiclock_blocks;
    for(int i = 0; i < module->number_of_nodes; ++i) {

        t_node* node = module->array_of_nodes[i];

        if(strcmp(node->type, "stratixiv_ram_block") == 0) {

            int clock_count = 0;
            for(int j = 0; j < node->number_of_ports; j++) {
                t_node_port_association* node_port = node->array_of_ports[j];

                //TODO: don't hard code this, ideally somehow the arch file should describe which
                //      block types can have multiple clocks
                if(strstr(node_port->port_name, "clk") != NULL) {
                    clock_count++;
                }
            }

            if(clock_count >= 2) {
                cout << "\t\t>> Found " << node->type << " with " << clock_count << " clocks." << endl;
                multiclock_blocks.push_back(node);
            }
        }
    }

    cout << "\t\t>> Found "<< multiclock_blocks.size() << " multiclock blocks in total" << endl;
    cout << endl;


    //Split the netlist primitives into two types
    duplicate_and_split_multiclock_blocks(module, multiclock_blocks);
    
}

void duplicate_and_split_multiclock_blocks(t_module* module, vector<t_node*>& multiclock_blocks) {
    /*
     *  We must split multi-clock blocks into several primitive blocks since VPR
     *  currently only supports primitives with a single clock.
     *
     *  The key idea is to duplicate the ram node, with one primitive for each clock.
     *
     *  We also add an extra 'dummy' connection between them which is later used
     *  by the VPR packer (using molecules) to ensure that these are always packed
     *  together.
     *
     *  For example:
     *
     *     Given a simple dual port, dual clock ram block in the VQM netlist:
     *
     *                    --------------------
     *                    |     orig_ram     |
     *                    |                  |
     *           -------->|data_a      data_b|-------->
     *                    |                  | 
     *           -------->|addr_a      addr_b|<--------
     *                    |                  |
     *           -------->|clk_a        clk_b|<--------
     *                    |                  |
     *                    --------------------
     *
     *     We will transform it to:
     *
     *                    --------------------
     *                    | split_orig_ram_a |
     *                    |                  |
     *           -------->|data_a      data_b|
     *                    |                  | 
     *           -------->|addr_a      addr_b|
     *                    |                  |
     *           -------->|clk_a        clk_b|
     *                    |                  |
     *                    |             dummy|---|
     *                    |                  |   |
     *                    --------------------   |
     *                                           |
     *               ----------------------------|
     *               |
     *               |    --------------------
     *               |    | split_orig_ram_b |
     *               |    |                  |
     *               |    |data_a      data_b|------->
     *               |    |                  | 
     *               |    |addr_a      addr_b|<-------
     *               |    |                  |
     *               |    |clk_a        clk_b|<-------
     *               |    |                  |
     *               |--->|dummy             |
     *                    |                  |
     *                    --------------------
     *
     *
     * Breaking things down the following work must be done:
     *   1) Duplicate orig_ram to create split_orig_ram_b
     *   2) Rename the block names and types
     *   3) Disconnect port b related singals on split_orig_ram_a
     *   4) Disconnect port a related singals on split_orig_ram_b
     *   5) Add dummy output to split_orig_ram_a
     *   6) Add dummy input to split_orig_ram_b
     */

    int dummy_net_count = 0;
    for(vector<t_node*>::iterator iter = multiclock_blocks.begin(); iter != multiclock_blocks.end(); ++iter) {
        t_node* orig_node = *iter;

        //cout << "Node " << orig_node->name << " (" << orig_node->type << ")" << endl;
        //dump_node_ports(orig_node);

        size_t new_len = 0;

        //1) Duplicate the original block and insert the new copy
        t_node* split_node_b = duplicate_node(orig_node);

        module->number_of_nodes++;
        module->array_of_nodes = (t_node**) my_realloc(module->array_of_nodes, sizeof(t_node*)*module->number_of_nodes);
        module->array_of_nodes[module->number_of_nodes-1] = split_node_b;

        //2) Rename the block names and types
        t_node* split_node_a = orig_node; //Overwrite the original


        //Give 'A' a unique type
        new_len = strlen(split_node_a->type);
        new_len += strlen(SPLIT_A_POSTFIX);
        char* old_type = split_node_a->type;
        split_node_a->type = (char*) my_malloc(sizeof(*split_node_a->type)*(new_len+1));
        snprintf(split_node_a->type, new_len+1, "%s%s", old_type, SPLIT_A_POSTFIX);
        free(old_type);

        //Give 'A' a unique name 
        new_len = strlen(split_node_a->name);
        new_len += strlen(SPLIT_A_POSTFIX);
        char* old_name = split_node_a->name;
        split_node_a->name = (char*) my_malloc(sizeof(*split_node_a->name)*(new_len+1));
        snprintf(split_node_a->name, new_len+1, "%s%s", old_name, SPLIT_A_POSTFIX);
        free(old_name);

        //Give 'B' a unique type
        new_len = strlen(split_node_b->type);
        new_len += strlen(SPLIT_B_POSTFIX);
        old_type = split_node_b->type;
        split_node_b->type = (char*) my_malloc(sizeof(*split_node_b->type)*(new_len+1));
        snprintf(split_node_b->type, new_len+1, "%s%s", old_type, SPLIT_B_POSTFIX);
        free(old_type);

        //Give 'B' a unique name 
        new_len = strlen(split_node_b->name);
        new_len += strlen(SPLIT_B_POSTFIX);
        old_name = split_node_b->name;
        split_node_b->name = (char*) my_malloc(sizeof(*split_node_b->name)*(new_len+1));
        snprintf(split_node_b->name, new_len+1, "%s%s", old_name, SPLIT_B_POSTFIX);
        free(old_name);

        //3) Determine on which entity each port should be connected
        map<t_node_port_association*, t_node*> node_a_port_to_node_map = map_ports_to_split_blocks(split_node_a, split_node_a, split_node_b);
        map<t_node_port_association*, t_node*> node_b_port_to_node_map = map_ports_to_split_blocks(split_node_b, split_node_a, split_node_b);

        //4) Disconnect port signals on 'A'
        size_t num_ports_removed = 0;
        for(int i = 0; i < split_node_a->number_of_ports; i++) {
            t_node_port_association* node_port = split_node_a->array_of_ports[i];

            assert(node_a_port_to_node_map.count(node_port));

            if(node_a_port_to_node_map[node_port] != split_node_a) {
                //Disconnect this port
                num_ports_removed++;
                free_port_association(split_node_a->array_of_ports[i]);
                split_node_a->array_of_ports[i] = NULL;
            }
        }
        
        //Reallocate
        t_node_port_association** new_port_array = (t_node_port_association**) my_malloc(sizeof(t_node_port_association*)*(split_node_a->number_of_ports - num_ports_removed));
        size_t index = 0;
        for(int i = 0; i < split_node_a->number_of_ports; i++) {
            t_node_port_association* node_port = split_node_a->array_of_ports[i];

            if(node_port != NULL) {
                assert(index < split_node_a->number_of_ports - num_ports_removed);
                new_port_array[index] = node_port;
                index++;
            }
        }
        //Replace the old array
        free(split_node_a->array_of_ports);
        split_node_a->array_of_ports = new_port_array;
        split_node_a->number_of_ports = split_node_a->number_of_ports - num_ports_removed;
        
        //cout << "Split node a: " << split_node_a->type << endl;
        //dump_node_ports(split_node_a);


        //4) Disconnect port signals on 'B'
        num_ports_removed = 0;
        for(int i = 0; i < split_node_b->number_of_ports; i++) {
            t_node_port_association* node_port = split_node_b->array_of_ports[i];
            
            assert(node_b_port_to_node_map.count(node_port));

            if(node_b_port_to_node_map[node_port] != split_node_b) {
                //Disconnect this port
                num_ports_removed++;
                free_port_association(split_node_b->array_of_ports[i]);
                split_node_b->array_of_ports[i] = NULL;
            }
        }
        
        new_port_array = (t_node_port_association**) my_malloc(sizeof(t_node_port_association*)*(split_node_b->number_of_ports - num_ports_removed));
        index = 0;
        for(int i = 0; i < split_node_b->number_of_ports; i++) {
            t_node_port_association* node_port = split_node_b->array_of_ports[i];

            if(node_port != NULL) {
                assert(index < split_node_b->number_of_ports - num_ports_removed);
                new_port_array[index] = node_port;
                index++;
            }
        }
        //Replace the old array
        free(split_node_b->array_of_ports);
        split_node_b->array_of_ports = new_port_array;
        split_node_b->number_of_ports = split_node_b->number_of_ports - num_ports_removed;

        //cout << "Split node b: " << split_node_b->type << endl;
        //dump_node_ports(split_node_b);

        //5) Create dummy net to connect 'A' to 'B', used by packer molecules to always place these together
        module->number_of_pins++;
        module->array_of_pins = (t_pin_def**) my_realloc(module->array_of_pins, sizeof(t_pin_def*)*module->number_of_pins);

        //Create the new net
        t_pin_def* new_net = (t_pin_def*) my_malloc(sizeof(t_pin_def));

        char buf[50];
        snprintf(buf, sizeof(char)*50, DUMMY_NET_NAME_FORMAT, dummy_net_count);
        new_net->name = (char*) my_malloc(strlen(buf)+1);
        strncpy(new_net->name, buf, strlen(buf)+1);
        new_net->left = 0;
        new_net->right = 0;
        new_net->indexed = T_FALSE;
        new_net->type = PIN_WIRE;

        //Insert the new net
        module->array_of_pins[module->number_of_pins-1] = new_net;
        dummy_net_count++;

        //5) Add dummy signal to 'A'
        split_node_a->number_of_ports++;
        split_node_a->array_of_ports = (t_node_port_association**) my_realloc(split_node_a->array_of_ports, split_node_a->number_of_ports*sizeof(t_node_port_association*));

        t_node_port_association* new_port_a = (t_node_port_association*) my_malloc(sizeof(t_node_port_association));
        new_port_a->port_name = (char*) my_malloc(sizeof(char)*strlen(DUMMY_A_PORT_NAME)+1);
        snprintf(new_port_a->port_name, strlen(DUMMY_A_PORT_NAME)+1, "%s", DUMMY_A_PORT_NAME);
        new_port_a->port_index = -1;
        new_port_a->associated_net = new_net;
        new_port_a->wire_index = 0;

        split_node_a->array_of_ports[split_node_a->number_of_ports-1] = new_port_a;

        //6) Add dummy signal to 'B'
        split_node_b->number_of_ports++;
        split_node_b->array_of_ports = (t_node_port_association**) my_realloc(split_node_b->array_of_ports, split_node_b->number_of_ports*sizeof(t_node_port_association*));

        t_node_port_association* new_port_b = (t_node_port_association*) my_malloc(sizeof(t_node_port_association));
        new_port_b->port_name = (char*) my_malloc(sizeof(char)*strlen(DUMMY_B_PORT_NAME)+1);
        snprintf(new_port_b->port_name, strlen(DUMMY_B_PORT_NAME)+1, "%s", DUMMY_B_PORT_NAME);
        new_port_b->port_index = -1;
        new_port_b->associated_net = new_net;
        new_port_b->wire_index = 0;

        split_node_b->array_of_ports[split_node_b->number_of_ports-1] = new_port_b;

    }
}

void dump_node_ports(t_node* node) {
    for(int i = 0; i < node->number_of_ports; i++) {
        t_node_port_association* node_port = node->array_of_ports[i];

        cout << "\t" << node_port->port_name << " (" << node_port->associated_net->name << ")" << endl;
    }
}

t_node_parameter* find_node_param(t_node* node, const char* param_name) {
    for(int i = 0; i < node->number_of_params; i++) {
        t_node_parameter* param = node->array_of_params[i];

        if(strcmp(param->name, param_name) == 0) {
            return param;
        }
    }
    return NULL;
}

t_node* duplicate_node(t_node* orig_node) {
    t_node* new_node = (t_node*) my_malloc(sizeof(t_node));
    
    //Copy each field
    new_node->type = strdup(orig_node->type);
    new_node->name = strdup(orig_node->name);

    new_node->number_of_params = orig_node->number_of_params;
    new_node->array_of_params = (t_node_parameter**) my_malloc(sizeof(t_node_parameter*)*orig_node->number_of_params);
    for(int i = 0; i < orig_node->number_of_params; i++) {
        new_node->array_of_params[i] = duplicate_param(orig_node->array_of_params[i]);
    }
    
    new_node->number_of_ports = orig_node->number_of_ports;
    new_node->array_of_ports = (t_node_port_association**) my_malloc(sizeof(t_node_port_association*)*orig_node->number_of_ports);
    for(int i = 0; i < orig_node->number_of_ports; i++) {
        new_node->array_of_ports[i] = duplicate_port(orig_node->array_of_ports[i]);
    }

    return new_node;
}

t_node_parameter* duplicate_param(t_node_parameter* orig_param) {
    t_node_parameter* new_param = (t_node_parameter*) my_malloc(sizeof(t_node_parameter));
    
    new_param->name = strdup(orig_param->name);
    new_param->type = orig_param->type;
    new_param->value = orig_param->value;

    return new_param;
}

t_node_port_association* duplicate_port(t_node_port_association* orig_port) {
    t_node_port_association* new_port = (t_node_port_association*) my_malloc(sizeof(t_node_port_association));

    new_port->port_name = strdup(orig_port->port_name);
    new_port->port_index = orig_port->port_index;
    new_port->associated_net = orig_port->associated_net;
    new_port->wire_index = orig_port->wire_index;

    return new_port;
}

map<t_node_port_association*, t_node*> map_ports_to_split_blocks(t_node* orig_node, t_node* split_node_a, t_node* split_node_b) {
    //This map is used to connect/disconnect the appropriate ports
    // for each of the two split instances.
    // The key is the pointer to the node port, the value
    // is a pointer to split node it is associated with
    map<t_node_port_association*, t_node*> port_to_node_map; 
    
    //Initially split_node_a has all ports connected
    for(int i = 0; i < orig_node->number_of_ports; i++) {
        t_node_port_association* node_port = orig_node->array_of_ports[i];

        //This section based on the QUIP WYSWIG documentation for Stratix III (same as Stratix IV)
        // see 'stratixiii_ram_wys_eda.pdf'
        //
        // From that document: 
        //     "An important note here is that although the port B inputs can choose between clk0
        //      and clk1, the inputs on the same port do not allow the use of different clocks. 
        //      They must be synchronous to the same clock. So if port B data in uses clk0, port B 
        //      addresses can’t use clk1."
        //
        //
        // By the above logic we need to check only a subset of parameters.  Specifically we can
        // conclude:
        //    1) portadataout clock is determined by the 'port_a_data_out_clock' parameter, and may be
        //       none if unregistered
        //    2) portbdataout clock is determined by the 'port_b_data_out_clock' parameter, and may be
        //       none if unregistered
        //    3) Any 'porta*' INPUT signal must be clocked by clk0
        //    4) Any 'portb*' INPUT signal must be clocked by the clock specified by the
        //       'port_b_address_clock' (or any of the other port_b input clock parameters, since they
        //       must be the same)
        //
        // Additionally the following conclusions can also be drawn (see document for details): 
        //    5) ena0 and ena2 are associated with clk0
        //    6) ena1 and ena3 are associated with clk1
        //    7) eccstatus clock is determined by the 'eccstatus_clock' parameter, and may be none if
        //       unregistered
        //    8) The clr0/clr1 signals associated clocks are determined by the parameters: 
        //          'eccstatus_clear' (and by extension 'eccstatus_clock'),
        //          'port_a_data_out_clear', and ''port_b_data_out_clear
        //     
        if (strcmp(node_port->port_name, "portadataout") == 0) {
            //Check the port_a_data_out_clock parameter
            t_node_parameter* node_param = find_node_param(orig_node, "port_a_data_out_clock");
            if(node_param == NULL) {
                //Unspecified, assume none and map to node a
                port_to_node_map[node_port] = split_node_a;
                
            } else {
                assert(node_param->type == NODE_PARAMETER_STRING);

                if(strcmp(node_param->value.string_value, "clock0") == 0) {
                    port_to_node_map[node_port] = split_node_a;
                } else if (strcmp(node_param->value.string_value, "clock1") == 0) {
                    port_to_node_map[node_port] = split_node_b;
                } else {
                    port_to_node_map[node_port] = split_node_a;
                }

            }
            
        } else if (strcmp(node_port->port_name, "portbdataout") == 0) {
            //Check the port_b_data_out_clock parameter
            t_node_parameter* node_param = find_node_param(orig_node, "port_b_data_out_clock");
            assert(node_param != NULL);
            if(node_param == NULL) {
                //Unspecified, assume none and map to node b
                port_to_node_map[node_port] = split_node_b;
                
            } else {
                assert(node_param->type == NODE_PARAMETER_STRING);

                if(strcmp(node_param->value.string_value, "clock0") == 0) {
                    port_to_node_map[node_port] = split_node_a;
                } else if (strcmp(node_param->value.string_value, "clock1") == 0) {
                    port_to_node_map[node_port] = split_node_b;
                } else {
                    port_to_node_map[node_port] = split_node_b;
                }
            }

        } else if (strcmp(node_port->port_name, "clk0") == 0 || strstr(node_port->port_name, "porta") != NULL ||
                   strcmp(node_port->port_name, "ena0") == 0 || strcmp(node_port->port_name, "ena2") == 0) {
            //All remaining 'porta*', ena0 and ena2 ports are clocked by clk0
            port_to_node_map[node_port] = split_node_a;
            
        } else if (strcmp(node_port->port_name, "clk1") == 0 || strstr(node_port->port_name, "portb") != NULL ||
                   strcmp(node_port->port_name, "ena1") == 0 || strcmp(node_port->port_name, "ena3") == 0) {
            //Check the port_b_address_clock parameter, must be defined if port b is used.
            // Port B must be used since this block has multiple clocks
            t_node_parameter* node_param = find_node_param(orig_node, "port_b_address_clock");
            assert(node_param != NULL);
            assert(node_param->type == NODE_PARAMETER_STRING);

            if(strcmp(node_param->value.string_value, "clock0") == 0) {
                port_to_node_map[node_port] = split_node_a;
            } else if (strcmp(node_param->value.string_value, "clock1") == 0) {
                port_to_node_map[node_port] = split_node_b;
            } else {
                assert(0);
            }
            
        
        } else if (strcmp(node_port->port_name, "eccstatus") == 0) {
            //Check the eccstatus_clock parameters
            t_node_parameter* node_param = find_node_param(orig_node, "eccstatus_clock");
            assert(node_param != NULL);
            assert(node_param->type == NODE_PARAMETER_STRING);

            if(strcmp(node_param->value.string_value, "clock0") == 0) {
                port_to_node_map[node_port] = split_node_a;
            } else if (strcmp(node_param->value.string_value, "clock1") == 0) {
                port_to_node_map[node_port] = split_node_b;
            } else {
                assert(0);
            }
           
        } else if (strstr(node_port->port_name, "clr0") != NULL) {
            //Check the port_a_data_out_clear, port_b_data_out_clear, and eccstatus_clear parameters
            t_node_parameter* node_param = find_node_param(orig_node, "port_a_data_out_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear0") == 0) {
                    port_to_node_map[node_port] = split_node_a;
                    continue;
                }
            }
            node_param = find_node_param(orig_node, "port_b_data_out_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear0") == 0) {
                    port_to_node_map[node_port] = split_node_b;
                    continue;
                }
            }
            node_param = find_node_param(orig_node, "eccstatus_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear0") == 0) {
                    t_node_parameter* node_param_clock = find_node_param(orig_node, "eccstatus_clock");
                    assert(node_param_clock != NULL);
                    assert(node_param_clock->type == NODE_PARAMETER_STRING);

                    if(strcmp(node_param_clock->value.string_value, "clock0") == 0) {
                        port_to_node_map[node_port] = split_node_a;
                    } else if (strcmp(node_param_clock->value.string_value, "clock1") == 0) {
                        port_to_node_map[node_port] = split_node_b;
                    } else {
                        assert(0);
                    }
                    continue;
                }
            }
            assert(0);
        } else if (strstr(node_port->port_name, "clr0") != NULL) {
            //Check the port_a_data_out_clear, port_b_data_out_clear, and eccstatus_clear parameters
            t_node_parameter* node_param = find_node_param(orig_node, "port_a_data_out_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear1") == 0) {
                    port_to_node_map[node_port] = split_node_a;
                    continue;
                }
            }
            node_param = find_node_param(orig_node, "port_b_data_out_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear1") == 0) {
                    port_to_node_map[node_port] = split_node_b;
                    continue;
                }
            }
            node_param = find_node_param(orig_node, "eccstatus_clear");
            if(node_param != NULL) {
                assert(node_param->type == NODE_PARAMETER_STRING);
                if(strcmp(node_param->value.string_value, "clear1") == 0) {
                    t_node_parameter* node_param_clock = find_node_param(orig_node, "eccstatus_clock");
                    assert(node_param_clock != NULL);
                    assert(node_param_clock->type == NODE_PARAMETER_STRING);

                    if(strcmp(node_param_clock->value.string_value, "clock0") == 0) {
                        port_to_node_map[node_port] = split_node_a;
                    } else if (strcmp(node_param_clock->value.string_value, "clock1") == 0) {
                        port_to_node_map[node_port] = split_node_b;
                    } else {
                        assert(0);
                    }
                    continue;
                }
            }
            assert(0); //Should never get here


        } else {
            assert(0);
        }
    }

    return port_to_node_map;
}


//============================================================================================
//============================================================================================
void add_global_to_nonglobal_buffers(t_module* module, t_arch* arch, t_type_descriptor* arch_types, int num_types){

    //Identify architecture block pins which are globals
    t_global_ports global_ports = identify_primitive_global_pins(arch, arch_types, num_types);

    //Identify nets which are global signals since they are
    // connected to global pins
    t_global_nets global_nets = identify_global_nets(module, global_ports);

    //Create an STL map which identifies the driver of each global net
    t_net_driver_map net_driver_map = identify_net_drivers(module, arch, global_ports, global_nets);

    //Identify global to local assignment connections
    t_assign_vec_pair global_local_assignments = identify_global_local_assignments(module, global_nets, net_driver_map);

    //Identify global to local pin connections
    t_node_port_vec_pair global_local_pins = identify_global_local_pins(module, arch, global_ports, global_nets, net_driver_map);

    //Insert buffer cells between global nets and non-global pins
    int num_buffers_inserted = insert_fake_global_buffer_cells(module, global_local_assignments, global_local_pins);

    printf("\t\tInserted %d buffers\n", num_buffers_inserted);
}


t_global_ports identify_primitive_global_pins(t_arch* arch, t_type_descriptor* arch_types, int num_types){
    t_global_ports global_ports; 

    printf("\t\tPrimitive Global Pins:\n");
    //Add the top level pb_types
    for(int top_pb_type_index = 0; top_pb_type_index < num_types; top_pb_type_index++) {
        t_type_descriptor type = arch_types[top_pb_type_index];

        t_pb_type* top_level_pb_type = type.pb_type;

        if(top_level_pb_type != NULL) {
            t_pb_type_queue pb_type_queue;
            pb_type_queue.push(top_level_pb_type);

            while(!pb_type_queue.empty()) {
                //Process this pb_type
                t_pb_type* pb_type = pb_type_queue.front();
                pb_type_queue.pop();
               
                //A leaf (primitive) pb_type
                // this is where we should look for is_non_global_clock attributes
                if(pb_type->blif_model != NULL) {
                    for(int port_index = 0; port_index < pb_type->num_ports; port_index++) {
                        //A port on the primitive pb_type
                        t_port port = pb_type->ports[port_index];

                        //Check if the port is a global signal
                        if(port.is_non_clock_global) {

                            //Drop the '.subckt ' prefix and the '.opmode{stuff}' postfix
                            char* blif_model_prim_name = extract_primitive_name(pb_type->blif_model);

                            //See if this pb_type is already in the global_ports
                            t_global_ports::iterator gp_iter = global_ports.find(blif_model_prim_name);

                            t_str_ports* port_strings;
                            if(gp_iter != global_ports.end()) {
                                //Found the pb_type already in the array;
                                port_strings = gp_iter->second;

                                //Check if this port already exists
                                t_str_ports::iterator sp_iter = port_strings->find(port.name);
                                if(sp_iter != port_strings->end()) {
                                    //Exists so continue
                                    continue;
                                } else {
                                    //Doesn't exist so add the new port
                                    port_strings->insert(port.name);
                                }
                            } else {
                                //Make a new set of ports and insert the current one
                                port_strings = new t_str_ports;
                                port_strings->insert(port.name);

                                //Add a new map entry for this blif model, and the global ports
                                global_ports[blif_model_prim_name] = port_strings;

                            }

                            if(verbose_mode) {
                                printf("\t\t\t%s.%s\n", blif_model_prim_name, port.name);
                            }
                        }
                    }

                }
                    
                //Add any children pb_types in the child modes
                for(int child_mode_index = 0; child_mode_index < pb_type->num_modes; child_mode_index++) {
                    t_mode mode = pb_type->modes[child_mode_index];
                    for(int child_pb_type_index = 0; child_pb_type_index < mode.num_pb_type_children; child_pb_type_index++) {
                        t_pb_type* child_pb_type = &mode.pb_type_children[child_pb_type_index];
                        pb_type_queue.push(child_pb_type);
                    }
                }
            }
        }
    }

    printf("\t\t\tFound: %zu global primitive pins in architecture\n", global_ports.size());
    return global_ports;
}

char* extract_primitive_name(char* pb_type_blif_model) {
    //Primitive blif_model must start with .subckt
    assert(strncmp(BLIF_NAME_PREFIX, pb_type_blif_model, strlen(BLIF_NAME_PREFIX)) == 0);

    //Calculate the new size of the string excluding the prefix and any opmode data after the primitive name

    //First identify where the opmode starts
    int num_dots_seen = 0; //The number of '.' characters seen
    size_t string_index = 0;
    while(num_dots_seen < 2 && string_index < strlen(pb_type_blif_model)) {
        char blif_char = pb_type_blif_model[string_index];

        if(blif_char == '.') {
            num_dots_seen++;
        }
        string_index++;
    }
    //Two dots, in '.subckt stratixiv_ram_block.opmode{dual_port}'
    // or one dot in '.subckt stratixiv_lcell_comb'
    assert(num_dots_seen <= 2); 

    int new_str_len = string_index; //Including the .subckt prefix and NO \0 at end
    if(num_dots_seen == 2) new_str_len -= 1; //Drop the ending '.' before opmode
    new_str_len -= strlen(BLIF_NAME_PREFIX); //Subtract the prefix (including space character)

    //Remove the .subckt prefix
    char* blif_model_no_prefix_base = pb_type_blif_model+strlen(BLIF_NAME_PREFIX); //The new base, after the prefix

    char* blif_model_no_prefix = (char*) malloc(sizeof(char)*(new_str_len+1)); //Allocate space, +1 for \0 at end
    strncpy(blif_model_no_prefix, blif_model_no_prefix_base, new_str_len); //Copy new_str_len characters
    blif_model_no_prefix[new_str_len] = '\0'; //End of string
    return blif_model_no_prefix;
}

void print_map(t_global_ports global_ports) {
    for(t_global_ports::iterator gp_iter = global_ports.begin(); gp_iter != global_ports.end(); gp_iter++) {
        char* key = gp_iter->first;
        t_str_ports* value = gp_iter->second;
        printf("Key: '%s'\n", key);
        for(t_str_ports::iterator sp_iter = value->begin(); sp_iter != value->end(); sp_iter++) {
            printf("\tValue: '%s'\n", *sp_iter);
        }
    }
}

t_global_nets identify_global_nets(t_module* module, t_global_ports global_ports) {
    /*
     * Look through each node, if its type is a key in global_ports then it
     * has some global ports on it.
     *
     * If it has global ports, check each port to see if it is in the set of
     * port strings the value in the global_ports map.
     *
     * If the port is a global port, recored the net it is connected to, as this
     * will be a global net.
     */
    t_global_nets global_nets;

    printf("\t\tNets connected to global pins:\n");
    for(int node_index = 0; node_index < module->number_of_nodes; node_index++) {
        t_node* node = module->array_of_nodes[node_index];

        //Check the node type
        t_global_ports::iterator gp_iter = global_ports.find(node->type);

        if(gp_iter != global_ports.end()) {
            //Found it

            t_str_ports* port_strings = gp_iter->second;

            for(int port_index = 0; port_index < node->number_of_ports; port_index++) {
                t_node_port_association* node_port = node->array_of_ports[port_index];

                //Look up the port type
                t_str_ports::iterator ports_iter = port_strings->find(node_port->port_name);

                if(ports_iter != port_strings->end()) {
                    //Found it

                    t_pin_def* associated_net = node_port->associated_net;

                    t_global_nets::iterator gn_iter = global_nets.find(associated_net);
                    if(gn_iter == global_nets.end()) {
                        //Haven't seen this global net before

                        //Add a global net
                        global_nets.insert(associated_net);

                        if(verbose_mode) {
                            printf("\t\t\t%s (connected to at least %s.%s)\n", associated_net->name, node->type, node_port->port_name);
                        }
                    }

                }
            }

        }

    }

    printf("\t\t\tFound: %zu nets connected to at least one global primitive pin\n", global_nets.size());
    return global_nets;
}

t_net_driver_map identify_net_drivers(t_module* module, t_arch* arch, t_global_ports global_ports, t_global_nets global_nets) {
    t_net_driver_map net_driver_map;

    int total_num_global_sinks = 0;
    int total_num_local_sinks = 0;
    int total_num_assign_sinks = 0;
    int total_num_sources = 0;

    printf("\t\tIdentify Globally Connected Net Drivers:\n");

    for(t_global_nets::iterator gn_iter = global_nets.begin(); gn_iter != global_nets.end(); gn_iter++) {
        t_pin_def* global_net = *gn_iter;

        int num_global_sinks = 0;
        int num_local_sinks = 0;
        int num_assign_sinks = 0;
        int num_sources = 0;
        
        t_net_driver net_driver;
        
        if(verbose_mode) {
            printf("\t\t\t%s (type %d):\n", global_net->name, global_net->type);
        }

        for(int assign_index = 0; assign_index < module->number_of_assignments; assign_index++) {
            t_assign* assign = module->array_of_assignments[assign_index];
            
            if(assign->source == global_net) {
                num_assign_sinks++;
            }

            if(assign->target == global_net) {
                num_sources++;

                //Save the driver type
                net_driver.driver_type = ASSIGN;
                net_driver.driver.assign = assign;
                net_driver.is_global = FALSE; //Assignments by default are never global

            }

        }
        for(int node_index = 0; node_index < module->number_of_nodes; node_index++) {
            t_node* node = module->array_of_nodes[node_index];

            //Check each port on the node
            for(int port_index = 0; port_index < node->number_of_ports; port_index++) {
                t_node_port_association* node_port = node->array_of_ports[port_index];

                if(node_port->associated_net == global_net) {
                    /*
                     *  Since these VQM nodes are blackboxes
                     *  we do not know whether the node_port
                     *  is an input or an output.
                     *
                     *  This can be determined by querying the
                     *  architecture file.
                     */

                    //Architecture pointers
                    t_model* arch_model;
                    t_model_ports* arch_model_port;

                    //Find the model
                    arch_model = find_model_in_architecture(arch->models, node);

                    //Find the architecure model port
                    arch_model_port = find_port_in_architecture_model(arch_model, node_port);

                    if (arch_model_port->dir == IN_PORT) {
                        if(is_node_port_global(node, node_port, global_ports)) {
                            num_global_sinks++;
                        } else {
                            num_local_sinks++;
                        }
                    } else if (arch_model_port->dir == OUT_PORT) {
                        num_sources++;

                        //Save the driver type
                        net_driver.driver_type = NODE_PORT;
                        net_driver.driver.node_port.node_port = node_port;
                        net_driver.driver.node_port.node = node;
                        net_driver.is_global = is_node_port_global(node, node_port, global_ports);
                        

                    } else {
                        cout << "Error: invalid port type for port'" << node_port->port_name << "' on model '" << node->type << "' in architecture file\n";
                        exit(1);
                    }
                }
            }
        }
        total_num_sources += num_sources;
        total_num_global_sinks += num_global_sinks;
        total_num_local_sinks += num_local_sinks;
        total_num_assign_sinks += num_assign_sinks;

        if(verbose_mode) {
            printf("\t\t\t\t%d driver(s), %d global_sink(s), %d local_sink(s), %d assign_sink(s)\n", num_sources, num_global_sinks, num_local_sinks, num_assign_sinks);
        }
        assert(num_sources == 1);


        /*
         *  Reconsider making an assignment drive a global, if it is mostly connected to globals
         */
        if(!net_driver.is_global) {
            float global_sink_fraction = (float) num_global_sinks / (num_global_sinks + num_local_sinks + num_assign_sinks);

            if (global_sink_fraction >= PROMOTE_NET_GLOBAL_SINK_FRAC) {
                printf("\t\t\t\tPromoting driver to global (Frac global sinks: %.2f). \n", global_sink_fraction);
                net_driver.is_global = TRUE; 
            }
        }

        if(verbose_mode) {
            if(net_driver.driver_type == NODE_PORT) {
                printf("\t\t\t\tDriver: Node Port %s.%s\n", net_driver.driver.node_port.node->type, net_driver.driver.node_port.node_port->port_name);

            } else if (net_driver.driver_type == ASSIGN) {
                if(net_driver.driver.assign->source != NULL) {
                    printf("\t\t\t\tDriver: Assignment from %s\n", net_driver.driver.assign->source->name);

                } else {
                    printf("\t\t\t\tDriver: Assignment from constant (%d)\n", net_driver.driver.assign->value);
                }
            }

            if(net_driver.is_global) {
                printf("\t\t\t\tDriver Type: GLOBAL\n");
            } else {
                printf("\t\t\t\tDriver Type: LOCAL\n");
            }
        }
        //Place in map
        net_driver_map[global_net] = net_driver;
    }

    printf("\t\t\tFound: %d driver(s), %d global_sink(s), %d local_sink(s), %d assign_sink(s) over %zu globally connected nets\n", total_num_sources, total_num_global_sinks, total_num_local_sinks, total_num_assign_sinks, global_nets.size()); 
    return net_driver_map;
}

boolean is_node_port_global(t_node* node, t_node_port_association* node_port, t_global_ports global_ports) {
    boolean node_port_is_global = FALSE;

    //Check if the node type has any global ports
    t_global_ports::iterator gp_iter = global_ports.find(node->type);
    if(gp_iter != global_ports.end()){
        //It does have global ports
        t_str_ports* global_port_strings = gp_iter->second;

        //See if node_port is in the list of global ports
        t_str_ports::iterator gps_iter = global_port_strings->find(node_port->port_name);
        if(gps_iter != global_port_strings->end()) {
            //It is in the list therefore, node_port is global
            node_port_is_global = TRUE;
        }
    }

    return node_port_is_global;
}

t_assign_vec_pair identify_global_local_assignments(t_module* module, t_global_nets global_nets, t_net_driver_map net_driver_map){
    t_assign_vec_pair global_local_assignments;
    
    printf("\t\tGlobal <-> Local assignments:\n");

    printf("\t\t\tScanning for global assignments...\n");
    //Check assignments
    for(int assign_index = 0; assign_index < module->number_of_assignments; assign_index++) {
        t_assign* assignment = module->array_of_assignments[assign_index];
        
        t_global_nets::iterator gn_iter; //Iterator used for search

        //Check the source
        gn_iter = global_nets.find(assignment->source);
        if(gn_iter != global_nets.end()) {
            t_pin_def* associated_net = *gn_iter;

            t_net_driver net_driver = net_driver_map[associated_net];

            if(net_driver.is_global) {
                if(verbose_mode) {
                    printf("\t\t\tFound assignment using globally driven net as source! (%s -> %s)\n", assignment->source->name, assignment->target->name);
                }
                global_local_assignments.global_to_local.push_back(assignment);
            }
        }
        
        //Check the target
        gn_iter = global_nets.find(assignment->target);
        if(gn_iter != global_nets.end()) {
            t_pin_def* associated_net = *gn_iter;

            t_net_driver net_driver = net_driver_map[associated_net];

            //An assignment with target net that is marked a global has been marked for promotion
            //So save this assignment to latter insert a l2g buffer after it
            if(net_driver.is_global) {
                if(verbose_mode) {
                    printf("\t\t\tFound promoted assignment which should drive a global! (%s -> %s)\n", assignment->source->name, assignment->target->name);
                }
                global_local_assignments.local_to_global.push_back(assignment);

            }
        }

    }

    printf("\t\t\tFound %zu assignments with globally driven sources, and %zu assignments driving globals\n", global_local_assignments.global_to_local.size(), global_local_assignments.local_to_global.size());
    return global_local_assignments;
}

boolean is_net_global(t_pin_def* net, t_global_nets global_nets) {
    boolean net_is_global = FALSE;
    
    if(global_nets.find(net) != global_nets.end()) {
        net_is_global = TRUE;
    }
    
    return net_is_global;
}

t_node_port_vec_pair identify_global_local_pins(t_module* module, t_arch* arch, t_global_ports global_ports,
                                                t_global_nets global_nets, t_net_driver_map net_driver_map){
    t_node_port_vec_pair global_local_pins;

    int num_drivers = 0;
    int num_driver_sink_globals = 0;
    int num_driver_sink_locals = 0;

    printf("\t\tGlobal <-> Local Pins\n");

    printf("\t\t\tScanning netlist for global connections...\n");
    //Check each node
    for(int node_index = 0; node_index < module->number_of_nodes; node_index++) {
        t_node* node = module->array_of_nodes[node_index];

        //Check each port on the node
        for(int port_index = 0; port_index < node->number_of_ports; port_index++) {
            t_node_port_association* node_port = node->array_of_ports[port_index];
            t_pin_def* associated_net = node_port->associated_net;

            if(is_net_global(associated_net, global_nets)) {
                
                //Figure out if node_port is global
                boolean node_port_is_global = is_node_port_global(node, node_port, global_ports);

                //The net driver
                t_net_driver net_driver = net_driver_map[associated_net];

                //1) node_port is the net driver
                if(net_driver.driver_type == NODE_PORT && net_driver.driver.node_port.node_port == node_port) {
                    //Pass
                    //printf("\t\t\t%s.%s (%s) is %s driver\n", node->type, node_port->port_name, node->name, associated_net->name);
                    num_drivers++;
                
                //2) node_port is the same type (global or local) as the net_driver
                } else if (net_driver.is_global == node_port_is_global){
                    //Pass
                    //printf("\t\t\t%s.%s (%s) on net %s is same type (global/local) as driver\n", node->type, node_port->port_name, node->name, associated_net->name);
                    if(net_driver.is_global) {
                        num_driver_sink_globals++;
                    } else {
                        num_driver_sink_locals++;
                    }
                
                //3) Different global/local types
                } else {
                    //3a) net_driver is global
                    if(net_driver.is_global) {
                        assert(node_port_is_global == FALSE);

                        //Add the the appropriate list, so that a g2l buffer is added before node_port
                        global_local_pins.global_to_local.push_back(node_port);

                        if(verbose_mode) {
                            //printf("\t\t\tFound local pin using globally driven net as source [adding g2l]: %s -> %s.%s (%s)\n", associated_net->name, node->type, node_port->port_name, node->name);
                        }


                    //3b) net_driver is not global
                    } else {
                        assert(node_port_is_global == TRUE);

                        //Add the the appropriate list, so that a l2g buffer is added before node_port
                        global_local_pins.local_to_global.push_back(node_port);

                        if(verbose_mode) {
                            //printf("\t\t\tFound global pin using locally driven net as source [adding l2g]: %s -> %s.%s (%s)\n", associated_net->name, node->type, node_port->port_name, node->name);
                        }
                    }

                }
            }
        }
    }
    printf("\t\t\t Found %d driver(s)/net(s) making %d global-global conns and %d local-local conns\n", num_drivers, num_driver_sink_globals, num_driver_sink_locals);
    return global_local_pins;
}

int insert_fake_global_buffer_cells(t_module* module, t_assign_vec_pair global_local_assign, t_node_port_vec_pair global_local_pins) {
    int num_buffers_inserted = 0;

    printf("\t\tInserting Buffers...\n");
    for(t_assign_vec::iterator avec_iter = global_local_assign.global_to_local.begin(); avec_iter != global_local_assign.global_to_local.end(); avec_iter++) {
        t_assign* global_to_local_assign = *avec_iter;
        /*
         * Global to Local connections performed by assignments have a fake
         * buffer inserted before the assignment
         *  e.g.
         *       assign local_net = global_net;
         *               
         *               becomes
         *
         *       fake_gbuf fake_gbuf_inst1 (
         *           .i (global_net) //buffer input
         *           .o (global_net_localized1) 
         *       );
         * 
         *       assign local_net = global_net_localized1;
         * 
         */

        //Connect the new buffer output to the assignment
        global_to_local_assign->source = add_new_input_buffer(module, global_to_local_assign->source, G2L_BUFFER, num_buffers_inserted);
        num_buffers_inserted++;
    }

    for(t_assign_vec::iterator avec_iter = global_local_assign.local_to_global.begin(); avec_iter != global_local_assign.local_to_global.end(); avec_iter++) {
        t_assign* local_to_global_assign = *avec_iter;
        /*
         * Local to Global connections performed by assignments have a fake
         * buffer inserted after the assignment
         *  e.g.
         *       assign global_net = local_net;
         *               
         *               becomes
         * 
         *       assign preglobal_net = local_net;
         * 
         *       fake_gbuf fake_gbuf_inst1 (
         *           .i (preglobal_net) //buffer input
         *           .o (global_net) 
         *       );
         */

        t_pin_def* global_net = local_to_global_assign->target;
        t_pin_def* local_net = local_to_global_assign->source;

        //Allocate new preglobal wire
        char str_buf_cnt[50];
        snprintf(str_buf_cnt, sizeof(str_buf_cnt), FAKE_L2G_BUFFER_NEW_NET_PREGLOBAL_FORMAT, num_buffers_inserted);
        t_pin_def* preglobal_net = duplicate_pin(module, global_net->name, str_buf_cnt, local_net, PIN_WIRE); //TODO: allocate it

        //Set assignment target to the preglobal net
        local_to_global_assign->target = preglobal_net; 

        //Allocate the new buffer
        t_node* new_buffer = add_buffer(module, L2G_BUFFER, num_buffers_inserted);

        //Connect the buffer input/output
        assert(new_buffer->number_of_ports == 2);
        new_buffer->array_of_ports[0]->associated_net = preglobal_net;
        new_buffer->array_of_ports[1]->associated_net = global_net;

        num_buffers_inserted++;
    }

    for(t_node_port_vec::iterator npvec_iter = global_local_pins.global_to_local.begin(); npvec_iter != global_local_pins.global_to_local.end(); npvec_iter++) {
        t_node_port_association* global_to_local_node_port = *npvec_iter;
        /*
         * Global to Local connections to a pin have a fake
         * buffer inserted before the pin
         *  e.g.
         *      some_node_type some_node_type_inst1(
         *              ...
         *          .local_in_port1(global_net),
         *              ...
         *      );
         *               
         *               becomes
         *
         *      fake_gbuf fake_g2l_buf_inst1 (
         *          .i (global_net) //buffer input
         *          .o (global_net_localized1) 
         *      );
         * 
         *      some_node_type some_node_type_inst1(
         *              ...
         *          .local_in_port1(global_net_localized1),
         *              ...
         *      );
         * 
         */

        //Connect the new buffer output to the pin
        global_to_local_node_port->associated_net = add_new_input_buffer(module, global_to_local_node_port->associated_net, G2L_BUFFER, num_buffers_inserted);
        num_buffers_inserted++;
    }
    
    for(t_node_port_vec::iterator npvec_iter = global_local_pins.local_to_global.begin(); npvec_iter != global_local_pins.local_to_global.end(); npvec_iter++) {
        t_node_port_association* local_to_global_node_port = *npvec_iter;
        /*
         * Local to Global connections performed to a pin have a fake
         * buffer inserted before the pin
         *  e.g.
         *      some_node_type some_node_type_inst1(
         *              ...
         *          .global_in_port(local_net),
         *              ...
         *      );
         *               
         *               becomes
         *
         *      fake_gbuf fake_l2g_buf_inst1 (
         *          .i (local_net) //buffer input
         *          .o (local_net_globalized1) 
         *      );
         * 
         *      some_node_type some_node_type_inst1(
         *              ...
         *          .global_in_port(local_net_globalized1),
         *              ...
         *      );
         * 
         */

        //Connect the new buffer output to the pin
        local_to_global_node_port->associated_net = add_new_input_buffer(module, local_to_global_node_port->associated_net, L2G_BUFFER, num_buffers_inserted);
        num_buffers_inserted++;
    }

    printf("\t\t\tBuffered: %zu global-local pin conns, %zu local-global pin conns, %zu global-local assignment conns, %zu local-global assignment conns.\n", global_local_pins.global_to_local.size(),   global_local_pins.local_to_global.size(), global_local_assign.global_to_local.size(), global_local_assign.local_to_global.size());
    return num_buffers_inserted;
}

t_pin_def* duplicate_pin(t_module* module, const char* name_prefix, const char* name_base, t_pin_def* src_pin, t_pin_def_type pin_type) {
    t_pin_def* new_pin = add_pin(module);

    //New name
    new_pin->name = prefix_string(name_prefix, name_base);

    //Copy values
    new_pin->left = src_pin->left;
    new_pin->right = src_pin->right;
    new_pin->indexed = src_pin->indexed;
    new_pin->type = pin_type;

    return new_pin;
}

t_pin_def* add_pin(t_module* module) {
    t_pin_def* new_net = (t_pin_def*) malloc(sizeof(t_pin_def));
   
    //Insert the new wire in the module
    module->number_of_pins++;
    module->array_of_pins = (t_pin_def**) realloc(module->array_of_pins, sizeof(t_pin_def**)*module->number_of_pins);
    module->array_of_pins[module->number_of_pins-1] = new_net;

    return new_net;
}

t_node* add_buffer(t_module* module, t_buffer_type buffer_type, int num_buffers_inserted) {
    const char* buffer_type_str = FAKE_G2L_BUFFER_TYPE;
    const char* buffer_input_port_name_str = FAKE_G2L_BUFFER_INPUT_PORT_NAME;
    const char* buffer_output_port_name_str = FAKE_G2L_BUFFER_OUTPUT_PORT_NAME;

    if(buffer_type == L2G_BUFFER) {
        buffer_type_str = FAKE_L2G_BUFFER_TYPE;
        buffer_input_port_name_str = FAKE_L2G_BUFFER_INPUT_PORT_NAME;
        buffer_output_port_name_str = FAKE_L2G_BUFFER_OUTPUT_PORT_NAME;
    }


    //Allocate a new buffer node
    t_node* new_buffer = (t_node*) malloc(sizeof(t_node));
    assert(new_buffer != NULL);

    //Reallocate space in the array for the new node
    module->number_of_nodes++;
    module->array_of_nodes = (t_node**) realloc(module->array_of_nodes, sizeof(t_node*)*module->number_of_nodes);
    assert(module->array_of_nodes != NULL);

    //Insert the new buffer into the array
    module->array_of_nodes[module->number_of_nodes-1] = new_buffer;



    //Initialize the new buffer
    new_buffer->type = prefix_string(buffer_type_str, "");
    
    //Generate the buffer count as a string
    char str_buf_cnt[50];
    snprintf(str_buf_cnt, sizeof(str_buf_cnt), FAKE_BUFFER_INST_FORMAT, num_buffers_inserted);
    new_buffer->name = prefix_string(buffer_type_str, str_buf_cnt);

    new_buffer->array_of_params = NULL;
    new_buffer->number_of_params = 0;

    new_buffer->number_of_ports = 2;
    new_buffer->array_of_ports = (t_node_port_association**) malloc(sizeof(t_node_port_association**)*new_buffer->number_of_ports);

    //Input port
    t_node_port_association* in_port = (t_node_port_association*) malloc(sizeof(t_node_port_association));
    in_port->port_name = prefix_string(buffer_input_port_name_str,"");
    in_port->port_index = -1; //TODO: check this is correct
    in_port->associated_net = NULL;
    in_port->wire_index = 0; //TODO: check this is correct

    //Add port    
    new_buffer->array_of_ports[0] = in_port;
    
    //Output port
    t_node_port_association* out_port = (t_node_port_association*) malloc(sizeof(t_node_port_association));
    out_port->port_name = prefix_string(buffer_output_port_name_str,"");
    out_port->port_index = -1; //TODO: check this is correct
    out_port->associated_net = NULL;
    out_port->wire_index = 0; //TODO: check this is correct
    
    //Add port
    new_buffer->array_of_ports[1] = out_port;

    return new_buffer;
}


t_pin_def* add_new_input_buffer(t_module* module, t_pin_def* global_src_net, t_buffer_type buffer_type, int num_buffers_inserted) {

    t_node* new_buffer = add_buffer(module, buffer_type, num_buffers_inserted);

    const char* buffer_output_net_format = FAKE_G2L_BUFFER_NEW_NET_FORMAT;
    if(buffer_type == L2G_BUFFER) {
        buffer_output_net_format = FAKE_L2G_BUFFER_NEW_NET_FORMAT;
    }

    //New wire segment at buffer output
    char str_buf_cnt[50];
    snprintf(str_buf_cnt, sizeof(str_buf_cnt), buffer_output_net_format, num_buffers_inserted); //Add number of buffers to ensure that the new wire seg name is unique
    t_pin_def* localized_global_src_net = duplicate_pin(module, global_src_net->name, str_buf_cnt, global_src_net, PIN_WIRE); //Allocate the new wire segment

    //Set the input/output nets
    new_buffer->array_of_ports[0]->associated_net = global_src_net;
    new_buffer->array_of_ports[1]->associated_net = localized_global_src_net;
        

    //Return the new buffer's output net
    return localized_global_src_net;
}
