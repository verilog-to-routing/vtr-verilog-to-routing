/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SubcktBLIFReader.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "odin_ii.h"
#include "odin_util.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "ast_util.h"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"


SubcktBLIFReader::SubcktBLIFReader(): BLIF::Reader() {}

SubcktBLIFReader::~SubcktBLIFReader() = default;

void* SubcktBLIFReader::__read() {
    printf("Reading top level module\n");
    fflush(stdout);

    /* Extracting the netlist by reading the blif file */
    printf("Reading blif netlist...");
    fflush(stdout);

    int position = -1;
    double time = wall_time();
    // A cache of hard block models indexed by name. As each one is read, it's stored here to be used again.
    hard_block_models* models = create_hard_block_models();
    printf("\n");
    char buffer[READ_BLIF_BUFFER];
    while (vtr::fgets(buffer, READ_BLIF_BUFFER, file) && read_tokens(buffer, models)) { // Print a progress bar indicating completeness.
        position = print_progress_bar((++line_count) / (double)num_lines, position, 50, wall_time() - time);
    }
    free_hard_block_models(models);
    /* Now look for high-level signals */
    rb_look_for_clocks();
    // We the estimate of completion is rough...make sure we end up at 100%. ;)
    print_progress_bar(1.0, position, 50, wall_time() - time);
    printf("-------------------------------------\n");
    fflush(stdout);

    // Outputs netlist graph.
    check_netlist(blif_netlist);
    // delete output_nets_hash;
    fclose(file);
    return static_cast<void*>(blif_netlist);
}


/**
 * #############################################################################################################################
 * #################################################### PROTECTED METHODS ######################################################
 * #############################################################################################################################
*/

/*---------------------------------------------------------------------------------------------
 * (function: read_tokens)
 *
 * Parses the given line from the blif file. Returns true if there are more lines
 * to read.
 *-------------------------------------------------------------------------------------------*/
 int SubcktBLIFReader::read_tokens(char* buffer, hard_block_models* models) {
    /* Figures out which, if any token is at the start of this line and *
     * takes the appropriate action.                                    */
    char* token = vtr::strtok(buffer, TOKENS, file, buffer);

    if (token) {
        if (skip_reading_bit_map && ((token[0] == '0') || (token[0] == '1') || (token[0] == '-'))) {
            dum_parse(buffer);
        } else {
            skip_reading_bit_map = false;
            if (strcmp(token, ".model") == 0) {
                model_parse(buffer); // store the scope model name for in/out name processing
                /* create the top level module */
                rb_create_top_driver_nets(blif_netlist->identifier);
            } else if (strcmp(token, ".inputs") == 0) {
                add_top_input_nodes(blif_netlist->identifier, NULL); // create the top input nodes
            } else if (strcmp(token, ".outputs") == 0) {
                rb_create_top_output_nodes(blif_netlist->identifier, NULL); // create the top output nodes
            } else if (strcmp(token, ".names") == 0) {
                create_internal_node_and_driver(blif_netlist->identifier);
            } else if (strcmp(token, ".subckt") == 0) {
                create_hard_block_nodes(blif_netlist->identifier, models);
            } else if (strcmp(token, ".end") == 0) {
                // Marks the end of the main module of the blif
                // Call function to hook up the nets
                hook_up_nets();
                //return (configuration.blif_type ! = blif_type_e::_ODIN_BLIF);
                return false;
            }
        }
    }
    return true;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: hook_up_nets)
 * 
 * @brief find the output nets and add the corresponding nets
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::hook_up_nets() {
    nnode_t** node_sets[] = {blif_netlist->internal_nodes, blif_netlist->ff_nodes, blif_netlist->top_output_nodes};
    int counts[] = {blif_netlist->num_internal_nodes, blif_netlist->num_ff_nodes, blif_netlist->num_top_output_nodes};
    int num_sets = 3;

    /* hook all the input pins in all the internal nodes to the net */
    int i;
    for (i = 0; i < num_sets; i++) {
        int j;
        for (j = 0; j < counts[i]; j++) {
            nnode_t* node = node_sets[i][j];
            SubcktBLIFReader::hook_up_node(node);
        }
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: hook_up_node)
 * 
 * @brief Connect the given node's input pins to their corresponding nets by
 * looking each one up in the output_nets_sc.
 * 
 * @param node represents one of netlist internal nodes or ff nodes or top output nodes
 * ---------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::hook_up_node(nnode_t* node) {
    int j;
    for (j = 0; j < node->num_input_pins; j++) {
        npin_t* input_pin = node->input_pins[j];

        nnet_t* output_net = (nnet_t*)output_nets_hash->get(input_pin->name);

        if (!output_net)
            error_message(PARSE_YOSYS_BLIF, my_location, "Error: Could not hook up the pin %s: not available.", input_pin->name);

        add_fanout_pin_to_net(output_net, input_pin);
    }
}

/**
 *------------------------------------------------------------------------------------------- 
 * (function:create_hard_block_nodes)
 * 
 * @brief to create the hard block nodes
 * 
 * @param models list of hard block models
 * @param file pointer to the input blif file
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::create_hard_block_nodes(const char* name_prefix, hard_block_models* models) {
    char buffer[READ_BLIF_BUFFER];
    char* subcircuit_name = vtr::strtok(NULL, TOKENS, file, buffer);

    // Name the node subcircuit_name~hard_block_number so that the name is unique.
    static long hard_block_number = 0;
    odin_sprintf(buffer, "%s~%ld", subcircuit_name, hard_block_number++);
    char* unique_subckt_name = make_full_ref_name(buffer, NULL, NULL, NULL, -1);
    
    /* storing the names on the formal-actual parameter */
    char* token;
    int count = 0;
    // Contains strings of the form port[pin]=port~pin
    char** names_parameters = NULL;
    while ((token = vtr::strtok(NULL, TOKENS, file, buffer)) != NULL) {
        names_parameters = (char**)vtr::realloc(names_parameters, sizeof(char*) * (count + 1));
        names_parameters[count++] = vtr::strdup(token);
    }

    // Split the name parameters at the equals sign.
    char** mappings = (char**)vtr::calloc(count, sizeof(char*));
    char** names = (char**)vtr::calloc(count, sizeof(char*));
    int i = 0;
    for (i = 0; i < count; i++) {
        mappings[i] = vtr::strdup(strtok(names_parameters[i], "="));
        names[i] = resolve_signal_name_based_on_blif_type(name_prefix, strtok(NULL, "="));
    }

    // Sort the mappings.
    qsort(mappings, count, sizeof(char*), compare_hard_block_pin_names);

    for (i = 0; i < count; i++)
        vtr::free(names_parameters[i]);

    vtr::free(names_parameters);

    // Index the mappings in a hard_block_ports struct.
    hard_block_ports* ports = get_hard_block_ports(mappings, count);

    // Look up the model in the models cache.
    hard_block_model* model = NULL;
    if ((subcircuit_name != NULL) /* && (!(model = get_hard_block_model(subcircuit_name, ports, models))) */) {
        // If the model isn's present, scan ahead and find it.
        model = read_hard_block_model(subcircuit_name, unique_subckt_name, ports, models);
        // Add it to the cache.
        add_hard_block_model(model, ports, models);
    }

     /* 
     * since yosys does not see the in/out of the model while instantiation,
     * it creates ports using numeric names, i.e. $1[] $2[] ..., in this case, 
     * we need to update the port names and mapping_index hashtable  
     * */
    harmonize_mappings_with_model_pin_names(model, mappings);

    // Associate mappings with their connections.
    Hashtable* mapping_index = associate_names(mappings, names, count);

    for (i = 0; i < count; i++) {
        vtr::free(mappings[i]);
        mappings[i] = NULL;
    }

    vtr::free(mappings);
    mappings = NULL;

    nnode_t* new_node = allocate_nnode(my_location);

    new_node->name = unique_subckt_name;

    // Determine the type of hard block.
    char subcircuit_name_prefix[] = {subcircuit_name[0], subcircuit_name[1], subcircuit_name[2], subcircuit_name[3], subcircuit_name[4], '\0'};

    if (!strcmp(subcircuit_name, "$mul") || !strcmp(subcircuit_name_prefix, "$mul")) {
        new_node->type = MULTIPLY;
    } else if (!strcmp(subcircuit_name, "$add") || !strcmp(subcircuit_name_prefix, "$add")) {
        new_node->type = ADD;

    } else if (!strcmp(subcircuit_name, "$sub") || !strcmp(subcircuit_name_prefix, "$sub")) {
        new_node->type = MINUS;

    } else if (!strcmp(subcircuit_name, "$dff") || !strcmp(subcircuit_name_prefix, "$dff")) {
        new_node->type = FF_NODE;
    } else if (!strcmp(subcircuit_name, "$mux") || !strcmp(subcircuit_name_prefix, "$mux")) {
        new_node->type = MULTI_BIT_MUX_2;
    } else {
        new_node->type = GENERIC; // TODO resolve .model into lower logic
    }
    // vtr::free(subcircuit_name_prefix);

    /* Add input and output ports to the new node. */
    {
        hard_block_ports* p;
        p = model->input_ports;
        for (i = 0; i < p->count; i++)
            add_input_port_information(new_node, p->sizes[i]);

        p = model->output_ports;
        for (i = 0; i < p->count; i++)
            add_output_port_information(new_node, p->sizes[i]);
    }

    // Allocate pins positions.
    if (model->inputs->count > 0)
        allocate_more_input_pins(new_node, model->inputs->count);
    if (model->outputs->count > 0)
        allocate_more_output_pins(new_node, model->outputs->count);

    // Add input pins.
    for (i = 0; i < model->inputs->count; i++) {
        char* mapping = model->inputs->names[i];
        char* name = (char*)mapping_index->get(mapping);

        if (!name)
            error_message(PARSE_YOSYS_BLIF, my_location, "Invalid hard block mapping: %s", mapping);

        npin_t* new_pin = allocate_npin();
        new_pin->name = vtr::strdup(name);
        new_pin->type = INPUT;
        new_pin->mapping = get_hard_block_port_name(mapping);

        add_input_pin_to_node(new_node, new_pin, i);
    }

    // Add output pins, nets, and index each net.
    for (i = 0; i < model->outputs->count; i++) {
        char* mapping = model->outputs->names[i];
        char* name = (char*)mapping_index->get(mapping);

        if (!name) error_message(PARSE_YOSYS_BLIF, my_location, "Invalid hard block mapping: %s", model->outputs->names[i]);

        npin_t* new_pin = allocate_npin();
        new_pin->name = vtr::strdup(name);
        new_pin->type = OUTPUT;
        new_pin->mapping = get_hard_block_port_name(mapping);

        add_output_pin_to_node(new_node, new_pin, i);

        nnet_t* new_net = allocate_nnet();
        new_net->name = vtr::strdup(name);

        add_driver_pin_to_net(new_net, new_pin);

        // Index the net by name.
        output_nets_hash->add(name, new_net);
    }

    // Create a fake ast node.
    new_node->related_ast_node = create_node_w_type(HARD_BLOCK, my_location);
    new_node->related_ast_node->children = (ast_node_t**)vtr::calloc(1, sizeof(ast_node_t*));
    new_node->related_ast_node->identifier_node = create_tree_node_id(vtr::strdup(subcircuit_name), my_location);

    /*add this node to blif_netlist as an internal node */
    blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
    blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;

    free_hard_block_ports(ports);
    mapping_index->destroy_free_items();
    delete mapping_index;
    vtr::free(names);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function:create_internal_node_and_driver)
 * 
 * @brief to create an internal node and driver from that node
 * 
 * @param name_prefix
 *-------------------------------------------------------------------------------------------*/

 void SubcktBLIFReader::create_internal_node_and_driver(const char* name_prefix) {
    /* Storing the names of the input and the final output in array names */
    char* ptr = NULL;
    char* resolved_name = NULL;
    char** names = NULL; // stores the names of the input and the output, last name stored would be of the output
    int input_count = 0;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        names = (char**)vtr::realloc(names, sizeof(char*) * (input_count + 1));
        resolved_name = resolve_signal_name_based_on_blif_type(name_prefix, ptr);
        names[input_count++] = vtr::strdup(resolved_name);
        vtr::free(resolved_name);
    }

    /* assigning the new_node */
    nnode_t* new_node = allocate_nnode(my_location);
    new_node->related_ast_node = NULL;

    /* gnd vcc unconn already created as top module so ignore them */
    if (
        !strcmp(names[input_count - 1], "gnd")
        || !strcmp(names[input_count - 1], "vcc")
        || !strcmp(names[input_count - 1], "unconn")) {
        skip_reading_bit_map = true;
        free_nnode(new_node);
    } else {
        /* assign the node type by seeing the name */
        operation_list node_type = (operation_list)assign_node_type_from_node_name(names[input_count - 1]);

        if (node_type != GENERIC) {
            new_node->type = node_type;
            skip_reading_bit_map = true;
        }
        /* Check for GENERIC type , change the node by reading the bit map */
        else if (node_type == GENERIC) {
            new_node->type = (operation_list)read_bit_map_find_unknown_gate(input_count - 1, new_node);
            skip_reading_bit_map = true;
        }

        /* allocate the input pin (= input_count-1)*/
        if (input_count - 1 > 0) // check if there is any input pins
        {
            allocate_more_input_pins(new_node, input_count - 1);

            /* add the port information */
            if (new_node->type == MUX_2) {
                add_input_port_information(new_node, (input_count - 1) / 2);
                add_input_port_information(new_node, (input_count - 1) / 2);
            } else {
                int i;
                for (i = 0; i < input_count - 1; i++)
                    add_input_port_information(new_node, 1);
            }
        }

        /* add names and type information to the created input pins */
        int i;
        for (i = 0; i <= input_count - 2; i++) {
            npin_t* new_pin = allocate_npin();
            new_pin->name = vtr::strdup(names[i]);
            new_pin->type = INPUT;
            add_input_pin_to_node(new_node, new_pin, i);
        }

        /* add information for the intermediate VCC and GND node (appears in ABC )*/
        if (new_node->type == GND_NODE) {
            allocate_more_input_pins(new_node, 1);
            add_input_port_information(new_node, 1);

            npin_t* new_pin = allocate_npin();
            new_pin->name = vtr::strdup(GND_NAME);
            new_pin->type = INPUT;
            add_input_pin_to_node(new_node, new_pin, 0);
        }

        if (new_node->type == VCC_NODE) {
            allocate_more_input_pins(new_node, 1);
            add_input_port_information(new_node, 1);

            npin_t* new_pin = allocate_npin();
            new_pin->name = vtr::strdup(VCC_NAME);
            new_pin->type = INPUT;
            add_input_pin_to_node(new_node, new_pin, 0);
        }

        /* allocate the output pin (there is always one output pin) */
        allocate_more_output_pins(new_node, 1);
        add_output_port_information(new_node, 1);

        /* add a name for the node, keeping the name of the node same as the output */
        new_node->name = make_full_ref_name(names[input_count - 1], NULL, NULL, NULL, -1);

        /*add this node to blif_netlist as an internal node */
        blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
        blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;

        /*add name information and a net(driver) for the output */

        npin_t* new_pin = allocate_npin();
        new_pin->name = new_node->name;
        new_pin->type = OUTPUT;

        add_output_pin_to_node(new_node, new_pin, 0);

        nnet_t* out_net = (nnet_t*)output_nets_hash->get(names[input_count - 1]);
        if (out_net == nullptr) {
            out_net = allocate_nnet();
            out_net->name = new_node->name;
            output_nets_hash->add(new_node->name, out_net);
        }
        add_driver_pin_to_net(out_net, new_pin);
    }
    /* Free the char** names */
    for (int i = 0; i < input_count; i++)
        vtr::free(names[i]);

    vtr::free(names);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: build_top_input_node)
 * 
 * @brief to build a the top level input
 * 
 * @param name_str representing the name input signal
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::build_top_input_node(const char* name_prefix, const char* name_str) {
    char* temp_string = resolve_signal_name_based_on_blif_type(name_prefix, name_str);

    /* create the .model input port if they do not exist */


    /* create a new top input node and net*/

    nnode_t* new_node = allocate_nnode(my_location);

    new_node->related_ast_node = NULL;
    new_node->type = INPUT_NODE;

    /* add the name of the input variable */
    new_node->name = temp_string;

    /* allocate the pins needed */
    allocate_more_output_pins(new_node, 1);
    add_output_port_information(new_node, 1);

    /* Create the pin connection for the net */
    npin_t* new_pin = allocate_npin();
    new_pin->name = vtr::strdup(temp_string);
    new_pin->type = OUTPUT;

    /* hookup the pin, net, and node */
    add_output_pin_to_node(new_node, new_pin, 0);

    nnet_t* new_net = allocate_nnet();
    new_net->name = vtr::strdup(temp_string);

    add_driver_pin_to_net(new_net, new_pin);

    blif_netlist->top_input_nodes = (nnode_t**)vtr::realloc(blif_netlist->top_input_nodes, sizeof(nnode_t*) * (blif_netlist->num_top_input_nodes + 1));
    blif_netlist->top_input_nodes[blif_netlist->num_top_input_nodes++] = new_node;

    //long sc_spot = sc_add_string(output_nets_sc, temp_string);
    //if (output_nets_sc->data[sc_spot])
    //warning_message(NETLIST,linenum,-1, "Net (%s) with the same name already created\n",temp_string);

    //output_nets_sc->data[sc_spot] = new_net;

    output_nets_hash->add(temp_string, new_net);
}

/**
 * (function: add_top_input_nodes)
 * 
 * @brief to add the top level inputs to the netlist
 *
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::add_top_input_nodes(const char* name_prefix, hard_block_model* model) {
    /**
     * insert a global clock for fall back.
     * in case of undriven internal clocks, they will attach to the global clock
     * this also fix the issue of constant verilog (no input)
     * that cannot simulate due to empty input vector
     */
    if (insert_global_clock) {
        insert_global_clock = false;
        SubcktBLIFReader::build_top_input_node(name_prefix, DEFAULT_CLOCK_NAME);
    }

    char* ptr;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        /* initialize the model struct input names*/
        if (model) {
            model->inputs->names = (char**)vtr::realloc(model->inputs->names, sizeof(char*) * (model->inputs->count + 1));
            model->inputs->names[model->inputs->count++] = vtr::strdup(ptr);
        }

        SubcktBLIFReader::build_top_input_node(name_prefix, ptr);
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: rb_create_top_output_nodes)
 * 
 * @brief to add the top level outputs to the netlist
 * 
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::rb_create_top_output_nodes(const char* name_prefix, hard_block_model* model) {
    char* ptr;
    char buffer[READ_BLIF_BUFFER];

    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        /* initialize the model struct output names*/
         if (model) {
            model->outputs->names = (char**)vtr::realloc(model->outputs->names, sizeof(char*) * (model->outputs->count + 1));
            model->outputs->names[model->outputs->count++] = vtr::strdup(ptr);
         }

        char* temp_string = resolve_signal_name_based_on_blif_type(name_prefix, ptr);

        /*add_a_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);*/

        /* create a new top output node and */
        nnode_t* new_node = allocate_nnode(my_location);
        new_node->related_ast_node = NULL;
        new_node->type = OUTPUT_NODE;

        /* add the name of the output variable */
        new_node->name = temp_string;

        /* allocate the input pin needed */
        allocate_more_input_pins(new_node, 1);
        add_input_port_information(new_node, 1);

        /* Create the pin connection for the net */
        npin_t* new_pin = allocate_npin();
        new_pin->name = temp_string;
        /* hookup the pin, net, and node */
        add_input_pin_to_node(new_node, new_pin, 0);

        /*adding the node to the blif_netlist output nodes
         * add_node_to_netlist() function can also be used */
        blif_netlist->top_output_nodes = (nnode_t**)vtr::realloc(blif_netlist->top_output_nodes, sizeof(nnode_t*) * (blif_netlist->num_top_output_nodes + 1));
        blif_netlist->top_output_nodes[blif_netlist->num_top_output_nodes++] = new_node;
    }
}


/**
 *---------------------------------------------------------------------------------------------
 * (function: verify_hard_block_ports_against_model)
 * 
 * @brief Check for inconsistencies between the hard block model and the ports found
 * in the hard block instance. Returns false if differences are found.
 * 
 * @param ports list of a hard block ports
 * @param model hard block model
 * 
 * @return the hard is verified against model or no.
 *-------------------------------------------------------------------------------------------*/
 bool SubcktBLIFReader::verify_hard_block_ports_against_model(hard_block_ports* ports, hard_block_model* model) {
    hard_block_ports* port_sets[] = {model->input_ports, model->output_ports};
    int i;
    for (i = 0; i < 2; i++) {
        hard_block_ports* p = port_sets[i];
        int j;
        for (j = 0; j < p->count; j++) {
            // Look up each port from the model in "ports"
            char* name = p->names[j];
            int size = p->sizes[j];
            int* idx = (int*)ports->index->get(name);
            // Model port not specified in ports.
            if (!idx) {
                warning_message(PARSE_YOSYS_BLIF, my_location, 
                                "Sub-circuit port %s is not specified in the related model! Will automatically mapped to the corresponding port(%s)", name, name);

                char* token = NULL;
                token = strtok(ports->names[((i == 0) ? 0 : model->input_ports->count) + j], "$");
                int port_index = atoi(token);

                rename_port_name(ports, p->names[j], port_index);
                                
                idx = (int*)ports->index->get(name);
                if (!idx)
                    return false;
            }

            // Make sure they match in size.
            int instance_size = ports->sizes[*idx];
            // Port sizes differ.
            if (size != instance_size) {
                //printf("Port sizes differ. %s\n", name);
                return false;
            }
        }
    }

    hard_block_ports* in = model->input_ports;
    hard_block_ports* out = model->output_ports;
    int j;
    for (j = 0; j < ports->count; j++) {
        // Look up each port from the subckt to make sure it appears in the model.
        char* name = ports->names[j];
        int* in_idx = (int*)in->index->get(name);
        int* out_idx = (int*)out->index->get(name);
        // Port does not appear in the model.
        if (!in_idx && !out_idx) {
            //printf("Port does not appear in the model. %s\n", name);
            return false;
        }
    }

    return true;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: read_hard_block_model)
 * 
 * @brief Scans ahead in the given file to find the
 * model for the hard block by the given name.
 * 
 * @param name_subckt representing the name of the sub-circuit 
 * @param ports list of a hard block ports
 * 
 * @return the file to its original position when finished.
 *-------------------------------------------------------------------------------------------*/
 hard_block_model* SubcktBLIFReader::read_hard_block_model(char* name_prefix, char* name_subckt, hard_block_ports* ports, hard_block_models* models) {
    // Store the current position in the file.
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);
    
    hard_block_model* model;

    while (1) {
        model = NULL;

        // Search the file for .model followed by the subcircuit name.
        char buffer[READ_BLIF_BUFFER];
        while (vtr::fgets(buffer, READ_BLIF_BUFFER, file)) {
            my_location.line += 1;
            char* token = vtr::strtok(buffer, TOKENS, file, buffer);
            // match .model followed by the subcircuit name.
            if (token && !strcmp(token, ".model") && !strcmp(vtr::strtok(NULL, TOKENS, file, buffer), name_prefix)) {
                model = (hard_block_model*)vtr::calloc(1, sizeof(hard_block_model));
                model->name = vtr::strdup(name_subckt);
                model->inputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
                model->inputs->count = 0;
                model->inputs->names = NULL;

                model->outputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
                model->outputs->count = 0;
                model->outputs->names = NULL;

                // Read the inputs and outputs.
                while (vtr::fgets(buffer, READ_BLIF_BUFFER, file)) {
                    char* first_word = vtr::strtok(buffer, TOKENS, file, buffer);
                    if (first_word) {
                        if (!strcmp(first_word, ".inputs")) {
                            SubcktBLIFReader::add_internal_input_nodes(model->name, model); // create the top input nodes
                        } else if (!strcmp(first_word, ".outputs")) {
                            SubcktBLIFReader::rb_create_internal_output_nodes(model->name, model); // create the top output nodes
                        } else if (!strcmp(first_word, ".subckt")) {
                            // need to call this function recursively to create the subcircuit node and assign its in/outputs
                            SubcktBLIFReader::create_hard_block_nodes(model->name, models);
                        } else if (!strcmp(first_word, ".names")) {
                            // to alias the .names with the driver pin
                            SubcktBLIFReader::create_internal_node_and_driver(model->name);
                        } else if (!strcmp(first_word, ".end")) {
                            break;
                        }
                    } 
                }
                break;
            }
        }

        if (!model || feof(file)) {
            if (configuration.in_blif_type != blif_type_e::_ODIN_BLIF)
                model = create_hard_block_model(name_prefix, ports);
            else
                error_message(PARSE_YOSYS_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name_subckt);
        }

        // Sort the names.
        qsort(model->inputs->names, model->inputs->count, sizeof(char*), compare_hard_block_pin_names);
        qsort(model->outputs->names, model->outputs->count, sizeof(char*), compare_hard_block_pin_names);

        // Index the names.
        model->inputs->index = index_names(model->inputs->names, model->inputs->count);
        model->outputs->index = index_names(model->outputs->names, model->outputs->count);

        // Organise the names into ports.
        model->input_ports = get_hard_block_ports(model->inputs->names, model->inputs->count);
        model->output_ports = get_hard_block_ports(model->outputs->names, model->outputs->count);

        // Check that the model we've read matches the ports of the instance we are trying to match.
        if (SubcktBLIFReader::verify_hard_block_ports_against_model(ports, model)) {
            break;
        } else { // If not, free it, and keep looking.
            free_hard_block_model(model);
        }
    }

    // Restore the original position in the file.
    my_location.line = last_line;
    fsetpos(file, &pos);

    return model;
}





/**
 * #############################################################################################################################
 * ##################################################### PRIVATE METHODS #######################################################
 * #############################################################################################################################
*/

/**
 *---------------------------------------------------------------------------------------------
 * (function: build_internal_input_node)
 * 
 * @brief to build a the top level input
 * 
 * @param name_str representing the name input signal
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::build_internal_input_node(const char* name_prefix, const char* name_str) {
    char* temp_string = resolve_signal_name_based_on_blif_type(name_prefix, name_str);

    /* create the .model input port if they do not exist */


    /* create a new top input node and net*/

    nnode_t* new_node = allocate_nnode(my_location);

    new_node->related_ast_node = NULL;
    new_node->type = INPUT_NODE;

    /* add the name of the input variable */
    new_node->name = temp_string;

    /* allocate the pins needed */
    allocate_more_output_pins(new_node, 1);
    add_output_port_information(new_node, 1);

    /* Create the pin connection for the net */
    npin_t* new_pin = allocate_npin();
    new_pin->name = vtr::strdup(temp_string);
    new_pin->type = OUTPUT;

    /* hookup the pin, net, and node */
    add_output_pin_to_node(new_node, new_pin, 0);

    nnet_t* new_net = allocate_nnet();
    new_net->name = vtr::strdup(temp_string);

    add_driver_pin_to_net(new_net, new_pin);

    blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
    blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;

    //long sc_spot = sc_add_string(output_nets_sc, temp_string);
    //if (output_nets_sc->data[sc_spot])
    //warning_message(NETLIST,linenum,-1, "Net (%s) with the same name already created\n",temp_string);

    //output_nets_sc->data[sc_spot] = new_net;

    output_nets_hash->add(temp_string, new_net);
}

/**
 * (function: add_internal_input_nodes)
 * 
 * @brief to add the top level inputs to the netlist
 *
 * @param file pointer to the input blif file
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::add_internal_input_nodes(const char* name_prefix, hard_block_model* model) {
    char* ptr;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        /* initialize the model struct input names*/
        if (model) {
            model->inputs->names = (char**)vtr::realloc(model->inputs->names, sizeof(char*) * (model->inputs->count + 1));
            model->inputs->names[model->inputs->count++] = vtr::strdup(ptr);
        }

        build_internal_input_node(name_prefix, ptr);
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: rb_create_internal_output_nodes)
 * 
 * @brief to add the top level outputs to the netlist
 * 
 * @param file pointer to the input blif file
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::rb_create_internal_output_nodes(const char* name_prefix, hard_block_model* model) {
    char* ptr;
    char buffer[READ_BLIF_BUFFER];

    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        /* initialize the model struct output names*/
         if (model) {
            model->outputs->names = (char**)vtr::realloc(model->outputs->names, sizeof(char*) * (model->outputs->count + 1));
            model->outputs->names[model->outputs->count++] = vtr::strdup(ptr);
         }

        char* temp_string = resolve_signal_name_based_on_blif_type(name_prefix, ptr);

        /*add_a_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);*/

        /* create a new top output node and */
        nnode_t* new_node = allocate_nnode(my_location);
        new_node->related_ast_node = NULL;
        new_node->type = OUTPUT_NODE;

        /* add the name of the output variable */
        new_node->name = temp_string;

        /* allocate the input pin needed */
        allocate_more_input_pins(new_node, 1);
        add_input_port_information(new_node, 1);

        /* Create the pin connection for the net */
        npin_t* new_pin = allocate_npin();
        new_pin->name = temp_string;
        /* hookup the pin, net, and node */
        add_input_pin_to_node(new_node, new_pin, 0);

        /*adding the node to the blif_netlist output nodes
         * add_node_to_netlist() function can also be used */
        blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
        blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: model_parse)
 * 
 * @brief in case of having other tool's blif file like yosys odin needs to read all .model
 * however the odin generated blif file includes only one .model representing the top module
 * 
 * @param buffer a global buffer for tokenizing
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::model_parse(char* buffer) {    
    blif_netlist->identifier = vtr::strdup(vtr::strtok(NULL, TOKENS, file, buffer));
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: resolve_signal_name_based_on_blif_type)
 * 
 * @brief to make the signal names of an input blif 
 * file compatible with the odin's name convention
 * 
 * @param name_str representing the name input signal
 *-------------------------------------------------------------------------------------------*/
 char* SubcktBLIFReader::resolve_signal_name_based_on_blif_type(const char* name_prefix, const char* name_str) {
    char* return_string = NULL;
    if (configuration.in_blif_type == blif_type_e::_SUBCKT_BLIF) {
        char* name = NULL;
        char* index = NULL;
        const char* pos = strchr(name_str, '[');
        if (pos) {
            name = (char*)vtr::malloc((pos - name_str + 1) * sizeof(char));
            memcpy(name, name_str, pos - name_str);
            name[pos - name_str] = '\0';
        }

        const char* pos2 = strchr(name_str, ']');
        if (pos2) {
            index = (char*)vtr::malloc((pos2 - (pos + 1) + 1) * sizeof(char));
            memcpy(index, pos + 1, pos2 - (pos + 1));
            index[pos2 - (pos + 1)] = '\0';
        }

        if (name) {
            int idx = vtr::atoi(index);
            if (name_prefix)
                return_string = make_full_ref_name(name_prefix, NULL, NULL, name, idx);
            else 
                return_string = make_full_ref_name(NULL, NULL, NULL, name, idx);

        } else {
            if (!strcmp(name_str, "$true")) {
                return_string =  make_full_ref_name(VCC_NAME, NULL, NULL, NULL, -1);

            } else if (!strcmp(name_str, "$false")) {
                return_string =  make_full_ref_name(GND_NAME, NULL, NULL, NULL, -1);

            } else if (!strcmp(name_str, "$undef")) {
                return_string =  make_full_ref_name(HBPAD_NAME, NULL, NULL, NULL, -1);

            } else {
                if (name_prefix)
                    return_string = make_full_ref_name(name_prefix, NULL, NULL, name_str, -1);
                else 
                    return_string = make_full_ref_name(name_str, NULL, NULL, NULL, -1);
            }
        }

        if (name)
            vtr::free(name);
        if (index)
            vtr::free(index);
    } else {
        return_string = make_full_ref_name(name_str, NULL, NULL, NULL, -1);
    }

    return return_string;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: rename_port_name)
 * 
 * @brief changing the mapping name of a port to make it compatible 
 * with its model in the following of chaning its name
 * 
 * @param ports list of a hard block ports
 * @param port_index the index of a port that is to be changed
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::rename_port_name(hard_block_ports* ports, char* const new_name, int port_index) {
    
    // keep the data in a container
    void* data = ports->index->get(ports->names[port_index-1]);
    ports->index->remove(ports->names[port_index-1]);
    // free the memory that contains the previous name
    if (ports->names[port_index-1])
        vtr::free(ports->names[port_index-1]);

    // chaning the na,e of the port
    ports->names[port_index-1] = vtr::strdup(new_name);
    // adding the hash key and data to the hashtable
    ports->index->add(ports->names[port_index-1], data);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: create_hard_block)
 * 
 * @brief create a hard block model based on the given hard block port
 * 
 * @param name representing the name of a hard block
 * @param ports list of a hard block ports
 *-------------------------------------------------------------------------------------------*/
 hard_block_model* SubcktBLIFReader::create_hard_block_model(const char* name, hard_block_ports* ports) {
    oassert(ports);

    
    hard_block_model* model = NULL;
    
    switch (configuration.in_blif_type) {
        case (blif_type_e::_SUBCKT_BLIF): {
            if (strcmp(name, "$add") == 0 || strcmp(name, "$sub") == 0 || strcmp(name, "$dff") == 0 || strcmp(name, "$mux") == 0) {
                model = create_multiple_inputs_one_output_port_model(name, ports);

            } else {
                error_message(PARSE_YOSYS_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name);
            }
            break;
        }
        default: {
            error_message(PARSE_YOSYS_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name);
        }
    }

    return model;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: create_multiple_inputs_one_output_port_model)
 * 
  @brief create a model that has multiple input ports and one output port.
 * port sizes will be specified based on the number of pins in the BLIF file
 * 
 * 
 * @param name representing the name of a hard block
 * @param ports list of a hard block ports
 *-------------------------------------------------------------------------------------------*/
 hard_block_model* SubcktBLIFReader::create_multiple_inputs_one_output_port_model(const char* name, hard_block_ports* ports) {
    int i, j;
    char* pin_name;
    hard_block_model* model = NULL;
    
    model = (hard_block_model*)vtr::calloc(1, sizeof(hard_block_model));
    model->name = vtr::strdup(name);

    hard_block_pins* inputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
    hard_block_pins* outputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));

    inputs->count = 0;
    outputs->count = 0;
    for (i = 0; i < ports->count; i++) {
        for (j = 0; j < ports->sizes[i]; j++) {
            pin_name = (char*)vtr::calloc(READ_BLIF_BUFFER, sizeof(char));

            if (ports->sizes[i] == 1)
                sprintf(pin_name, "%s", ports->names[i]);
            else
                sprintf(pin_name, "%s[%d]", ports->names[i], j);

            /* model input ports */
            if (i < ports->count - 1 ) {
                inputs->names = (char**)vtr::realloc(inputs->names, (inputs->count + 1) * sizeof(char*));

                inputs->names[inputs->count] = vtr::strdup(pin_name);
                inputs->count++;

            /* model single output port */
            } else {
                outputs->names = (char**)vtr::realloc(outputs->names, (outputs->count + 1) * sizeof(char*));

                outputs->names[outputs->count] = vtr::strdup(pin_name);
                outputs->count++;
            }

            vtr::free(pin_name);
        }
    }

    model->inputs = inputs;
    model->outputs = outputs;

    return model;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: harmonize_mappings_with_model_pin_names)
 * 
  @brief makes the mappings compatible with the actual model port names
 * 
 * 
 * @param model hard block model
 * @param mappings instance ports names read from the blif file
 *-------------------------------------------------------------------------------------------*/
 void SubcktBLIFReader::harmonize_mappings_with_model_pin_names(hard_block_model* model, char** mappings) {
    int i, j;
    hard_block_pins* pins_set[] = {model->inputs, model->outputs};

    for (i = 0; i < 2; i++) {
        hard_block_pins* pins = pins_set[i];

        for (j = 0; j < pins->count; j++) {
            int mappings_index = ((i == 0) ? 0 : pins_set[0]->count) + j;

            if (strcmp(mappings[mappings_index], pins->names[j])) {
                // found at least one port that does not match with the model, so mappings need to be changed
                vtr::free(mappings[mappings_index]);
                mappings[mappings_index] = vtr::strdup(pins->names[j]);
            }
        }
    }
}
