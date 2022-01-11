/**
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
 * @file: definition of BLIF Writer routine to generate a BLIF output 
 * file. Unlike other tools that print each BLIF record in a single 
 * line, Odin-II BLIF Writer prints BLIF records in a specified size. 
 * That means it makes the BLIF file more readable by splitting the 
 * line by the specified threshold. 
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
#include "BLIFElaborate.hpp"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "netlist_cleanup.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"

#include "BLIF.hpp"

/**
 * -----------------------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------- Writer -----------------------------------------------------------
 * -----------------------------------------------------------------------------------------------------------------------------
 */

BLIF::Writer::Writer()
    : GenericWriter() {
    this->haveOutputLatchBlackbox = false;
}

BLIF::Writer::~Writer() = default;

inline void BLIF::Writer::_write(const netlist_t* netlist) {
    output_blif(this->output_file, netlist);
}

inline void BLIF::Writer::_create_file(const char* file_name, const file_type_e file_type) {
    // validate the file_name pionter
    oassert(file_name);
    // validate the file type
    if (file_type != _BLIF)
        error_message(UTIL, unknown_location,
                      "BLIF back-end entity cannot create file types(%d) other than BLIF", file_type);
    // create the BLIF file and set it as the output file
    this->output_file = create_blif(file_name);
}
/**
 * ---------------------------------------------------------------------------------------------
 * (function: warn_undriven)
 * 	
 * @brief to check is a net undriven
 * 
 * @param node pointer to the netlist node
 * @param net pointer to the net 
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
 * oassert(pin_idx < node->num_input_pins);
 * nnet_t* net = node->input_pins[pin_idx]->net;
 * warn_undriven(node, net);
 * // Merge node with all inputs with fanout of 1
 * if (net->num_fanout_pins <= 1) {
 * for (int i = 0; i < net->num_driver_pins; i++) {
 * npin_t* driver = net->driver_pins[i];
 * if (driver->name != NULL && ((driver->node->type == MULTIPLY) || (driver->node->type == HARD_IP) || (driver->node->type == MEMORY) || (driver->node->type == ADD) || (driver->node->type == MINUS))) {
 * vtr::free(driver->name);
 * driver->name = vtr::strdup(node->name);
 * } else {
 * vtr::free(driver->node->name);
 * driver->node->name = vtr::strdup(node->name);
 * }
 * }
 * }
 * } */

/**
 * ---------------------------------------------------------------------------------------------
 * (function: print_net_driver)
 * 
 * @brief The function that prints the driver of a given net
 *
 * @param out the output blif file
 * @param node pointer to the netlist node
 * @param net pointer to the net
 * @param driver_idx index of the driver pin
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
 * 
 * @brief The function that prints the single input pin driver 
 * 
 * @param out the output blif file
 * @param node pointer to the netlist node
 * @param pin_idx index of the driver pin
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
 * 
 * @brief The function that prints the output pins of a given node
 *
 * @param out the output blif file
 * @param node pointer to the netlist node
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
 * 
 * @brief The function that prints .model, .inputs, .outputs, etc.
 *
 * @param out the output blif file
 * @param node pointer to the netlist node
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
 * 
 * @brief The function that creates the output blif file
 *
 * @param file_name cstring of the output blif file name
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
 * 
 * @brief The function that prints out the details for a blif formatted file
 *
 * @param out the output blif file
 * @param netlist pointer to the netlist
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
    if (configuration.output_file_type == file_type_e::_BLIF) {
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
 *
 * @brief traverse the netlist to output the blif file
 *
 * @param marker_value unique traversal mark for output blif pass
 * @param fp the output blif file
 * @param netlist pointer to the netlist
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::depth_first_traversal_to_output(short marker_value, FILE* fp, const netlist_t* netlist) {
    int i;

    /* if a coarsen BLIF is recieved, these variables are already created */
    if (!coarsen_cleanup) {
        netlist->gnd_node->name = vtr::strdup("gnd");
        netlist->vcc_node->name = vtr::strdup("vcc");
        netlist->pad_node->name = vtr::strdup("unconn");
    }

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
 *
 * @brief traverse the netlist to output the blif file
 *
 * @param node pointer to the netlist node
 * @param traverse_mark_number unique traversal mark for output blif pass
 * @param fp the output blif file
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
 * 	
 * @brief Depending on node type, figures out what to print for this node
 *
 * @param node pointer to the netlist node
 * @param traverse_number unique traversal mark for output blif pass
 * @param fp the output blif file
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

        case SMUX_2:
            define_set_input_logical_function(node, "01- 1\n1-1 1\n", fp);
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
        case CLOCK_NODE:
            define_clock(node, fp);
            break;
        case INPUT_NODE:
        case OUTPUT_NODE:
        case PAD_NODE:
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
 * 
 * @brief print out the bit map of a logical function
 *
 * @param node pointer to the netlist node
 * @param out the output blif file
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
 *
 * @brief print out the given bit map
 *
 * @param node pointer to the netlist node
 * @param bit_output given bit map
 * @param out the output blif file
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
 * (function: define_clock)
 * 
 * @brief print out the clock node record
 *
 * @param node pointer to the netlist node
 * @param out the output blif file
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_clock(nnode_t* node, FILE* out) {
    oassert(node->num_input_pins < 2);

    if (node->num_input_pins == 1) {
        print_dot_names_header(out, node);

        /* print out the blif definition of this gate */
        fprintf(out, "%s", "1 1");

        fprintf(out, "\n");
    }
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: define_ff)
 * 
 * @brief print out the FF node record
 *
 * @param node pointer to the netlist node
 * @param out the output blif file
 * ---------------------------------------------------------------------------------------------
 */
void BLIF::Writer::define_ff(nnode_t* node, FILE* out) {
    oassert(node->num_output_pins == 1);
    oassert(node->num_input_pins == 2);

    // grab the edge sensitivity of the flip flop
    const char* clk_edge_type_str = edge_type_blif_str(node->attributes->clk_edge_type, node->loc);

    std::string input;
    std::string output;
    std::string clock_driver;

    fprintf(out, ".latch");

    /* input */
    print_input_single_driver(out, node, 0);

    /* output */
    print_output_pin(out, node);

    /* sensitivity */
    fprintf(out, " %s", clk_edge_type_str);

    /* clock */
    print_input_single_driver(out, node, 1);

    /* initial value */
    fprintf(out, " %d\n\n", node->initial_value);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: define_decoded_mux)
 * 
 * @brief print out the decoded mux node record
 *
 * @param node pointer to the netlist node
 * @param out the output blif file
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
