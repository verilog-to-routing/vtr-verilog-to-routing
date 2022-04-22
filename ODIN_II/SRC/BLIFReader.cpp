/**
 *
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
 *
 * @file: includes the definition of BLIF Reader class to read a given
 * BLIF file. The Odin-II BLIF Reader can also read large files since 
 * it read each file with a BLIF_READ_BUFFER chunk or line by line.
 * Odin-II BLIF Reader caches the read models while traversing the
 * BLIF file to avoid searching the BLIF file looking for the model 
 * definition. Moreover, function getbline provides developers with 
 * the ability to read a BLIF file line by line or using a chunk.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "odin_ii.h"
#include "odin_util.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "ast_util.h"
#include "BLIFElaborate.hpp"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"

#include "BLIF.hpp"

int line_count;
int num_lines;
bool skip_reading_bit_map;
bool insert_global_clock;

FILE* file;
netlist_t* blif_netlist;
Hashtable* output_nets_hash;

/**
 * -----------------------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------- Reader -----------------------------------------------------------
 * -----------------------------------------------------------------------------------------------------------------------------
 */

BLIF::Reader::Reader() {
    insert_global_clock = true;

    my_location.file = 0;
    my_location.line = -1;
    my_location.col = -1;

    blif_netlist = allocate_netlist();

    skip_reading_bit_map = false;
    /*Opening the blif file */
    file = vtr::fopen(configuration.list_of_file_names[my_location.file].c_str(), "r");
    if (file == NULL) {
        error_message(PARSE_ARGS, my_location, "cannot open file: %s\n", configuration.list_of_file_names[my_location.file].c_str());
    }
    line_count = 0;
    num_lines = count_blif_lines();

    output_nets_hash = new Hashtable();
}

BLIF::Reader::~Reader() {
    delete output_nets_hash;
}

void* BLIF::Reader::_read() {
    printf("Reading top level module\n");
    fflush(stdout);

    /* Extracting the netlist by reading the blif file */
    printf("Reading blif netlist...");
    fflush(stdout);

    /* find the top module name */
    find_top_module();

    /* create the top level module */
    rb_create_top_driver_nets(blif_netlist->identifier);

    int position = -1;
    double time = wall_time();
    // A cache of hard block models indexed by name. As each one is read, it's stored here to be used again.
    hard_block_models* models = create_hard_block_models();
    printf("\n");
    char* buffer = NULL;
    while (getbline(buffer, READ_BLIF_BUFFER, file) && read_tokens(buffer, models)) { // Print a progress bar indicating completeness.
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

    printf("Elaborating the netlist created from the input BLIF file\n");
    blif_elaborate_top(blif_netlist);

    /* clean up */
    vtr::free(buffer);

    return static_cast<void*>(blif_netlist);
}

/**
 * #############################################################################################################################
 * #################################################### PROTECTED METHODS ######################################################
 * #############################################################################################################################
 */

/**
 * ---------------------------------------------------------------------------------------------
 * (function: read_tokens)
 *
 * @brief Parses the given line from the blif file. 
 * Returns true if there are more lines to read.
 *
 * @param buffer read blif buffer
 * @param models list of all hard block models
 *-------------------------------------------------------------------------------------------*/
int BLIF::Reader::read_tokens(char* buffer, hard_block_models* models) {
    /* Figures out which, if any token is at the start of this line and *
     * takes the appropriate action.                                    */
    char* token = vtr::strtok(buffer, TOKENS, file, buffer);

    if (token) {
        if (skip_reading_bit_map && ((token[0] == '0') || (token[0] == '1') || (token[0] == '-'))) {
            dum_parse(buffer);
        } else {
            skip_reading_bit_map = false;
            if (strcmp(token, ".inputs") == 0) {
                add_top_input_nodes(); // create the top input nodes
            } else if (strcmp(token, ".outputs") == 0) {
                rb_create_top_output_nodes(); // create the top output nodes
            } else if (strcmp(token, ".names") == 0) {
                create_internal_node_and_driver();
            } else if (strcmp(token, ".latch") == 0) {
                create_latch_node_and_driver();
            } else if (strcmp(token, ".subckt") == 0) {
                create_hard_block_nodes(models);
            } else if (strcmp(token, ".end") == 0) {
                // Marks the end of the main module of the blif
                // Call function to hook up the nets
                hook_up_nets();
                return false;
            }
        }
    }
    return true;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: find_top_module)
 * 
 * @brief to find the name of top module
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::find_top_module() {
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);
    rewind(file);

    char* top_module = NULL;

    bool found = false;
    while (!found) {
        char* buffer = NULL;
        getbline(buffer, READ_BLIF_BUFFER, file);
        my_location.line += 1;

        if (feof(file)) {
            /* clean up */
            vtr::free(buffer);
            break;
        }

        char* token = vtr::strtok(buffer, TOKENS, file, buffer);
        if (token && !strcmp(token, ".model")) {
            top_module = vtr::strdup(vtr::strtok(NULL, TOKENS, file, buffer));
            found = true;
        }

        /* clean up */
        vtr::free(buffer);
    }

    if (!found) {
        warning_message(PARSE_BLIF, unknown_location,
                        "%s", "The top module name has not been specifed in the BLIF file, automatically considered as 'top'.\n");

        blif_netlist->identifier = vtr::strdup("top");
    } else {
        blif_netlist->identifier = top_module;
    }

    my_location.line = last_line;
    fsetpos(file, &pos);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: hook_up_nets)
 * 
 * @brief find the output nets and add the corresponding nets
 *-------------------------------------------------------------------------------------------*/
void BLIF::Reader::hook_up_nets() {
    nnode_t** node_sets[] = {blif_netlist->internal_nodes, blif_netlist->ff_nodes, blif_netlist->top_output_nodes};
    int counts[] = {blif_netlist->num_internal_nodes, blif_netlist->num_ff_nodes, blif_netlist->num_top_output_nodes};
    int num_sets = 3;

    /* hook all the input pins in all the internal nodes to the net */
    int i;
    for (i = 0; i < num_sets; i++) {
        int j;
        for (j = 0; j < counts[i]; j++) {
            nnode_t* node = node_sets[i][j];
            hook_up_node(node);
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
void BLIF::Reader::hook_up_node(nnode_t* node) {
    int j;
    for (j = 0; j < node->num_input_pins; j++) {
        npin_t* input_pin = node->input_pins[j];

        nnet_t* output_net = (nnet_t*)output_nets_hash->get(input_pin->name);

        if (!output_net)
            error_message(PARSE_BLIF, my_location, "Error: Could not hook up the pin %s: not available.", input_pin->name);

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
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::create_hard_block_nodes(hard_block_models* models) {
    char buffer[READ_BLIF_BUFFER];
    char* subcircuit_name = vtr::strtok(NULL, TOKENS, file, buffer);

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
        names[i] = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, strtok(NULL, "="));
    }

    // Associate mappings with their connections.
    Hashtable* mapping_index = associate_names(mappings, names, count);

    // Sort the mappings.
    qsort(mappings, count, sizeof(char*), compare_hard_block_pin_names);

    for (i = 0; i < count; i++)
        vtr::free(names_parameters[i]);

    vtr::free(names_parameters);

    // Index the mappings in a hard_block_ports struct.
    hard_block_ports* ports = get_hard_block_ports(mappings, count);

    for (i = 0; i < count; i++) {
        vtr::free(mappings[i]);
        mappings[i] = NULL;
    }

    vtr::free(mappings);
    mappings = NULL;

    t_model* hb_model = NULL;
    nnode_t* new_node = allocate_nnode(my_location);

    // Name the node subcircuit_name~hard_block_number so that the name is unique.
    static long hard_block_number = 0;
    odin_sprintf(buffer, "%s~%ld", subcircuit_name, hard_block_number++);
    new_node->name = make_full_ref_name(buffer, NULL, NULL, NULL, -1);

    // init the edge sensitivity of hard block
    if (configuration.coarsen)
        hard_block_sensitivities(subcircuit_name, new_node);

    {
        // getting the subckt prefix name
        char* subcircuit_stripped_name = get_stripped_name(subcircuit_name);
        /* check for coarse-grain configuration */
        if (configuration.coarsen) {
            if (yosys_subckt_strmap.find(subcircuit_name) != yosys_subckt_strmap.end())
                new_node->type = yosys_subckt_strmap[subcircuit_name];

            if (new_node->type == NO_OP && yosys_subckt_strmap.find(subcircuit_stripped_name) != yosys_subckt_strmap.end())
                new_node->type = yosys_subckt_strmap[subcircuit_stripped_name];

            if (new_node->type == NO_OP) {
                char new_name[READ_BLIF_BUFFER];
                vtr::free(new_node->name);
                /* in case of weird names, need to add memories manually */
                int sc_spot = -1;
                char* yosys_subckt_str = NULL;
                if ((yosys_subckt_str = retrieve_node_type_from_subckt_name(subcircuit_stripped_name)) != NULL) {
                    /* specify node type */
                    new_node->type = yosys_subckt_strmap[yosys_subckt_str];
                    /* specify node name */
                    odin_sprintf(new_name, "\\%s~%ld", yosys_subckt_str, hard_block_number - 1);
                } else if ((sc_spot = sc_lookup_string(hard_block_names, subcircuit_stripped_name)) != -1) {
                    /* specify node type */
                    new_node->type = HARD_IP;
                    /* specify node name */
                    odin_sprintf(new_name, "\\%s~%ld", subcircuit_stripped_name, hard_block_number - 1);
                    /* Detect used hard block for the blif generation */
                    hb_model = find_hard_block(subcircuit_stripped_name);
                    if (hb_model) {
                        hb_model->used = 1;
                    }
                } else {
                    error_message(PARSE_BLIF, unknown_location,
                                  "Unsupported subcircuit type (%s) in BLIF file.\n", subcircuit_name);
                }
                new_node->name = make_full_ref_name(new_name, NULL, NULL, NULL, -1);

                // CLEAN UP
                vtr::free(yosys_subckt_str);
            }

            if (new_node->type == BRAM) {
                new_node->type = (new_node->attributes->RD_PORTS
                                  && new_node->attributes->WR_PORTS)
                                     ? BRAM
                                     : (new_node->attributes->RD_PORTS
                                        && !new_node->attributes->WR_PORTS)
                                           ? ROM
                                           : operation_list_END;
            }
        } else {
            new_node->type = odin_subckt_strmap[subcircuit_name];

            /* check for subcircuit prefix prefix */
            if (subcircuit_stripped_name && new_node->type == NO_OP)
                new_node->type = odin_subckt_strmap[subcircuit_stripped_name];

            if (new_node->type == NO_OP)
                new_node->type = MEMORY;
        }
        // CLEAN UP
        vtr::free(subcircuit_stripped_name);
    }

    // Look up the model in the models cache.
    hard_block_model* model = NULL;
    if ((subcircuit_name != NULL) && (!(model = get_hard_block_model(subcircuit_name, ports, models)))) {
        // If the model isn's present, scan ahead and find it.
        model = read_hard_block_model(subcircuit_name, new_node->type, ports);
        // Add it to the cache.
        add_hard_block_model(model, ports, models);
    }

    if (!model)
        error_message(PARSE_BLIF, unknown_location,
                      "Failed to retrieve subcircuit model (%s)\n", subcircuit_name);

    /* Add input and output ports to the new node. */
    else {
        hard_block_ports* p = NULL;
        p = model->input_ports;
        for (i = 0; i < p->count; i++)
            add_input_port_information(new_node, p->sizes[i]);

        p = model->output_ports;
        for (i = 0; i < p->count; i++)
            add_output_port_information(new_node, p->sizes[i]);

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
                error_message(PARSE_BLIF, my_location, "Invalid hard block mapping: %s", mapping);

            npin_t* new_pin = allocate_npin();
            new_pin->name = vtr::strdup(name);
            new_pin->type = INPUT;
            new_pin->mapping = (new_node->type == BRAM
                                || new_node->type == ROM
                                || new_node->type == SPRAM
                                || new_node->type == DPRAM
                                || new_node->type == MEMORY
                                || hb_model != NULL)
                                   ? get_hard_block_port_name(mapping)
                                   : NULL;

            add_input_pin_to_node(new_node, new_pin, i);
        }

        // Add output pins, nets, and index each net.
        for (i = 0; i < model->outputs->count; i++) {
            char* mapping = model->outputs->names[i];
            char* name = (char*)mapping_index->get(mapping);

            if (!name) error_message(PARSE_BLIF, my_location, "Invalid hard block mapping: %s", model->outputs->names[i]);

            npin_t* new_pin = allocate_npin();
            new_pin->name = NULL; //vtr::strdup(name);
            new_pin->type = OUTPUT;
            new_pin->mapping = (new_node->type == BRAM
                                || new_node->type == ROM
                                || new_node->type == SPRAM
                                || new_node->type == DPRAM
                                || new_node->type == MEMORY
                                || hb_model != NULL)
                                   ? get_hard_block_port_name(mapping)
                                   : NULL;

            add_output_pin_to_node(new_node, new_pin, i);

            nnet_t* new_net = allocate_nnet();
            new_net->name = vtr::strdup(name);

            add_driver_pin_to_net(new_net, new_pin);

            // Index the net by name.
            output_nets_hash->add(name, new_net);
        }

        // Create a fake ast node.
        if (!configuration.coarsen || new_node->type == HARD_IP) {
            new_node->related_ast_node = create_node_w_type(HARD_BLOCK, my_location);
            new_node->related_ast_node->children = (ast_node_t**)vtr::calloc(1, sizeof(ast_node_t*));
            new_node->related_ast_node->identifier_node = create_tree_node_id(vtr::strdup(subcircuit_name), my_location);
        }

        /*add this node to blif_netlist as an internal node */
        blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
        blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;

        free_hard_block_ports(ports);
        mapping_index->destroy_free_items();
        delete mapping_index;
        vtr::free(names);
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function:create_internal_node_and_driver)
 * 
 * @brief to create an internal node and driver from that node
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::create_internal_node_and_driver() {
    /* Storing the names of the input and the final output in array names */
    char* ptr = NULL;
    char** names = NULL; // stores the names of the input and the output, last name stored would be of the output
    int input_count = 0;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        names = (char**)vtr::realloc(names, sizeof(char*) * (input_count + 1));
        names[input_count++] = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, ptr);
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
            new_node->type = (operation_list)read_bit_map_find_unknown_gate(input_count - 1, new_node, names);
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
            } else if (new_node->type == SMUX_2) {
                add_input_port_information(new_node, 1);
                add_input_port_information(new_node, input_count - 2);
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
        new_pin->name = vtr::strdup(new_node->name);
        new_pin->type = OUTPUT;

        add_output_pin_to_node(new_node, new_pin, 0);

        nnet_t* out_net = (nnet_t*)output_nets_hash->get(names[input_count - 1]);
        if (out_net == nullptr) {
            out_net = allocate_nnet();
            out_net->name = vtr::strdup(new_node->name);
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
 * @param name_str representing the input signal name
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::build_top_input_node(const char* name_str) {
    char* temp_string = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, name_str);

    /* create a new top input node and net*/

    nnode_t* new_node = allocate_nnode(my_location);

    new_node->related_ast_node = NULL;
    new_node->type = INPUT_NODE;

    /* add the name of the input variable */
    new_node->name = vtr::strdup(temp_string);

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
    vtr::free(temp_string);
}

/**
 * (function: add_top_input_nodes)
 * 
 * @brief to add the top level inputs to the netlist
 *
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::add_top_input_nodes() {
    /**
     * insert a global clock for fall back.
     * in case of undriven internal clocks, they will attach to the global clock
     * this also fix the issue of constant verilog (no input)
     * that cannot simulate due to empty input vector
     */
    if (insert_global_clock && !configuration.coarsen) {
        insert_global_clock = false;
        build_top_input_node(DEFAULT_CLOCK_NAME);
    }

    char* ptr;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        build_top_input_node(ptr);
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: rb_create_top_output_nodes)
 * 
 * @brief to add the top level outputs to the netlist
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::rb_create_top_output_nodes() {
    char* ptr;
    char buffer[READ_BLIF_BUFFER];

    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        char* temp_string = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, ptr);

        /*add_a_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);*/

        /* create a new top output node and */
        nnode_t* new_node = allocate_nnode(my_location);
        new_node->related_ast_node = NULL;
        new_node->type = OUTPUT_NODE;

        /* add the name of the output variable */
        new_node->name = vtr::strdup(temp_string);

        /* allocate the input pin needed */
        allocate_more_input_pins(new_node, 1);
        add_input_port_information(new_node, 1);

        /* Create the pin connection for the net */
        npin_t* new_pin = allocate_npin();
        new_pin->name = vtr::strdup(temp_string);
        /* hookup the pin, net, and node */
        add_input_pin_to_node(new_node, new_pin, 0);

        /*adding the node to the blif_netlist output nodes
         * add_node_to_netlist() function can also be used */
        blif_netlist->top_output_nodes = (nnode_t**)vtr::realloc(blif_netlist->top_output_nodes, sizeof(nnode_t*) * (blif_netlist->num_top_output_nodes + 1));
        blif_netlist->top_output_nodes[blif_netlist->num_top_output_nodes++] = new_node;
        vtr::free(temp_string);
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
 * -------------------------------------------------------------------------------------------
 */
bool BLIF::Reader::verify_hard_block_ports_against_model(hard_block_ports* ports, hard_block_model* model) {
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
                //printf("Model port not specified in ports. %s\n", name);
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
 * -------------------------------------------------------------------------------------------
 */
hard_block_model* BLIF::Reader::read_hard_block_model(char* name_subckt, operation_list type, hard_block_ports* ports) {
    // Store the current position in the file.
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);

    hard_block_model* model;

    while (1) {
        model = NULL;

        // Search the file for .model followed buy the subcircuit name.
        char* buffer = NULL;
        while (getbline(buffer, READ_BLIF_BUFFER, file)) {
            my_location.line += 1;
            char* token = vtr::strtok(buffer, TOKENS, file, buffer);
            // match .model followed by the subcircuit name.
            if (token && !strcmp(token, ".model") && !strcmp(vtr::strtok(NULL, TOKENS, file, buffer), name_subckt)) {
                model = (hard_block_model*)vtr::calloc(1, sizeof(hard_block_model));
                model->name = vtr::strdup(name_subckt);
                model->inputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
                model->inputs->count = 0;
                model->inputs->names = NULL;

                model->outputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
                model->outputs->count = 0;
                model->outputs->names = NULL;

                // Read the inputs and outputs.
                while (getbline(buffer, READ_BLIF_BUFFER, file)) {
                    char* first_word = vtr::strtok(buffer, TOKENS, file, buffer);
                    if (first_word) {
                        if (!strcmp(first_word, ".inputs")) {
                            char* name;
                            while ((name = vtr::strtok(NULL, TOKENS, file, buffer))) {
                                model->inputs->names = (char**)vtr::realloc(model->inputs->names, sizeof(char*) * (model->inputs->count + 1));
                                model->inputs->names[model->inputs->count++] = vtr::strdup(name);
                            }
                        } else if (!strcmp(first_word, ".outputs")) {
                            char* name;
                            while ((name = vtr::strtok(NULL, TOKENS, file, buffer))) {
                                model->outputs->names = (char**)vtr::realloc(model->outputs->names, sizeof(char*) * (model->outputs->count + 1));
                                model->outputs->names[model->outputs->count++] = vtr::strdup(name);
                            }
                        } else if (!strcmp(first_word, ".end")) {
                            break;
                        }
                    }
                }
                break;
            }
        }

        /* clean up */
        vtr::free(buffer);

        if (!model || feof(file)) {
            if (configuration.coarsen)
                model = create_hard_block_model(name_subckt, type, ports);
            else
                error_message(PARSE_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name_subckt);
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
        if (verify_hard_block_ports_against_model(ports, model)) {
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
 * ---------------------------------------------------------------------------------------------
 * (function: rb_create_top_driver_nets)
 *
 * @brief Creates the drivers for the top module
 * Special as all inputs are actually drivers.
 * Also make the 0 and 1 constant nodes at this point.
 *
 * @param instance_name_prefix the prefix of the instance name
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::rb_create_top_driver_nets(const char* instance_name_prefix) {
    npin_t* new_pin;
    /* create the constant nets */

    /* ZERO net */
    /* description given for the zero net is same for other two */
    blif_netlist->zero_net = allocate_nnet();                  // allocate memory to net pointer
    blif_netlist->gnd_node = allocate_nnode(unknown_location); // allocate memory to node pointer
    blif_netlist->gnd_node->type = GND_NODE;                   // mark the type
    allocate_more_output_pins(blif_netlist->gnd_node, 1);      // alloacate 1 output pin pointer to this node
    add_output_port_information(blif_netlist->gnd_node, 1);    // add port info. this port has 1 pin ,till now number of port for this is one
    new_pin = allocate_npin();
    add_output_pin_to_node(blif_netlist->gnd_node, new_pin, 0); // add this pin to output pin pointer array of this node
    add_driver_pin_to_net(blif_netlist->zero_net, new_pin);     // add this pin to net as driver pin

    /*ONE net*/
    blif_netlist->one_net = allocate_nnet();
    blif_netlist->vcc_node = allocate_nnode(unknown_location);
    blif_netlist->vcc_node->type = VCC_NODE;
    allocate_more_output_pins(blif_netlist->vcc_node, 1);
    add_output_port_information(blif_netlist->vcc_node, 1);
    new_pin = allocate_npin();
    add_output_pin_to_node(blif_netlist->vcc_node, new_pin, 0);
    add_driver_pin_to_net(blif_netlist->one_net, new_pin);

    /* Pad net */
    blif_netlist->pad_net = allocate_nnet();
    blif_netlist->pad_node = allocate_nnode(unknown_location);
    blif_netlist->pad_node->type = PAD_NODE;
    allocate_more_output_pins(blif_netlist->pad_node, 1);
    add_output_port_information(blif_netlist->pad_node, 1);
    new_pin = allocate_npin();
    add_output_pin_to_node(blif_netlist->pad_node, new_pin, 0);
    add_driver_pin_to_net(blif_netlist->pad_net, new_pin);

    /* CREATE the driver for the ZERO */
    blif_netlist->zero_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, zero_string, -1);
    output_nets_hash->add(GND_NAME, blif_netlist->zero_net);

    /* CREATE the driver for the ONE and store twice */
    blif_netlist->one_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, one_string, -1);
    output_nets_hash->add(VCC_NAME, blif_netlist->one_net);

    /* CREATE the driver for the PAD */
    blif_netlist->pad_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, pad_string, -1);
    output_nets_hash->add(HBPAD_NAME, blif_netlist->pad_net);

    blif_netlist->vcc_node->name = vtr::strdup(VCC_NAME);
    blif_netlist->gnd_node->name = vtr::strdup(GND_NAME);
    blif_netlist->pad_node->name = vtr::strdup(HBPAD_NAME);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: rb_look_for_clocks)
 *
 * @brief looking for clock nodes in the blif file by 
 * going through tthe clock driver of ff nodes
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::rb_look_for_clocks() {
    int i;
    for (i = 0; i < blif_netlist->num_ff_nodes; i++) {
        oassert(blif_netlist->ff_nodes[i]->input_pins[1]->net->num_driver_pins == 1);
        if (blif_netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node->type != CLOCK_NODE) {
            blif_netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node->type = CLOCK_NODE;
        }
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: dum_parse)
 *
 * @brief continue reading file to the end
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::dum_parse(char* buffer) {
    /* Continue parsing to the end of this (possibly continued) line. */
    while (vtr::strtok(NULL, TOKENS, file, buffer))
        ;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function:assign_node_type_from_node_name)
 *
 * @brief This function tries to assign the node->type by looking at the name
 * Else return GENERIC. function will decide the node->type of the given node
 *
 * @param output_name the node name
 * ---------------------------------------------------------------------------------------------
 */
operation_list BLIF::Reader::assign_node_type_from_node_name(char* output_name) {
    //variable to extract the type
    operation_list result = GENERIC;

    int start, end;
    int length_string = strlen(output_name);
    for (start = length_string - 1; (start >= 0) && (output_name[start] != '^'); start--)
        ;
    for (end = length_string - 1; (end >= 0) && (output_name[end] != '~'); end--)
        ;

    if ((start < end) && (end > 0)) {
        // Stores the extracted string
        char* extracted_string = (char*)vtr::calloc(end - start + 2, sizeof(char));
        int i, j;
        for (i = start + 1, j = 0; i < end; i++, j++) {
            extracted_string[j] = output_name[i];
        }

        extracted_string[j] = '\0';
        for (i = 0; i < operation_list_END; i++) {
            if (!strcmp(extracted_string, operation_list_STR[i][ODIN_LONG_STRING])
                || !strcmp(extracted_string, operation_list_STR[i][ODIN_SHORT_STRING])) {
                result = static_cast<operation_list>(i);
                break;
            }
        }

        vtr::free(extracted_string);
    }
    return result;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: read_bit_map_find_unknown_gate)
 * 
 * @brief read the bit map for simulation
 *
 * @param input_count number of inputs
 * @param node pointer to the netlist node
 * @param names list of node inputs
 * ---------------------------------------------------------------------------------------------
 */
operation_list BLIF::Reader::read_bit_map_find_unknown_gate(int input_count, nnode_t* node, char** names) {
    operation_list to_return = operation_list_END;

    fpos_t pos;
    int last_line = my_location.line;
    const char* One = "1";
    const char* Zero = "0";
    fgetpos(file, &pos);

    char** bit_map = NULL;
    char* output_bit_map = NULL; // to distinguish whether for the bit_map output is 1 or 0
    int line_count_bitmap = 0;   //stores the number of lines in a particular bit map
    char* buffer = NULL;

    if (!input_count) {
        getbline(buffer, READ_BLIF_BUFFER, file);
        my_location.line += 1;

        char* ptr = vtr::strtok(buffer, "\t\n", file, buffer);
        if (!ptr) {
            to_return = GND_NODE;
        } else if (!strcmp(ptr, " 1")) {
            to_return = VCC_NODE;
        } else if (!strcmp(ptr, " 0")) {
            to_return = GND_NODE;
        } else {
            to_return = VCC_NODE;
        }
    } else {
        while (1) {
            getbline(buffer, READ_BLIF_BUFFER, file);
            my_location.line += 1;

            if (!(buffer[0] == '0' || buffer[0] == '1' || buffer[0] == '-'))
                break;

            bit_map = (char**)vtr::realloc(bit_map, sizeof(char*) * (line_count_bitmap + 1));
            bit_map[line_count_bitmap++] = vtr::strdup(vtr::strtok(buffer, TOKENS, file, buffer));
            if (output_bit_map != NULL) vtr::free(output_bit_map);
            output_bit_map = vtr::strdup(vtr::strtok(NULL, TOKENS, file, buffer));
        }

        oassert(output_bit_map);

        /*Pattern recognition for faster simulation*/
        if (!strcmp(output_bit_map, One)) {
            //On-gate recognition
            //TODO move off-logic parts to appropriate code block

            vtr::free(output_bit_map);
            output_bit_map = vtr::strdup(One);
            node->generic_output = 1;

            /* Single line bit map : */
            if (line_count_bitmap == 1) {
                // GT
                if (!strcmp(bit_map[0], "100")) {
                    to_return = GT;
                }

                // LT
                else if (!strcmp(bit_map[0], "010")) {
                    to_return = LT;
                }

                /* LOGICAL_AND and LOGICAL_NAND for ABC*/
                else {
                    int i;
                    for (i = 0; i < input_count && bit_map[0][i] == '1'; i++)
                        ;

                    if (i == input_count) {
                        if (!strcmp(output_bit_map, "1")) {
                            if (input_count == 1)
                                to_return = BUF_NODE;
                            else
                                to_return = LOGICAL_AND;
                        } else if (!strcmp(output_bit_map, "0")) {
                            to_return = LOGICAL_NAND;
                        }
                    }

                    /* BITWISE_NOT */
                    if (!strcmp(bit_map[0], "0") && to_return == operation_list_END) {
                        to_return = BITWISE_NOT;
                    }
                    /* LOGICAL_NOR and LOGICAL_OR for ABC */
                    for (i = 0; i < input_count && bit_map[0][i] == '0'; i++)
                        ;

                    if (i == input_count && to_return == operation_list_END) {
                        if (!strcmp(output_bit_map, "1")) {
                            to_return = LOGICAL_NOR;
                        } else if (!strcmp(output_bit_map, "0")) {
                            to_return = LOGICAL_OR;
                        }
                    }
                }
            }
            /* Assumption that bit map is in order when read from blif */
            else if (line_count_bitmap == 2) {
                /* LOGICAL_XOR */
                if ((strcmp(bit_map[0], "01") == 0) && (strcmp(bit_map[1], "10") == 0)) {
                    to_return = LOGICAL_XOR;
                }
                /* LOGICAL_XNOR */
                else if ((strcmp(bit_map[0], "00") == 0) && (strcmp(bit_map[1], "11") == 0)) {
                    to_return = LOGICAL_XNOR;
                }
                /* SMUX_2 */
                else if (((strcmp(bit_map[0], "01-") == 0) && (strcmp(bit_map[1], "1-1") == 0)) || ((strcmp(bit_map[0], "1-0") == 0) && (strcmp(bit_map[1], "-11") == 0))) {
                    to_return = SMUX_2;

                    /** 
                     * if in some BLIF files the mux selector is considered as 
                     * the last input instead of the first, here we handle it 
                     * to move the selector to the first port
                     */
                    if (strcmp(bit_map[0], "1-0") == 0) {
                        char* selector_name = names[2];
                        names[2] = names[1];
                        names[1] = names[0];
                        names[0] = selector_name;
                    }
                }
            } else if (line_count_bitmap == 4) {
                /* ADDER_FUNC */
                if (
                    (!strcmp(bit_map[0], "001"))
                    && (!strcmp(bit_map[1], "010"))
                    && (!strcmp(bit_map[2], "100"))
                    && (!strcmp(bit_map[3], "111"))) {
                    to_return = ADDER_FUNC;
                }
                /* CARRY_FUNC */
                else if (
                    (!strcmp(bit_map[0], "011"))
                    && (!strcmp(bit_map[1], "101"))
                    && (!strcmp(bit_map[2], "110"))
                    && (!strcmp(bit_map[3], "111"))) {
                    to_return = CARRY_FUNC;
                }
                /* LOGICAL_XOR */
                else if (
                    (!strcmp(bit_map[0], "001"))
                    && (!strcmp(bit_map[1], "010"))
                    && (!strcmp(bit_map[2], "100"))
                    && (!strcmp(bit_map[3], "111"))) {
                    to_return = LOGICAL_XOR;
                }
                /* LOGICAL_XNOR */
                else if (
                    (!strcmp(bit_map[0], "000"))
                    && (!strcmp(bit_map[1], "011"))
                    && (!strcmp(bit_map[2], "101"))
                    && (!strcmp(bit_map[3], "110"))) {
                    to_return = LOGICAL_XNOR;
                }
            }

            if (line_count_bitmap == input_count && to_return == operation_list_END) {
                /* LOGICAL_OR */
                int i;
                for (i = 0; i < line_count_bitmap; i++) {
                    if (bit_map[i][i] == '1') {
                        int j;
                        for (j = 1; j < input_count; j++) {
                            if (bit_map[i][(i + j) % input_count] != '-') {
                                break;
                            }
                        }

                        if (j != input_count) {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                if (i == line_count_bitmap) {
                    to_return = LOGICAL_OR;
                } else {
                    /* LOGICAL_NAND */
                    for (i = 0; i < line_count_bitmap; i++) {
                        if (bit_map[i][i] == '0') {
                            int j;
                            for (j = 1; j < input_count; j++) {
                                if (bit_map[i][(i + j) % input_count] != '-') {
                                    break;
                                }
                            }

                            if (j != input_count) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    if (i == line_count_bitmap) {
                        to_return = LOGICAL_NAND;
                    }
                }
            }

            /* MUX_2 */
            if (line_count_bitmap * 2 == input_count && to_return == operation_list_END) {
                int i;
                for (i = 0; i < line_count_bitmap; i++) {
                    if ((bit_map[i][i] == '1') && (bit_map[i][i + line_count_bitmap] == '1')) {
                        int j;
                        for (j = 1; j < line_count_bitmap; j++) {
                            if (
                                (bit_map[i][(i + j) % line_count_bitmap] != '-')
                                || (bit_map[i][((i + j) % line_count_bitmap) + line_count_bitmap] != '-')) {
                                break;
                            }
                        }

                        if (j != input_count) {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                if (i == line_count_bitmap) {
                    to_return = MUX_2;
                }
            }
        } else {
            //Off-gate recognition
            //TODO

            vtr::free(output_bit_map);
            output_bit_map = vtr::strdup(Zero);
            node->generic_output = 0;
        }

        /* assigning the bit_map to the node if it is GENERIC */
        if (to_return == operation_list_END) {
            node->bit_map = bit_map;
            node->bit_map_line_count = line_count_bitmap;
            to_return = GENERIC;
        }
    }
    /* clean up */
    vtr::free(buffer);

    if (output_bit_map) {
        vtr::free(output_bit_map);
    }
    if (bit_map) {
        for (int i = 0; i < line_count_bitmap; i++) {
            vtr::free(bit_map[i]);
        }
        vtr::free(bit_map);
    }

    my_location.line = last_line;
    fsetpos(file, &pos);

    return to_return;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: create_latch_node_and_driver)
 * 
 * @brief to create an ff node and driver from that node format
 * .latch <input> <output> [<type> <control/clock>] <initial val>
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::create_latch_node_and_driver() {
    /* Storing the names of the input and the final output in array names */
    char** names = NULL;       // Store the names of the tokens
    int input_token_count = 0; /*to keep track whether controlling clock is specified or not */
    /*input_token_count=3 it is not and =5 it is */
    char* ptr = NULL;

    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer)) != NULL) {
        input_token_count += 1;
        names = (char**)vtr::realloc(names, (sizeof(char*)) * (input_token_count));

        names[input_token_count - 1] = resolve_signal_name_based_on_blif_type(blif_netlist->identifier, ptr);
    }

    /* assigning the new_node */
    if (input_token_count != 5) {
        /* supported added for the ABC .latch output without control */
        if (input_token_count == 3) {
            input_token_count = 5;
            names = (char**)vtr::realloc(names, sizeof(char*) * input_token_count);

            names[3] = search_clock_name();
            names[4] = names[2];
            names[2] = vtr::strdup("re");
        } else {
            std::string line = "";
            for (int i = 0; i < input_token_count; i++) {
                line += names[i];
                line += " ";
            }

            error_message(PARSE_BLIF, my_location, "This .latch Format not supported: <%s> \n\t required format :.latch <input> <output> [<type> <control/clock>] <initial val>",
                          line.c_str());
        }
    }

    nnode_t* new_node = allocate_nnode(my_location);
    new_node->related_ast_node = NULL;
    new_node->type = FF_NODE;
    new_node->attributes->clk_edge_type = edge_type_blif_enum(names[2], my_location);

    new_node->initial_value = (init_value_e)atoi(names[4]);

    /* allocate the output pin (there is always one output pin) */
    allocate_more_output_pins(new_node, 1);
    add_output_port_information(new_node, 1);

    /* allocate the input pin */
    allocate_more_input_pins(new_node, 2); /* input[1] is clock */

    /* add the port information */
    int i;
    for (i = 0; i < 2; i++) {
        add_input_port_information(new_node, 1);
    }

    /* add names and type information to the created input pins */
    npin_t* new_pin = allocate_npin();
    new_pin->name = vtr::strdup(names[0]);
    new_pin->type = INPUT;
    add_input_pin_to_node(new_node, new_pin, 0);

    new_pin = allocate_npin();
    new_pin->name = vtr::strdup(names[3]);
    new_pin->type = INPUT;
    add_input_pin_to_node(new_node, new_pin, 1);

    /* add a name for the node, keeping the name of the node same as the output */
    new_node->name = make_full_ref_name(names[1], NULL, NULL, NULL, -1);

    /*add this node to blif_netlist as an ff (flip-flop) node */
    blif_netlist->ff_nodes = (nnode_t**)vtr::realloc(blif_netlist->ff_nodes, sizeof(nnode_t*) * (blif_netlist->num_ff_nodes + 1));
    blif_netlist->ff_nodes[blif_netlist->num_ff_nodes++] = new_node;

    /*add name information and a net(driver) for the output */
    nnet_t* new_net = allocate_nnet();
    new_net->name = new_node->name;

    new_pin = allocate_npin();
    new_pin->name = new_node->name;
    new_pin->type = OUTPUT;
    add_output_pin_to_node(new_node, new_pin, 0);
    add_driver_pin_to_net(new_net, new_pin);

    output_nets_hash->add(new_node->name, new_net);

    /* Free the char** names */
    for (i = 0; i < input_token_count; i++)
        vtr::free(names[i]);

    vtr::free(names);
    vtr::free(ptr);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: search_clock_name)
 * 
 * @brief to search the clock if the control in the latch
 * is not mentioned
 * ---------------------------------------------------------------------------------------------
 */
char* BLIF::Reader::search_clock_name() {
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);
    rewind(file);

    char* to_return = NULL;
    char** input_names = NULL;
    int input_names_count = 0;
    int found = 0;
    while (!found) {
        char* buffer = NULL;
        getbline(buffer, READ_BLIF_BUFFER, file);
        my_location.line += 1;

        // not sure if this is needed
        if (feof(file))
            break;

        char* ptr = NULL;
        if ((ptr = vtr::strtok(buffer, TOKENS, file, buffer))) {
            if (!strcmp(ptr, ".end"))
                break;

            if (!strcmp(ptr, ".inputs")) {
                /* store the inputs in array of string */
                while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
                    input_names = (char**)vtr::realloc(input_names, sizeof(char*) * (input_names_count + 1));
                    input_names[input_names_count++] = vtr::strdup(ptr);
                }
            } else if (!strcmp(ptr, ".names") || !strcmp(ptr, ".latch")) {
                while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
                    int i;
                    for (i = 0; i < input_names_count; i++) {
                        if (!strcmp(ptr, input_names[i])) {
                            vtr::free(input_names[i]);
                            input_names[i] = input_names[--input_names_count];
                        }
                    }
                }
            } else if (input_names_count == 1) {
                found = 1;
            }
        }

        /* clean up */
        vtr::free(buffer);
    }
    my_location.line = last_line;
    fsetpos(file, &pos);

    if (found) {
        to_return = input_names[0];
    } else {
        to_return = vtr::strdup(DEFAULT_CLOCK_NAME);
        for (int i = 0; i < input_names_count; i++) {
            if (input_names[i]) {
                vtr::free(input_names[i]);
            }
        }
    }

    vtr::free(input_names);

    return to_return;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (funtion: get_hard_block_port_name)
 *
 * @brief Gets the text in the given string which occurs
 * before the first instance of "[". The string is
 * presumably of the form "port[pin_number]"
 *
 * The retuned string is strduped and must be freed.
 * The original string is unaffected.
 *
 * @param name the cstring of the port name
 * ---------------------------------------------------------------------------------------------
 */
char* BLIF::Reader::get_hard_block_port_name(char* name) {
    name = vtr::strdup(name);
    if (strchr(name, '['))
        return strtok(name, "[");
    else
        return name;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: get_hard_block_pin_number)
 * 
 * @brief Parses a port name of the form port[pin_number]
 * and returns the pin number as a long. Returns -1
 * if there is no [pin_number] in the name. Throws an
 * error if pin_number is not parsable as a long.
 * The original string is unaffected.
 *
 * @param original_name
 * ---------------------------------------------------------------------------------------------
 */
long BLIF::Reader::get_hard_block_pin_number(char* original_name) {
    if (!strchr(original_name, '['))
        return -1;

    char* name = vtr::strdup(original_name);
    strtok(name, "[");
    char* endptr;
    char* pin_number_string = strtok(NULL, "]");
    long pin_number = strtol(pin_number_string, &endptr, 10);

    if (pin_number_string == endptr)
        error_message(PARSE_BLIF, my_location, "The given port name \"%s\" does not contain a valid pin number.", original_name);

    vtr::free(name);

    return pin_number;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: compare_hard_block_pin_names)
 *
 * @brief Callback function for qsort which compares pin names
 * of the form port_name[pin_number] primarily
 * on the port_name, and on the pin_number if the port_names
 * are identical.
 *
 * @param p1 pin name 1
 * @param p2 pin name 2
 * ---------------------------------------------------------------------------------------------
 */
int BLIF::Reader::compare_hard_block_pin_names(const void* p1, const void* p2) {
    char* name1 = *(char* const*)p1;
    char* name2 = *(char* const*)p2;

    char* port_name1 = get_hard_block_port_name(name1);
    char* port_name2 = get_hard_block_port_name(name2);
    int portname_difference = strcmp(port_name1, port_name2);
    vtr::free(port_name1);
    vtr::free(port_name2);

    // If the portnames are the same, compare the pin numbers.
    if (!portname_difference) {
        int n1 = get_hard_block_pin_number(name1);
        int n2 = get_hard_block_pin_number(name2);
        return n1 - n2;
    } else {
        return portname_difference;
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: get_hard_block_ports)
 * 
 * @brief Organises the given strings representing pin names on a hard block
 * model into ports, and indexes the ports by name. 
 *
 * @param pins list of hard block pins
 * @param count number of hard block pins
 *
 * @return the organised ports as a hard_block_ports struct.
 * ---------------------------------------------------------------------------------------------
 */
hard_block_ports* BLIF::Reader::get_hard_block_ports(char** pins, int count) {
    // Count the input port sizes.
    hard_block_ports* ports = (hard_block_ports*)vtr::calloc(1, sizeof(hard_block_ports));
    ports->count = 0;
    ports->sizes = NULL;
    ports->names = NULL;
    char* prev_portname = NULL;
    int i;
    for (i = 0; i < count; i++) {
        char* portname = get_hard_block_port_name(pins[i]);
        // Compare the part of the name before the "["
        if (!i || strcmp(prev_portname, portname)) {
            ports->sizes = (int*)vtr::realloc(ports->sizes, sizeof(int) * (ports->count + 1));
            ports->names = (char**)vtr::realloc(ports->names, sizeof(char*) * (ports->count + 1));

            ports->sizes[ports->count] = 0;
            ports->names[ports->count] = vtr::strdup(portname);
            ports->count++;
        }

        if (prev_portname != NULL)
            vtr::free(prev_portname);

        prev_portname = portname;
        ports->sizes[ports->count - 1]++;
    }

    if (prev_portname != NULL)
        vtr::free(prev_portname);

    ports->signature = generate_hard_block_ports_signature(ports);
    ports->index = index_names(ports->names, ports->count);

    return ports;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: index_names)
 *
 * @brief Creates a hashtable index for an array of strings of
 * the form names[i]=>i.
 *
 * @param names the list of names
 * @param count the number of names
 * ---------------------------------------------------------------------------------------------
 */
Hashtable* BLIF::Reader::index_names(char** names, int count) {
    Hashtable* index = new Hashtable();
    for (long i = 0; i < count; i++) {
        int* offset = (int*)vtr::calloc(1, sizeof(int));
        *offset = i;
        index->add(names[i], offset);
    }
    return index;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: associate_names)
 *
 * @brief Create an associative index of names1[i]=>names2[i]
 * NOTE: the size of both lists should be the same
 *
 * @param names1 the first list of names
 * @param names2 the second list of names
 * @param count the number of names
 * ---------------------------------------------------------------------------------------------
 */
Hashtable* BLIF::Reader::associate_names(char** names1, char** names2, int count) {
    Hashtable* index = new Hashtable();
    for (long i = 0; i < count; i++)
        index->add(names1[i], names2[i]);

    return index;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: get_hard_block_model)
 *
 * @brief Looks up a hard block model by name. Returns null if the
 * model is not found.
 *
 * @param name hard block model name
 * @param ports list of the hard block's ports
 * @param models list of all hard block models
 * ---------------------------------------------------------------------------------------------
 */
hard_block_model* BLIF::Reader::get_hard_block_model(char* name, hard_block_ports* ports, hard_block_models* models) {
    hard_block_model* to_return = NULL;
    char needle[READ_BLIF_BUFFER] = {0};

    if (name && ports && ports->signature)
        sprintf(needle, "%s%s", name, ports->signature);
    else if (name)
        sprintf(needle, "%s", name);
    else if (ports && ports->signature)
        sprintf(needle, "%s", ports->signature);

    if (strlen(needle) > 0)
        to_return = (hard_block_model*)models->index->get(needle);

    return to_return;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: add_hard_block_model)
 *
 * @brief Adds the given model to the hard block model cache.
 *
 * @param m the hard block model
 * @param ports list of the hard block's ports
 * @param models list of all hard block models
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::add_hard_block_model(hard_block_model* m, hard_block_ports* ports, hard_block_models* models) {
    if (models && m) {
        char needle[READ_BLIF_BUFFER] = {0};

        if (m->name && ports && ports->signature)
            sprintf(needle, "%s%s", m->name, ports->signature);
        else if (m->name)
            sprintf(needle, "%s", m->name);
        else if (ports && ports->signature)
            sprintf(needle, "%s", ports->signature);

        if (strlen(needle) > 0) {
            models->count += 1;

            models->models = (hard_block_model**)vtr::realloc(models->models, models->count * sizeof(hard_block_model*));
            models->models[models->count - 1] = m;
            models->index->add(needle, m);
        }
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: generate_hard_block_ports_signature)
 *
 * @brief Generates string which represents the geometry of the given hard block ports.
 *
 * @param ports list of the hard block's ports
 * ---------------------------------------------------------------------------------------------
 */
char* BLIF::Reader::generate_hard_block_ports_signature(hard_block_ports* ports) {
    char buffer[READ_BLIF_BUFFER];
    buffer[0] = '\0';

    strcat(buffer, "_");

    int j;
    for (j = 0; j < ports->count; j++) {
        char buffer1[READ_BLIF_BUFFER];
        odin_sprintf(buffer1, "%s_%d_", ports->names[j], ports->sizes[j]);
        strcat(buffer, buffer1);
    }
    return vtr::strdup(buffer);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: create_hard_block_models)
 * 
 * @brief Creates a new hard block model cache.
 * ---------------------------------------------------------------------------------------------
 */
hard_block_models* BLIF::Reader::create_hard_block_models() {
    hard_block_models* m = (hard_block_models*)vtr::calloc(1, sizeof(hard_block_models));
    m->models = NULL;
    m->count = 0;
    m->index = new Hashtable();

    return m;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: count_blif_lines)
 * 
 * @brief Counts the number of lines in the given blif file
 * before a .end token is hit.
 * ---------------------------------------------------------------------------------------------
 */
int BLIF::Reader::count_blif_lines() {
    int local_num_lines = 0;
    char* buffer = NULL;
    while (getbline(buffer, READ_BLIF_BUFFER, file)) {
        if (strstr(buffer, ".end"))
            break;
        local_num_lines++;
    }

    rewind(file);

    /* clean up */
    vtr::free(buffer);

    return local_num_lines;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: free_hard_block_models)
 *
 * @brief Frees the hard block model cache, freeing
 * all encapsulated hard block models.
 *
 * @param models list of all hard block models
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::free_hard_block_models(hard_block_models* models) {
    //does not delete the items in the hash
    delete models->index;
    int i;
    for (i = 0; i < models->count; i++)
        free_hard_block_model(models->models[i]);

    vtr::free(models->models);
    vtr::free(models);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: free_hard_block_model)
 *
 * @brief Frees a hard_block_model.
 * 
 * @param models list of all hard block models
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::free_hard_block_model(hard_block_model* model) {
    free_hard_block_pins(model->inputs);
    free_hard_block_pins(model->outputs);

    free_hard_block_ports(model->input_ports);
    free_hard_block_ports(model->output_ports);

    vtr::free(model->name);
    vtr::free(model);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: free_hard_block_pins)
 *
 * @brief Frees hard_block_pins
 *
 * @param p list of hard block pins
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::free_hard_block_pins(hard_block_pins* p) {
    while (p->count--)
        vtr::free(p->names[p->count]);

    vtr::free(p->names);

    p->index->destroy_free_items();
    delete p->index;
    vtr::free(p);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (free_hard_block_ports)
 *
 * @brief Frees hard_block_ports
 *
 * @param p list of hard block ports
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::free_hard_block_ports(hard_block_ports* p) {
    while (p->count--)
        vtr::free(p->names[p->count]);

    vtr::free(p->signature);
    vtr::free(p->names);
    vtr::free(p->sizes);

    p->index->destroy_free_items();
    delete p->index;
    vtr::free(p);
}

/**
 * #############################################################################################################################
 * ##################################################### PRIVATE METHODS #######################################################
 * #############################################################################################################################
 */

/**
 *---------------------------------------------------------------------------------------------
 * (function: resolve_signal_name_based_on_blif_type)
 * 
 * @brief to make the signal names of an input blif 
 * file compatible with the odin's name convention
 * 
 * @param name_str representing the name input signal
 * -------------------------------------------------------------------------------------------
 */
char* BLIF::Reader::resolve_signal_name_based_on_blif_type(const char* name_prefix, const char* name_str) {
    char* return_string = NULL;

    char* name = NULL;
    char* index = NULL;
    char* first_part = NULL;
    char* second_part = NULL;
    const char* pos = strchr(name_str, '[');
    while (pos) {
        const char* pre_pos = pos;
        pos = strchr(pos + 1, '[');
        if (!pos) {
            pos = pre_pos;

            const char* pos_colon = strchr(pos + 1, ':');
            if (pos_colon) {
                pos = NULL;
            }
            break;
        }
    }

    if (pos) {
        name = (char*)vtr::malloc((pos - name_str + 1) * sizeof(char));
        memcpy(name, name_str, pos - name_str);
        name[pos - name_str] = '\0';

        const char* pos2 = strchr(pos, ']');
        if (pos2) {
            index = (char*)vtr::malloc((pos2 - (pos + 1) + 1) * sizeof(char));
            memcpy(index, pos + 1, pos2 - (pos + 1));
            index[pos2 - (pos + 1)] = '\0';

            const char* end = strchr(pos, '\0');
            if (end) {
                second_part = (char*)vtr::malloc((end - (pos2 + 1) + 1) * sizeof(char));
                memcpy(second_part, pos2 + 1, end - (pos2 + 1));
                second_part[end - (pos2 + 1)] = '\0';
            } else {
                error_message(PARSE_BLIF, my_location, "Invalid pin name (%s)\n", name_str);
            }
        }
    }

    if (name && configuration.coarsen) {
        oassert(index && "Invalid signal indexing!\n");
        int idx = vtr::atoi(index);
        if (name_prefix)
            first_part = make_full_ref_name(name_prefix, NULL, NULL, name, idx);
        else
            first_part = make_full_ref_name(NULL, NULL, NULL, name, idx);

        return_string = vtr::strdup((std::string(first_part) + std::string(second_part)).c_str());
    } else {
        if (!strcmp(name_str, "$true")) {
            return_string = make_full_ref_name(VCC_NAME, NULL, NULL, NULL, -1);

        } else if (!strcmp(name_str, "$false")) {
            return_string = make_full_ref_name(GND_NAME, NULL, NULL, NULL, -1);

        } else if (!strcmp(name_str, "$undef")) {
            return_string = make_full_ref_name(HBPAD_NAME, NULL, NULL, NULL, -1);

        } else {
            if (name_prefix && configuration.coarsen && strcmp(name_str, DEFAULT_CLOCK_NAME))
                return_string = make_full_ref_name(name_prefix, NULL, NULL, name_str, -1);
            else
                return_string = make_full_ref_name(name_str, NULL, NULL, NULL, -1);
        }
    }

    if (name)
        vtr::free(name);
    if (index)
        vtr::free(index);
    if (first_part)
        vtr::free(first_part);
    if (second_part)
        vtr::free(second_part);

    return return_string;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: create_hard_block)
 * 
 * @brief create a hard block model based on the given hard block port
 * 
 * @param name representing the name of a hard block
 * @param type node type
 * @param ports list of a hard block ports
 * -------------------------------------------------------------------------------------------
 */
hard_block_model* BLIF::Reader::create_hard_block_model(const char* name, operation_list type, hard_block_ports* ports) {
    oassert(ports);

    hard_block_model* model = NULL;

    switch (type) {
        case (LT):             //fallthrough
        case (GT):             //fallthrough
        case (LTE):            //fallthrough
        case (GTE):            //fallthrough
        case (SL):             //fallthrough
        case (SR):             //fallthrough
        case (ASL):            //fallthrough
        case (ASR):            //fallthrough
        case (ADD):            //fallthrough
        case (PMUX):           //fallthrough
        case (MINUS):          //fallthrough
        case (MULTIPLY):       //fallthrough
        case (POWER):          //fallthrough
        case (MODULO):         //fallthrough
        case (DIVIDE):         //fallthrough
        case (DLATCH):         //fallthrough
        case (ADLATCH):        //fallthrough
        case (DFFE):           //fallthrough
        case (FF_NODE):        //fallthrough
        case (BITWISE_OR):     //fallthrough
        case (BITWISE_NOT):    //fallthrough
        case (BITWISE_AND):    //fallthrough
        case (LOGICAL_OR):     //fallthrough
        case (LOGICAL_XOR):    //fallthrough
        case (LOGICAL_AND):    //fallthrough
        case (LOGICAL_NOT):    //fallthrough
        case (LOGICAL_XNOR):   //fallthrough
        case (LOGICAL_EQUAL):  //fallthrough
        case (NOT_EQUAL):      //fallthrough
        case (CASE_EQUAL):     //fallthrough
        case (CASE_NOT_EQUAL): //fallthrough
        case (MULTIPORT_nBIT_SMUX): {
            // create a model with single output port, being read as the port [n-1] among [0...n-1]
            model = create_model(name, ports, ports->count - 1, ports->count);
            break;
        }
        case (SETCLR): //fallthrough
        case (SPRAM):  //fallthrough
        case (ROM):    //fallthrough
        case (SDFF):   //fallthrough
        case (SDFFE):  //fallthrough
        case (SDFFCE): //fallthrough
        case (DFFSR):  //fallthrough
        case (DFFSRE): {
            // create a model with single output port, being read as the port [n-2] among [0...n-1]
            model = create_model(name, ports, ports->count - 2, ports->count - 1);
            break;
        }
        case (DPRAM): {
            // create a model with two output ports,being read as the port [n-4, n-3] among [0...n-1]
            model = create_model(name, ports, ports->count - 4, ports->count - 2);
            break;
        }
        case (YMEM): //fallthorugh
        case (BRAM): {
            // create a model with single output port, being read as the second port among [0...n-1]
            model = create_model(name, ports, 2, 3);
            break;
        }
        default: {
            error_message(PARSE_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name);
        }
    }

    return model;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: create_model)
 * 
 * @brief create a model that has multiple input ports and one output port.
 * port sizes will be specified based on the number of pins in the BLIF file
 * 
 * @param name representing the name of a hard block
 * @param ports list of a hard block ports
 * @param output_idx_START showing the beginning idx of output ports
 * @param output_idx_END showing the end idx of output ports (outputs exluding the END idx)
 * -------------------------------------------------------------------------------------------
 */
hard_block_model* BLIF::Reader::create_model(const char* name, hard_block_ports* ports, int output_idx_START, int output_idx_END) {
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
            if (i < output_idx_START || i >= output_idx_END) {
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
 * (function: hard_block_sensitivities)
 * 
 * @brief specify sensitivities related to a hard block, they
 * are specified under .param tab in yosys generated blif files
 * 
 * @param subckt_name hard block name 
 * @param new_node pointer to the netlist node
 * -------------------------------------------------------------------------------------------
 */
void BLIF::Reader::hard_block_sensitivities(const char* subckt_name, nnode_t* new_node) {
    // Store the current position in the file.
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);

    char* ptr;
    char* buffer = NULL;
    attr_t* attributes = new_node->attributes;
    operation_list op = (yosys_subckt_strmap.find(subckt_name) != yosys_subckt_strmap.end())
                            ? yosys_subckt_strmap[subckt_name]
                            : NO_OP;

    if (need_params(op)) {
        while (getbline(buffer, READ_BLIF_BUFFER, file)) {
            my_location.line += 1;
            ptr = vtr::strtok(buffer, TOKENS, file, buffer);

            if (!strcmp(ptr, ".param")) {
                ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                if (!strcmp(ptr, "SRST_VALUE")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->sreset_value = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "ARST_VALUE")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->areset_value = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "OFFSET")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->offset = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "SIZE")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->size = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "WIDTH")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->DBITS = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "RD_PORTS")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->RD_PORTS = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "WR_PORTS")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->WR_PORTS = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "ABITS")) {
                    ptr = vtr::strtok(NULL, TOKENS, file, buffer);
                    attributes->ABITS = std::bitset<sizeof(long) * 8>(ptr).to_ulong();

                } else if (!strcmp(ptr, "MEMID")) {
                    std::string memory_id(vtr::strtok(NULL, TOKENS, file, buffer));
                    unsigned first_back_slash = memory_id.find_last_of(YOSYS_ID_FIRST_DELIMITER);
                    unsigned last_double_quote = memory_id.find_last_not_of(YOSYS_ID_LAST_DELIMITER);
                    attributes->memory_id = vtr::strdup(
                        memory_id.substr(
                                     first_back_slash + 1, last_double_quote - first_back_slash)
                            .c_str());
                } else {
                    int sensitivity = atoi(vtr::strtok(NULL, TOKENS, file, buffer));
                    if (!strcmp(ptr, "A_SIGNED")) {
                        if (sensitivity == 1)
                            attributes->port_a_signed = SIGNED;
                        else if (sensitivity == 0)
                            attributes->port_a_signed = UNSIGNED;

                    } else if (!strcmp(ptr, "B_SIGNED")) {
                        if (sensitivity == 1)
                            attributes->port_b_signed = SIGNED;
                        else if (sensitivity == 0)
                            attributes->port_b_signed = UNSIGNED;

                    } else if (!strcmp(ptr, "CLK_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->clk_edge_type = RISING_EDGE_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->clk_edge_type = FALLING_EDGE_SENSITIVITY;

                    } else if (!strcmp(ptr, "CLR_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->clr_polarity = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->clr_polarity = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "SET_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->set_polarity = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->set_polarity = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "EN_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->enable_polarity = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->enable_polarity = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "ARST_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->areset_polarity = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->areset_polarity = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "SRST_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->sreset_polarity = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->sreset_polarity = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "RD_CLK_ENABLE")) {
                        if (sensitivity == 1)
                            attributes->RD_CLK_ENABLE = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->RD_CLK_ENABLE = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "WR_CLK_ENABLE")) {
                        if (sensitivity == 1)
                            attributes->WR_CLK_ENABLE = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->WR_CLK_ENABLE = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "RD_CLK_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->RD_CLK_POLARITY = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->RD_CLK_POLARITY = ACTIVE_LOW_SENSITIVITY;

                    } else if (!strcmp(ptr, "WR_CLK_POLARITY")) {
                        if (sensitivity == 1)
                            attributes->WR_CLK_POLARITY = ACTIVE_HIGH_SENSITIVITY;
                        else if (sensitivity == 0)
                            attributes->WR_CLK_POLARITY = ACTIVE_LOW_SENSITIVITY;
                    }
                }
            } else if (!strcmp(ptr, ".end") || !strcmp(ptr, ".subckt")) {
                break;
            }
        }
    }

    /* clean up */
    vtr::free(buffer);

    // Restore the original position in the file.
    my_location.line = last_line;
    fsetpos(file, &pos);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: need_params)
 * 
 * @brief specify whether a type needs clock sensitivity or not
 * 
 * @param type node type
 * -------------------------------------------------------------------------------------------
 */
bool BLIF::Reader::need_params(operation_list type) {
    bool return_value = false;
    switch (type) {
        case (SL):      //fallthrough
        case (SR):      //fallthrough
        case (ASL):     //fallthrough
        case (ASR):     //fallthrough
        case (DLATCH):  //fallthrough
        case (ADLATCH): //fallthrough
        case (SETCLR):  //fallthrough
        case (SDFF):    //fallthrough
        case (DFFE):    //fallthrough
        case (SDFFE):   //fallthrough
        case (SDFFCE):  //fallthrough
        case (DFFSR):   //fallthrough
        case (DFFSRE):  //fallthrough
        case (SPRAM):   //fallthrough
        case (DPRAM):   //fallthrough
        case (YMEM):    //fallthrough
        case (BRAM):    //fallthrough
        case (ROM):     //fallthrough
        case (FF_NODE): {
            return_value = true;
            break;
        }
        default:
            break;
    }

    return return_value;
}
