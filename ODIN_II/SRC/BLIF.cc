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
#include <stdarg.h>

#include "odin_ii.h"
#include "odin_util.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "ast_util.h"
#include "blif_elaborate.hh"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"

#include "VerilogReader.hh"
#include "BLIF.hh"
#include "OdinBLIFReader.hh"
#include "SubcktBLIFReader.hh"


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


void* BLIF::Reader::__read() {
    void* netlist = NULL;

    switch(configuration.in_blif_type) {
        case(blif_type_e::_ODIN_BLIF): {
            this->odin_blif_reader = new OdinBLIFReader();
            netlist = odin_blif_reader->__read();
            break;
        }
        case(blif_type_e::_SUBCKT_BLIF): {
            this->subckt_blif_reader = new SubcktBLIFReader();
            netlist = subckt_blif_reader->__read();
            break;
        }
        /**
         * [TODO]
         * case(blif_type_e::_OTHER_BLIF): {
                other_blif_reader = new OtherBLIFReader();
                netlist = other_blif_reader->read();
            }
        */
        default: {
            error_message(PARSE_BLIF, unknown_location, "%s", "Unknown input BLIF file format! Should have resolved earlier\n");
            exit(ERROR_PARSE_BLIF);
        }
    }


    printf("Elaborating the netlist created from the input BLIF file to make it compatible with ODIN_II partial mapping\n");
    verilog_netlist = static_cast<netlist_t*>(netlist);
    blif_elaborate_top(verilog_netlist);

    return static_cast<void*>(netlist);
}

/**
 * [NOTE] Functions mentioned below need to be specified based on the input blif type
 * so that we will leave them here and define them in the each BLIF reader class
*/

int BLIF::Reader::read_tokens(char* /* buffer */, hard_block_models* /* models */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::hook_up_nets() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::hook_up_node(nnode_t* /* node */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::create_hard_block_nodes(hard_block_models* /* models */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::create_internal_node_and_driver() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::build_top_input_node(const char* /* name_prefix */, const char* /* name_str */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::add_top_input_nodes() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

void BLIF::Reader::rb_create_top_output_nodes() {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

int BLIF::Reader::verify_hard_block_ports_against_model(hard_block_ports* /* ports */, hard_block_model* /* model */) {
    error_message(PARSE_BLIF, unknown_location, 
                 "Function \"%s\" is called for reading the general input BLIF file without definition provided!\n", __PRETTY_FUNCTION__); 
    exit(ERROR_PARSE_BLIF);
}

hard_block_model* BLIF::Reader::read_hard_block_model(char* /* name_subckt */, hard_block_ports* /* ports */) {
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
 * (function: look_for_clocks)
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
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Reader::dum_parse(char* buffer) {
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
 * function: read_bit_map_find_unknown_gate
 * read the bit map for simulation
 * ---------------------------------------------------------------------------------------------
 */
operation_list BLIF::Reader::read_bit_map_find_unknown_gate(int input_count, nnode_t* node) {
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
char* BLIF::Reader::get_hard_block_port_name(char* name) {
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
 * Callback function for qsort which compares pin names
 * of the form port_name[pin_number] primarily
 * on the port_name, and on the pin_number if the port_names
 * are identical.
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
 * Organises the given strings representing pin names on a hard block
 * model into ports, and indexes the ports by name. Returns the organised
 * ports as a hard_block_ports struct.
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
 * Creates a hashtable index for an array of strings of
 * the form names[i]=>i.
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
 * Create an associative index of names1[i]=>names2[i]
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
 * Looks up a hard block model by name. Returns null if the
 * model is not found.
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
 * Adds the given model to the hard block model cache.
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
 * Generates string which represents the geometry of the given hard block ports.
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
 * Creates a new hard block model cache.
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
 * Counts the number of lines in the given blif file
 * before a .end token is hit.
 * ---------------------------------------------------------------------------------------------
 */
int BLIF::Reader::count_blif_lines() {
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
 * Frees a hard_block_model.
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
 * Frees hard_block_pins
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
 * Frees hard_block_ports
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
 * -----------------------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------- Writer -----------------------------------------------------------
 * -----------------------------------------------------------------------------------------------------------------------------
*/

BLIF::Writer::Writer(): GenericWriter() {
    this->haveOutputLatchBlackbox = false;
}

BLIF::Writer::~Writer() = default;

void BLIF::Writer::__write(const netlist_t* netlist) {

    this->output_file = create_blif(global_args.output_file.value().c_str());

    output_blif(this->output_file, netlist);
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: warn_undriven)
 * 	to check is a net undriven
 *---------------------------------------------------------------------------------------------
 */
bool BLIF::Writer::warn_undriven(nnode_t* node, nnet_t* net) {
    if (!net->num_driver_pins) {
        // Add a warning for an undriven net.
        warning_message(NETLIST, node->loc,
                        "Net %s driving node %s is itself undriven.",
                        net->name, node->name);

        return true;
    }

    return false;
}

// [TODO] Uncomment this for In Outs
/* static void BLIF::Writer::merge_with_inputs(nnode_t* node, long pin_idx) {
   oassert(pin_idx < node->num_input_pins);
   nnet_t* net = node->input_pins[pin_idx]->net;
   warn_undriven(node, net);
   // Merge node with all inputs with fanout of 1
   if (net->num_fanout_pins <= 1) {
       for (int i = 0; i < net->num_driver_pins; i++) {
           npin_t* driver = net->driver_pins[i];
           if (driver->name != NULL && ((driver->node->type == MULTIPLY) || (driver->node->type == HARD_IP) || (driver->node->type == MEMORY) || (driver->node->type == ADD) || (driver->node->type == MINUS))) {
               vtr::free(driver->name);
               driver->name = vtr::strdup(node->name);
           } else {
               vtr::free(driver->node->name);
               driver->node->name = vtr::strdup(node->name);
           }
       }
   }
} */
/**
 * ---------------------------------------------------------------------------------------------
 * (function: print_net_driver)
 * 	The function that prints the driver of a given net
 *---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::print_net_driver(FILE* out, nnode_t* node, nnet_t* net, long driver_idx) {
    oassert(driver_idx < net->num_driver_pins);
    npin_t* driver = net->driver_pins[driver_idx];
    if (!driver->node) {
        // Add a warning for an undriven net.
        warning_message(NETLIST, node->loc,
                        "Net %s driving node %s is itself undriven.",
                        net->name, node->name);

        fprintf(out, " %s", "unconn");
    } else if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED
               && driver->node->related_ast_node != NULL) {
        fprintf(out, " %s^^%i-%i",
                driver->node->name,
                driver->node->related_ast_node->far_tag,
                driver->node->related_ast_node->high_number);
    } else {
        if (driver->name != NULL && ((driver->node->type == MULTIPLY) || (driver->node->type == HARD_IP) || (driver->node->type == MEMORY) || (driver->node->type == ADD) || (driver->node->type == MINUS))) {
            fprintf(out, " %s", driver->name);
        } else {
            fprintf(out, " %s", driver->node->name);
        }
    }
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: print_input_single_driver)
 * 	The function that prints the single input pin driver 
 *---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::print_input_single_driver(FILE* out, nnode_t* node, long pin_idx) {
    oassert(pin_idx < node->num_input_pins);
    nnet_t* net = node->input_pins[pin_idx]->net;
    if (warn_undriven(node, net)) {
        fprintf(out, " unconn");
    } else {
        oassert(net->num_driver_pins == 1);
        print_net_driver(out, node, net, 0);
    }
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: print_output_pin)
 * 	The function that prints the output pins of a given node
 *---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::print_output_pin(FILE* out, nnode_t* node) {
    /* now print the output */
    if (node->related_ast_node != NULL
        && global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED)
        fprintf(out, " %s^^%i-%i",
                node->name,
                node->related_ast_node->far_tag,
                node->related_ast_node->high_number);
    else
        fprintf(out, " %s", node->name);
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: print_dot_names_header)
 * 	The function that prints .model, .inputs, .outputs, etc.
 *---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::print_dot_names_header(FILE* out, nnode_t* node) {
    char** names = (char**)vtr::calloc(node->num_input_pins, sizeof(char*));

    // Create an implicit buffer if there are multiple drivers to the component
    for (int i = 0; i < node->num_input_pins; i++) {
        nnet_t* input_net = node->input_pins[i]->net;
        if (input_net->num_driver_pins > 1) {
            names[i] = op_node_name(BUF_NODE, node->name);
            // Assign each driver to the implicit buffer
            for (int j = 0; j < input_net->num_driver_pins; j++) {
                fprintf(out, ".names");
                print_net_driver(out, node, input_net, j);
                fprintf(out, " %s\n1 1\n\n", names[i]);
            }
        }
    }

    // Print the actual header
    fprintf(out, ".names");
    for (int i = 0; i < node->num_input_pins; i++) {
        if (names[i]) {
            // Use the implicit buffer we created before
            fprintf(out, " %s", names[i]);
            vtr::free(names[i]);
        } else {
            // Directly use the driver
            print_input_single_driver(out, node, i);
        }
    }
    vtr::free(names);

    oassert(node->num_output_pins == 1);
    print_output_pin(out, node);
    fprintf(out, "\n");
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: create_blif)
 * 	The function that creates the output blif file
 *---------------------------------------------------------------------------------------------
 */
FILE* BLIF::Writer::create_blif(const char* file_name) {
    FILE* out = NULL;

    /* open the file for output */
    if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED) {
        std::string out_file = "";
        out_file = out_file + file_name + "_" + global_args.high_level_block.value() + ".blif";
        out = fopen(out_file.c_str(), "w+");
    } else {
        out = fopen(file_name, "w+");
    }

    if (out == NULL) {
        error_message(NETLIST, unknown_location, "Could not open output file %s\n", file_name);
    }
    return out;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: output_blif)
 * 	The function that prints out the details for a blif formatted file
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::output_blif(FILE* out, const netlist_t* netlist) {
    fprintf(out, ".model %s\n", netlist->identifier);

    /* generate all the signals */

    fprintf(out, ".inputs");
    for (long i = 0; i < netlist->num_top_input_nodes; i++) {
        nnode_t* top_input_node = netlist->top_input_nodes[i];
        print_output_pin(out, top_input_node);
    }
    fprintf(out, "\n");

    fprintf(out, ".outputs");
    for (long i = 0; i < netlist->num_top_output_nodes; i++) {
        nnode_t* top_output_node = netlist->top_output_nodes[i];
        if (!top_output_node->input_pins[0]->net->num_driver_pins) {
            warning_message(NETLIST,
                            top_output_node->loc,
                            "This output is undriven (%s) and will be removed\n",
                            top_output_node->name);
        } else {
            print_output_pin(out, top_output_node);
        }
    }
    fprintf(out, "\n");

    /* add gnd, unconn, and vcc */
    fprintf(out, "\n.names gnd\n.names unconn\n.names vcc\n1\n");
    fprintf(out, "\n");

    // TODO Uncomment this for In Outs
    // connect all the outputs up to the last gate
    // for (long i = 0; i < netlist->num_top_output_nodes; i++) {
    //     nnode_t* node = netlist->top_output_nodes[i];
    //     for (int j = 0; j < node->num_input_pins; j++) {
    //         merge_with_inputs(node, j);
    //     }
    // }

    /* traverse the internals of the flat net-list */
    if (strcmp(configuration.output_type.c_str(), "blif") == 0) {
        depth_first_traversal_to_output(OUTPUT_TRAVERSE_VALUE, out, netlist);
    } else {
        error_message(NETLIST, unknown_location, "%s", "Invalid output file type.");
    }

    /* connect all the outputs up to the last gate */
    for (long i = 0; i < netlist->num_top_output_nodes; i++) {
        nnode_t* node = netlist->top_output_nodes[i];

        // TODO Change this to > 1 for In Outs
        if (node->input_pins[0]->net->num_fanout_pins > 0) {
            nnet_t* net = node->input_pins[0]->net;
            warn_undriven(node, net);
            for (int j = 0; j < net->num_driver_pins; j++) {
                fprintf(out, ".names");
                print_net_driver(out, node, net, j);
                print_output_pin(out, node);
                fprintf(out, "\n");

                fprintf(out, "1 1\n\n");
            }
        }
    }

    /* finish off the top level module */
    fprintf(out, ".end\n");
    fprintf(out, "\n");

    /* Print out any hard block modules */
    add_the_blackbox_for_mults(out);
    add_the_blackbox_for_adds(out);

    output_hard_blocks(out);
    fflush(out);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_to_parital_map()
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::depth_first_traversal_to_output(short marker_value, FILE* fp, const netlist_t* netlist) {
    int i;

    netlist->gnd_node->name = vtr::strdup("gnd");
    netlist->vcc_node->name = vtr::strdup("vcc");
    netlist->pad_node->name = vtr::strdup("unconn");
    /* now traverse the ground, vcc, and unconn pins */
    depth_traverse_output_blif(netlist->gnd_node, marker_value, fp);
    depth_traverse_output_blif(netlist->vcc_node, marker_value, fp);
    depth_traverse_output_blif(netlist->pad_node, marker_value, fp);

    /* start with the primary input list */
    for (i = 0; i < netlist->num_top_input_nodes; i++) {
        if (netlist->top_input_nodes[i] != NULL) {
            depth_traverse_output_blif(netlist->top_input_nodes[i], marker_value, fp);
        }
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::depth_traverse_output_blif(nnode_t* node, uintptr_t traverse_mark_number, FILE* fp) {
    int i, j;
    nnode_t* next_node;
    nnet_t* next_net;

    if (node->traverse_visited == traverse_mark_number) {
        return;
    } else {
        /* ELSE - this is a new node so depth visit it */

        /* POST traverse  map the node since you might delete */
        output_node(node, traverse_mark_number, fp);

        /* mark that we have visitied this node now */
        node->traverse_visited = traverse_mark_number;

        for (i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL)
                continue;

            next_net = node->output_pins[i]->net;
            for (j = 0; j < next_net->num_fanout_pins; j++) {
                if (next_net->fanout_pins[j] == NULL)
                    continue;

                next_node = next_net->fanout_pins[j]->node;
                if (next_node == NULL)
                    continue;

                /* recursive call point */
                depth_traverse_output_blif(next_node, traverse_mark_number, fp);
            }
        }
    }
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: output_node)
 * 	Depending on node type, figures out what to print for this node
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::output_node(nnode_t* node, short /*traverse_number*/, FILE* fp) {
    switch (node->type) {
        case GT:
            define_set_input_logical_function(node, "100 1\n", fp);
            oassert(node->num_input_pins == 3);
            oassert(node->input_pins[2] != NULL);
            break;
        case LT:
            define_set_input_logical_function(node, "010 1\n", fp); // last input decides if this
            oassert(node->num_input_pins == 3);
            oassert(node->input_pins[2] != NULL);
            break;
        case ADDER_FUNC:
            define_set_input_logical_function(node, "001 1\n010 1\n100 1\n111 1\n", fp);
            break;
        case CARRY_FUNC:
            define_set_input_logical_function(node, "011 1\n101 1\n110 1\n111 1\n", fp);
            break;
        case BITWISE_NOT:
            define_set_input_logical_function(node, "0 1\n", fp);
            break;
        case BUF_NODE:
            define_set_input_logical_function(node, "1 1\n", fp);
            break;
        case LOGICAL_AND:
        case LOGICAL_OR:
        case LOGICAL_XOR:
        case LOGICAL_XNOR:
        case LOGICAL_NAND:
        case LOGICAL_NOR:
        case LOGICAL_EQUAL:
        case NOT_EQUAL:
        case LOGICAL_NOT:
            define_logical_function(node, fp);
            break;

        case MUX_2:
            define_decoded_mux(node, fp);
            break;

        case FF_NODE:
            define_ff(node, fp);
            break;

        case MULTIPLY:
            oassert(hard_multipliers); /* should be soft logic! */
            define_mult_function(node, fp);
            break;

        case ADD:
            oassert(hard_adders); /* should be soft logic! */
            define_add_function(node, fp);
            break;

        case MINUS:
            oassert(hard_adders); /* should be soft logic! */
            define_add_function(node, fp);
            break;

        case MEMORY:
        case HARD_IP:
            define_hard_block(node, fp);
            break;
        case INPUT_NODE:
        case OUTPUT_NODE:
        case PAD_NODE:
        case CLOCK_NODE:
        case GND_NODE:
        case VCC_NODE:
            /* some nodes already converted */
            break;

        case BITWISE_AND:
        case BITWISE_NAND:
        case BITWISE_NOR:
        case BITWISE_XNOR:
        case BITWISE_XOR:
        case BITWISE_OR:
        case MULTI_PORT_MUX:
        case SL:
        case ASL:
        case SR:
        case ASR:
        case CASE_EQUAL:
        case CASE_NOT_EQUAL:
        case DIVIDE:
        case MODULO:
        case GTE:
        case LTE:
        default:
            /* these nodes should have been converted to softer versions */
            error_message(NETLIST, node->loc, "%s", "Output blif: node should have been converted to softer version.");
            break;
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: define_logical_function)
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_logical_function(nnode_t* node, FILE* out) {
    int i, j;
    char* temp_string;

    print_dot_names_header(out, node);

    /* print out the blif definition of this gate */
    switch (node->type) {
        case LOGICAL_AND: {
            /* generates: 111111 1 */
            for (i = 0; i < node->num_input_pins; i++) {
                fprintf(out, "1");
            }
            fprintf(out, " 1\n");
            break;
        }
        case LOGICAL_OR: {
            /* generates: 1----- 1\n-1----- 1\n ... */
            for (i = 0; i < node->num_input_pins; i++) {
                for (j = 0; j < node->num_input_pins; j++) {
                    if (i == j)
                        fprintf(out, "1");
                    else
                        fprintf(out, "-");
                }
                fprintf(out, " 1\n");
            }
            break;
        }
        case LOGICAL_NAND: {
            /* generates: 0----- 1\n-0----- 1\n ... */
            for (i = 0; i < node->num_input_pins; i++) {
                for (j = 0; j < node->num_input_pins; j++) {
                    if (i == j)
                        fprintf(out, "0");
                    else
                        fprintf(out, "-");
                }
                fprintf(out, " 1\n");
            }
            break;
        }
        case LOGICAL_NOT:
        case LOGICAL_NOR: {
            /* generates: 0000000 1 */
            for (i = 0; i < node->num_input_pins; i++) {
                fprintf(out, "0");
            }
            fprintf(out, " 1\n");
            break;
        }
        case LOGICAL_EQUAL:
        case LOGICAL_XOR: {
            oassert(node->num_input_pins <= 3);
            /* generates: a 1 when odd number of 1s */
            for (i = 0; i < my_power(2, node->num_input_pins); i++) {
                if ((i % 8 == 1) || (i % 8 == 2) || (i % 8 == 4) || (i % 8 == 7)) {
                    temp_string = convert_long_to_bit_string(i, node->num_input_pins);
                    fprintf(out, "%s", temp_string);
                    vtr::free(temp_string);
                    fprintf(out, " 1\n");
                }
            }
            break;
        }
        case NOT_EQUAL:
        case LOGICAL_XNOR: {
            oassert(node->num_input_pins <= 3);
            for (i = 0; i < my_power(2, node->num_input_pins); i++) {
                if ((i % 8 == 0) || (i % 8 == 3) || (i % 8 == 5) || (i % 8 == 6)) {
                    temp_string = convert_long_to_bit_string(i, node->num_input_pins);
                    fprintf(out, "%s", temp_string);
                    vtr::free(temp_string);
                    fprintf(out, " 1\n");
                }
            }
            break;
        }
        default:
            oassert(false);
            break;
    }

    fprintf(out, "\n");
}

/** 
 * ---------------------------------------------------------------------------------------------
 * (function: define_set_input_logical_function)
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_set_input_logical_function(nnode_t* node, const char* bit_output, FILE* out) {
    oassert(node->num_input_pins >= 1);

    print_dot_names_header(out, node);

    /* print out the blif definition of this gate */
    if (bit_output != NULL) {
        fprintf(out, "%s", bit_output);
    }
    fprintf(out, "\n");
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: define_ff)
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_ff(nnode_t* node, FILE* out) {
    oassert(node->num_output_pins == 1);
    oassert(node->num_input_pins == 2);

    // grab the edge sensitivity of the flip flop
    const char* edge_type_str = edge_type_blif_str(node);

    std::string input;
    std::string output;
    std::string clock_driver;

    fprintf(out, ".latch");

    /* input */
    print_input_single_driver(out, node, 0);

    /* output */
    print_output_pin(out, node);

    /* sensitivity */
    fprintf(out, " %s", edge_type_str);

    /* clock */
    print_input_single_driver(out, node, 1);

    /* initial value */
    fprintf(out, " %d\n\n", node->initial_value);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: define_decoded_mux)
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_decoded_mux(nnode_t* node, FILE* out) {
    oassert(node->input_port_sizes[0] == node->input_port_sizes[1]);
    print_dot_names_header(out, node);

    /* generates: 1----- 1\n-1----- 1\n ... */
    for (long i = 0; i < node->input_port_sizes[0]; i++) {
        for (long j = 0; j < node->num_input_pins; j++) {
            if (i == j)
                fprintf(out, "1");
            else if (i + node->input_port_sizes[0] == j)
                fprintf(out, "1");
            else if (i > node->input_port_sizes[0])
                fprintf(out, "0");
            else
                fprintf(out, "-");
        }
        fprintf(out, " 1\n");
    }

    fprintf(out, "\n");
}