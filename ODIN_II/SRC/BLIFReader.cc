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

#include "BLIFReader.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "odin_ii.h"
#include "odin_util.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "ast_util.h"
#include "blif_elaborate.h"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"

#include "VerilogReader.hh"
#include "BLIFReader.hh"
#include "OdinBLIFReader.hh"
#include "SubcktBLIFReader.hh"

int line_count;
int num_lines;
bool skip_reading_bit_map;
bool insert_global_clock;

FILE* file;

netlist_t* blif_netlist;
Hashtable* output_nets_hash;

BLIFReader::BLIFReader(): GenericReader() {
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

BLIFReader::~BLIFReader() {
    if (this->odin_blif_reader)
        this->odin_blif_reader->~OdinBLIFReader();

    if (this->subckt_blif_reader)
        this->subckt_blif_reader->~SubcktBLIFReader();

    /**
     *  [TODO]
     *  if (this->eblif_reader)
     *      ~EBLIFReader(); 
    */
}


void* BLIFReader::read() {
    void* netlist = NULL;

    switch(configuration.in_blif_type) {
        case(blif_type_e::_ODIN_BLIF): {
            this->odin_blif_reader = new OdinBLIFReader();
            netlist = odin_blif_reader->read();
            break;
        }
        case(blif_type_e::_SUBCKT_BLIF): {
            this->subckt_blif_reader = new SubcktBLIFReader();
            netlist = subckt_blif_reader->read();
            break;
        }
        /**
         * [TODO]
         * case(blif_type_e::_SUBCKT_BLIF): {
                subckt_blif_reader = new SubcktBLIFReader();
                netlist = subckt_blif_reader->read();
            }
        */
        default: {
            error_message(PARSE_BLIF, unknown_location, "%s", "Unknown input BLIF file format! Should have resolved earlier\n");
            exit(ERROR_PARSE_BLIF);
        }
    }

    printf("Elaborating the netlist created from the input BLIF file to make it compatible with ODIN_II partial mapping\n");
    blif_elaborate_top(verilog_netlist);

    return static_cast<void*>(netlist);
}

/**
 * [NOTE] Functions mentioned below need to be specified based on the input blif type
 * so that we will leave them here and define them in the each BLIF reader class
*/

int BLIFReader::read_tokens(char* /* buffer */, hard_block_models* /* models */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::hook_up_nets() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::hook_up_node(nnode_t* /* node */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::create_hard_block_nodes(hard_block_models* /* models */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::create_internal_node_and_driver() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::build_top_input_node(const char* /* name_prefix */, const char* /* name_str */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::add_top_input_nodes() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIFReader::rb_create_top_output_nodes() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

int BLIFReader::verify_hard_block_ports_against_model(hard_block_ports* /* ports */, hard_block_model* /* model */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

hard_block_model* BLIFReader::read_hard_block_model(char* /* name_subckt */, hard_block_ports* /* ports */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}



/**
 * #############################################################################################################################
 * #################################################### PROTECTED METHODS ######################################################
 * #############################################################################################################################
*/

/*
 * ---------------------------------------------------------------------------------------------
 * function: Creates the drivers for the top module
 * Top module is :
 * Special as all inputs are actually drivers.
 * Also make the 0 and 1 constant nodes at this point.
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::rb_create_top_driver_nets(const char* instance_name_prefix) {
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
 * (function: look_for_clocks)
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::rb_look_for_clocks() {
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
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::dum_parse(char* buffer) {
    /* Continue parsing to the end of this (possibly continued) line. */
    while (vtr::strtok(NULL, TOKENS, file, buffer))
        ;
}


/**
 * ---------------------------------------------------------------------------------------------
 * function:assign_node_type_from_node_name(char *)
 * This function tries to assign the node->type by looking at the name
 * Else return GENERIC
 * function will decide the node->type of the given node
 * ---------------------------------------------------------------------------------------------
 */
operation_list BLIFReader::assign_node_type_from_node_name(char* output_name) {
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
 * function: read_bit_map_find_unknown_gate
 * read the bit map for simulation
 * ---------------------------------------------------------------------------------------------
 */
operation_list BLIFReader::read_bit_map_find_unknown_gate(int input_count, nnode_t* node) {
    operation_list to_return = operation_list_END;

    fpos_t pos;
    int last_line = my_location.line;
    const char* One = "1";
    const char* Zero = "0";
    fgetpos(file, &pos);

    char** bit_map = NULL;
    char* output_bit_map = NULL; // to distinguish whether for the bit_map output is 1 or 0
    int line_count_bitmap = 0;   //stores the number of lines in a particular bit map
    char buffer[READ_BLIF_BUFFER];

    if (!input_count) {
        vtr::fgets(buffer, READ_BLIF_BUFFER, file);
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
            vtr::fgets(buffer, READ_BLIF_BUFFER, file);
            my_location.line += 1;

            if (!(buffer[0] == '0' || buffer[0] == '1' || buffer[0] == '-'))
                break;

            bit_map = (char**)vtr::realloc(bit_map, sizeof(char*) * (line_count_bitmap + 1));
            bit_map[line_count_bitmap++] = vtr::strdup(vtr::strtok(buffer, TOKENS, file, buffer));
            if (output_bit_map != NULL) vtr::free(output_bit_map);
            output_bit_map = vtr::strdup(vtr::strtok(NULL, TOKENS, file, buffer));
        }

        oassert(output_bit_map);

        /*Patern recognition for faster simulation*/
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
 * function:create_latch_node_and_driver
 * to create an ff node and driver from that node
 * format .latch <input> <output> [<type> <control/clock>] <initial val>
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::create_latch_node_and_driver() {
    /* Storing the names of the input and the final output in array names */
    char** names = NULL;       // Store the names of the tokens
    int input_token_count = 0; /*to keep track whether controlling clock is specified or not */
    /*input_token_count=3 it is not and =5 it is */
    char* ptr = NULL;

    char buffer[READ_BLIF_BUFFER];
    while ((ptr = vtr::strtok(NULL, TOKENS, file, buffer)) != NULL) {
        input_token_count += 1;
        names = (char**)vtr::realloc(names, (sizeof(char*)) * (input_token_count));

        names[input_token_count - 1] = vtr::strdup(ptr);
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
    new_node->edge_type = edge_type_blif_enum(names[2], my_location);

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
 * function: search_clock_name
 * to search the clock if the control in the latch
 * is not mentioned
 * ---------------------------------------------------------------------------------------------
 */
char* BLIFReader::search_clock_name() {
    fpos_t pos;
    int last_line = my_location.line;
    fgetpos(file, &pos);
    rewind(file);

    char* to_return = NULL;
    char** input_names = NULL;
    int input_names_count = 0;
    int found = 0;
    while (!found) {
        char buffer[READ_BLIF_BUFFER];
        vtr::fgets(buffer, READ_BLIF_BUFFER, file);
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
 * Gets the text in the given string which occurs
 * before the first instance of "[". The string is
 * presumably of the form "port[pin_number]"
 *
 * The retuned string is strduped and must be freed.
 * The original string is unaffected.
 * ---------------------------------------------------------------------------------------------
 */
char* BLIFReader::get_hard_block_port_name(char* name) {
    name = vtr::strdup(name);
    if (strchr(name, '['))
        return strtok(name, "[");
    else
        return name;
}


/**
 * ---------------------------------------------------------------------------------------------
 * Parses a port name of the form port[pin_number]
 * and returns the pin number as a long. Returns -1
 * if there is no [pin_number] in the name. Throws an
 * error if pin_number is not parsable as a long.
 *
 * The original string is unaffected.
 * ---------------------------------------------------------------------------------------------
 */
long BLIFReader::get_hard_block_pin_number(char* original_name) {
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
 * Callback function for qsort which compares pin names
 * of the form port_name[pin_number] primarily
 * on the port_name, and on the pin_number if the port_names
 * are identical.
 * ---------------------------------------------------------------------------------------------
 */
int BLIFReader::compare_hard_block_pin_names(const void* p1, const void* p2) {
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
 * Organises the given strings representing pin names on a hard block
 * model into ports, and indexes the ports by name. Returns the organised
 * ports as a hard_block_ports struct.
 * ---------------------------------------------------------------------------------------------
 */
hard_block_ports* BLIFReader::get_hard_block_ports(char** pins, int count) {
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
 * Creates a hashtable index for an array of strings of
 * the form names[i]=>i.
 * ---------------------------------------------------------------------------------------------
 */
Hashtable* BLIFReader::index_names(char** names, int count) {
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
 * Create an associative index of names1[i]=>names2[i]
 * ---------------------------------------------------------------------------------------------
 */
Hashtable* BLIFReader::associate_names(char** names1, char** names2, int count) {
    Hashtable* index = new Hashtable();
    for (long i = 0; i < count; i++)
        index->add(names1[i], names2[i]);

    return index;
}


/**
 * ---------------------------------------------------------------------------------------------
 * Looks up a hard block model by name. Returns null if the
 * model is not found.
 * ---------------------------------------------------------------------------------------------
 */
hard_block_model* BLIFReader::get_hard_block_model(char* name, hard_block_ports* ports, hard_block_models* models) {
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
 * Adds the given model to the hard block model cache.
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::add_hard_block_model(hard_block_model* m, hard_block_ports* ports, hard_block_models* models) {
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
 * Generates string which represents the geometry of the given hard block ports.
 * ---------------------------------------------------------------------------------------------
 */
char* BLIFReader::generate_hard_block_ports_signature(hard_block_ports* ports) {
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
 * Creates a new hard block model cache.
 * ---------------------------------------------------------------------------------------------
 */
hard_block_models* BLIFReader::create_hard_block_models() {
    hard_block_models* m = (hard_block_models*)vtr::calloc(1, sizeof(hard_block_models));
    m->models = NULL;
    m->count = 0;
    m->index = new Hashtable();

    return m;
}

/**
 * ---------------------------------------------------------------------------------------------
 * Counts the number of lines in the given blif file
 * before a .end token is hit.
 * ---------------------------------------------------------------------------------------------
 */
int BLIFReader::count_blif_lines() {
    int local_num_lines = 0;
    char buffer[READ_BLIF_BUFFER];
    while (vtr::fgets(buffer, READ_BLIF_BUFFER, file)) {
        if (strstr(buffer, ".end"))
            break;
        local_num_lines++;
    }
    rewind(file);
    return local_num_lines;
}


/**
 * ---------------------------------------------------------------------------------------------
 * Frees the hard block model cache, freeing
 * all encapsulated hard block models.
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::free_hard_block_models(hard_block_models* models) {
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
 * Frees a hard_block_model.
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::free_hard_block_model(hard_block_model* model) {
    free_hard_block_pins(model->inputs);
    free_hard_block_pins(model->outputs);

    free_hard_block_ports(model->input_ports);
    free_hard_block_ports(model->output_ports);

    vtr::free(model->name);
    vtr::free(model);
}

/**
 * ---------------------------------------------------------------------------------------------
 * Frees hard_block_pins
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::free_hard_block_pins(hard_block_pins* p) {
    while (p->count--)
        vtr::free(p->names[p->count]);

    vtr::free(p->names);

    p->index->destroy_free_items();
    delete p->index;
    vtr::free(p);
}

/**
 * ---------------------------------------------------------------------------------------------
 * Frees hard_block_ports
 * ---------------------------------------------------------------------------------------------
 */
void BLIFReader::free_hard_block_ports(hard_block_ports* p) {
    while (p->count--)
        vtr::free(p->names[p->count]);

    vtr::free(p->signature);
    vtr::free(p->names);
    vtr::free(p->sizes);

    p->index->destroy_free_items();
    delete p->index;
    vtr::free(p);
}
