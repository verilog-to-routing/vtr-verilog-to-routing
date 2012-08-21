//============================================================================================
//				INCLUDES
//============================================================================================
#include "../include/preprocess.h"


//============================================================================================
//			INTERNAL FUNCTION DECLARATIONS
//============================================================================================
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

//============================================================================================
//============================================================================================
void preprocess_netlist(t_module* module, t_arch* arch){
    /*
     * Put all netlist pre-processing function calls here
     */

    //Clean up 'inout' pins by replacing them with
    // seperate 'input' and 'output' pins
    cout << "\t>> Preprocessing Netlist to decompose inout pins\n";
    decompose_inout_pins(module, arch);
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

    //Find the correct model, by name matching
    t_model* arch_model = arch_models;
    while((arch_model) && (strcmp(node->type, arch_model->name) != 0)) {
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
