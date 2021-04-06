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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "odin_globals.h"
#include "odin_util.h"
#include "ast_util.h"
#include "read_yosys_blif.h"
#include "string_cache.h"

#include "netlist_utils.h"
#include "odin_types.h"
#include "netlist_check.h"
#include "node_creation_library.h"
#include "simulate_blif.h"
#include "vtr_util.h"
#include "vtr_memory.h"

namespace yosys {

char* scope_model_name = NULL;

/* DECLARATIONS */
int read_tokens(char* buffer, hard_block_models* models, FILE* file, Hashtable* output_nets_hash);
void create_hard_block_nodes(hard_block_models* models, FILE* file, Hashtable* output_nets_hash);
void add_top_input_nodes(FILE* file, Hashtable* output_nets_hash);
void rb_create_top_output_nodes(FILE* file);
static void build_top_input_node(const char* name_str, Hashtable* output_nets_hash);
static void model_parse(char* buffer, FILE* file);
static char* resolve_signal_name_based_on_blif_type(const char* name_str);
hard_block_model* read_hard_block_model(char* name_subckt, hard_block_ports* ports, FILE* file);
hard_block_model* create_hard_block_model(const char* name, hard_block_ports* ports);

/* DEFINITIONS */
/**
 * @brief Reads a blif file with the given filename and produces
 * a netlist which is referred to by the global variable
 * "blif_netlist".
 * 
 * @return the generated netlist file 
 */
netlist_t* read_blif() {
    insert_global_clock = true;

    my_location.file = 0;
    my_location.line = -1;
    my_location.col = -1;

    blif_netlist = allocate_netlist();
    /*Opening the blif file */
    FILE* file = vtr::fopen(configuration.list_of_file_names[my_location.file].c_str(), "r");
    if (file == NULL) {
        error_message(PARSE_ARGS, my_location, "cannot open file: %s\n", configuration.list_of_file_names[my_location.file].c_str());
    }
    int num_lines = count_blif_lines(file);

    Hashtable* output_nets_hash = new Hashtable();

    printf("Reading top level module\n");
    fflush(stdout);
    /* create the top level module */
    rb_create_top_driver_nets("top", output_nets_hash);

    /* Extracting the netlist by reading the blif file */
    printf("Reading blif netlist...");
    fflush(stdout);

    line_count = 0;
    int position = -1;
    double time = wall_time();
    // A cache of hard block models indexed by name. As each one is read, it's stored here to be used again.
    hard_block_models* models = create_hard_block_models();
    printf("\n");
    char buffer[READ_BLIF_BUFFER];
    while (vtr::fgets(buffer, READ_BLIF_BUFFER, file) && yosys::read_tokens(buffer, models, file, output_nets_hash)) { // Print a progress bar indicating completeness.
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
    delete output_nets_hash;
    fclose(file);
    return blif_netlist;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: read_tokens)
 *
 * @brief Parses the given line from the blif file. 
 * Returns true if there are more lines to read.
 * 
 * @param buffer a global buffer for tokenizing
 * @param models list of hard block models
 * @param file pointer to the input blif file
 * @param output_nets_hash hashmap of output nets
 *-------------------------------------------------------------------------------------------*/
int read_tokens(char* buffer, hard_block_models* models, FILE* file, Hashtable* output_nets_hash) {
    /* Figures out which, if any token is at the start of this line and *
     * takes the appropriate action.                                    */
    char* token = vtr::strtok(buffer, TOKENS, file, buffer);

    if (token) {
        if (skip_reading_bit_map && ((token[0] == '0') || (token[0] == '1') || (token[0] == '-'))) {
            dum_parse(buffer, file);
        } else {
            skip_reading_bit_map = false;
            if (strcmp(token, ".model") == 0) {
                yosys::model_parse(buffer, file); // store the scope model name for in/out name processing
            } else if (strcmp(token, ".inputs") == 0) {
                yosys::add_top_input_nodes(file, output_nets_hash); // create the top input nodes
            } else if (strcmp(token, ".outputs") == 0) {
                yosys::rb_create_top_output_nodes(file); // create the top output nodes
            } else if (strcmp(token, ".names") == 0) {
                create_internal_node_and_driver(file, output_nets_hash);
            } else if (strcmp(token, ".latch") == 0) {
                create_latch_node_and_driver(file, output_nets_hash);
            } else if (strcmp(token, ".subckt") == 0) {
                yosys::create_hard_block_nodes(models, file, output_nets_hash);
            } else if (strcmp(token, ".end") == 0) {
                // Marks the end of the main module of the blif
                // Call function to hook up the nets
                hook_up_nets(output_nets_hash);
                // [TODO] For yosys we need to read other .model as well as the first one!
                //return (configuration.blif_type ! = blif_type_e::_ODIN_BLIF);
                return false;
            }
        }
    }
    return true;
}

/**
 *------------------------------------------------------------------------------------------- 
 * (function:create_hard_block_nodes)
 * 
 * @brief to create the hard block nodes
 * 
 * @param models list of hard block models
 * @param file pointer to the input blif file
 * @param output_nets_hash hashmap of output nets
 *-------------------------------------------------------------------------------------------*/
void create_hard_block_nodes(hard_block_models* models, FILE* file, Hashtable* output_nets_hash) {
    char buffer[READ_BLIF_BUFFER];
    char* subcircuit_name = vtr::strtok(NULL, TOKENS, file, buffer);
    // subcircuit_name = make_full_ref_name(scope_model_name, NULL, subcircuit_name, NULL,-1);

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
        names[i] = resolve_signal_name_based_on_blif_type(vtr::strdup(strtok(NULL, "=")));
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

    // Look up the model in the models cache.
    hard_block_model* model = NULL;
    if ((subcircuit_name != NULL) && (!(model = get_hard_block_model(subcircuit_name, ports, models)))) {
        // If the model isn's present, scan ahead and find it.
        model = yosys::read_hard_block_model(subcircuit_name, ports, file);
        // Add it to the cache.
        add_hard_block_model(model, ports, models);
    }

    nnode_t* new_node = allocate_nnode(my_location);

    // Name the node subcircuit_name~hard_block_number so that the name is unique.
    static long hard_block_number = 0;
    odin_sprintf(buffer, "%s~%ld", subcircuit_name, hard_block_number++);
    new_node->name = make_full_ref_name(buffer, NULL, NULL, NULL, -1);

    // Determine the type of hard block.
    char* subcircuit_name_prefix = vtr::strdup(subcircuit_name);
    subcircuit_name_prefix[5] = '\0';
    if (!strcmp(subcircuit_name, "multiply") || !strcmp(subcircuit_name_prefix, "mult_")
        || !strcmp(subcircuit_name, "$mul") || !strcmp(subcircuit_name_prefix, "$mul")) {
        new_node->type = MULTIPLY;
    } else if (!strcmp(subcircuit_name, "adder") || !strcmp(subcircuit_name_prefix, "adder")
               || !strcmp(subcircuit_name, "$add") || !strcmp(subcircuit_name_prefix, "$add")) {
        new_node->type = ADD;

    } else if (!strcmp(subcircuit_name, "sub") || !strcmp(subcircuit_name_prefix, "sub")
               || !strcmp(subcircuit_name, "$sub") || !strcmp(subcircuit_name_prefix, "$sub")) {
        new_node->type = MINUS;

    } else if (!strcmp(subcircuit_name, "$dff") || !strcmp(subcircuit_name_prefix, "$dff")) {
        new_node->type = FF_NODE;
    } else {
        new_node->type = MEMORY;
    }
    vtr::free(subcircuit_name_prefix);

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
            error_message(PARSE_BLIF, my_location, "Invalid hard block mapping: %s", mapping);

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

        if (!name) error_message(PARSE_BLIF, my_location, "Invalid hard block mapping: %s", model->outputs->names[i]);

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
 *---------------------------------------------------------------------------------------------
 * (function: build_top_input_node)
 * 
 * @brief to build a the top level input
 * 
 * @param name_str representing the name input signal
 * @param output_nets_hash hashmap of output nets
 *-------------------------------------------------------------------------------------------*/
static void build_top_input_node(const char* name_str, Hashtable* output_nets_hash) {
    char* temp_string = resolve_signal_name_based_on_blif_type(name_str);

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
 * @param file pointer to the input blif file
 * @param output_nets_hash hashmap of output nets
 *-------------------------------------------------------------------------------------------*/
void add_top_input_nodes(FILE* file, Hashtable* output_nets_hash) {
    /**
     * insert a global clock for fall back.
     * in case of undriven internal clocks, they will attach to the global clock
     * this also fix the issue of constant verilog (no input)
     * that cannot simulate due to empty input vector
     */
    if (insert_global_clock) {
        insert_global_clock = false;
        yosys::build_top_input_node(DEFAULT_CLOCK_NAME, output_nets_hash);
    }

    char* ptr;
    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        yosys::build_top_input_node(ptr, output_nets_hash);
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: rb_create_top_output_nodes)
 * 
 * @brief to add the top level outputs to the netlist
 * 
 * @param file pointer to the input blif file
 *-------------------------------------------------------------------------------------------*/
void rb_create_top_output_nodes(FILE* file) {
    char* ptr;
    char buffer[READ_BLIF_BUFFER];

    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer))) {
        char* temp_string = resolve_signal_name_based_on_blif_type(ptr);

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
 * (function: model_parse)
 * 
 * @brief in case of having other tool's blif file like yosys odin needs to read all .model
 * however the odin generated blif file includes only one .model representing the top module
 * 
 * @param buffer a global buffer for tokenizing
 * @param file pointer to the input blif file
 *-------------------------------------------------------------------------------------------*/
static void model_parse(char* buffer, FILE* file) {
    if (configuration.blif_type == blif_type_e::_ODIN_BLIF) {
        // Ignore models.
        dum_parse(buffer, file);
    } else if (configuration.blif_type == blif_type_e::_YOSYS_BLIF) {
        scope_model_name = vtr::strdup(vtr::strtok(NULL, TOKENS, file, buffer));
    }
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
static char* resolve_signal_name_based_on_blif_type(const char* name_str) {
    char* return_string = NULL;
    if (configuration.blif_type == blif_type_e::_YOSYS_BLIF) {
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
            return_string = make_full_ref_name(scope_model_name, NULL, NULL, name, idx);
        } else {
            return_string = make_full_ref_name(name_str, NULL, NULL, NULL, -1);
        }
    } else {
        return_string = make_full_ref_name(name_str, NULL, NULL, NULL, -1);
    }

    return return_string;
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
 * @param file pointer to the input blif file
 * 
 * @return the file to its original position when finished.
 *-------------------------------------------------------------------------------------------*/
hard_block_model* read_hard_block_model(char* name_subckt, hard_block_ports* ports, FILE* file) {
    // Store the current position in the file.
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);

    hard_block_model* model;

    while (1) {
        model = NULL;

        // Search the file for .model followed buy the subcircuit name.
        char buffer[READ_BLIF_BUFFER];
        while (vtr::fgets(buffer, READ_BLIF_BUFFER, file)) {
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
                while (vtr::fgets(buffer, READ_BLIF_BUFFER, file)) {
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

        if (!model || feof(file)) {
            if (configuration.blif_type != blif_type_e::_ODIN_BLIF)
                model = create_hard_block_model(name_subckt, ports);
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
 *---------------------------------------------------------------------------------------------
 * (function: create_hard_block)
 * 
 * @brief create a hard block model based on the given hard block port
 * 
 * @param name representing the name of a hard block
 * @param ports list of a hard block ports
 *-------------------------------------------------------------------------------------------*/
hard_block_model* create_hard_block_model(const char* name, hard_block_ports* ports) {
    oassert(ports);

    int i, j;
    hard_block_model* model = NULL;

    switch (configuration.blif_type) {
        case (blif_type_e::_YOSYS_BLIF): {
            if (strcmp(name, "$add") == 0 || strcmp(name, "$sub") == 0) {
                model = (hard_block_model*)vtr::calloc(1, sizeof(hard_block_model));
                model->name = vtr::strdup(name);

                hard_block_pins* inputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));
                hard_block_pins* outputs = (hard_block_pins*)vtr::calloc(1, sizeof(hard_block_pins));

                inputs->count = 0;
                for (i = 0; i < 2; i++) {
                    for (j = 0; j < ports->sizes[i]; j++) {
                        char pin_name[READ_BLIF_BUFFER] = {0};
                        inputs->names = (char**)vtr::realloc(inputs->names, (inputs->count + 1) * sizeof(char*));

                        if (ports->sizes[i] == 1)
                            sprintf(pin_name, "%s", ports->names[i]);
                        else
                            sprintf(pin_name, "%s[%d]", ports->names[i], j);
                        inputs->names[inputs->count] = vtr::strdup(pin_name);
                        inputs->count++;
                    }
                }

                outputs->count = 0;
                for (i = 0; i < ports->sizes[2]; i++) {
                    char pin_name[READ_BLIF_BUFFER] = {0};
                    outputs->names = (char**)vtr::realloc(outputs->names, (outputs->count + 1) * sizeof(char*));

                    if (ports->sizes[2] == 1)
                        sprintf(pin_name, "%s", ports->names[2]);
                    else
                        sprintf(pin_name, "%s[%d]", ports->names[2], i);
                    outputs->names[outputs->count] = vtr::strdup(pin_name);
                    outputs->count++;
                }

                model->inputs = inputs;
                model->outputs = outputs;
            } else if (strcmp(name, "$dff") == 0) {
            } else {
                error_message(PARSE_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name);
            }
            break;
        }
        default: {
            error_message(PARSE_BLIF, my_location, "A subcircuit model for '%s' with matching ports was not found.", name);
            break;
        }
    }

    return model;
}

} // namespace yosys