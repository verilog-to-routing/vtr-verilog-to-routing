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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"
#include "output_blif.h"

#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_util.h"
#include "vtr_memory.h"

bool haveOutputLatchBlackbox = false;

void depth_first_traversal_to_output(short marker_value, FILE* fp, netlist_t* netlist);
void depth_traverse_output_blif(nnode_t* node, int traverse_mark_number, FILE* fp);
void output_node(nnode_t* node, short traverse_number, FILE* fp);
void define_logical_function(nnode_t* node, FILE* out);
void define_set_input_logical_function(nnode_t* node, const char* bit_output, FILE* out);
void define_ff(nnode_t* node, FILE* out);
void define_decoded_mux(nnode_t* node, FILE* out);
void output_blif_pin_connect(nnode_t* node, FILE* out);

static void print_input_pin(FILE* out, nnode_t* node, long pin_idx) {
    oassert(pin_idx < node->num_input_pins);
    nnet_t* net = node->input_pins[pin_idx]->net;
    if (!net->driver_pin || !net->driver_pin->node) {
        // Add a warning for an undriven net.
        int line_number = node->related_ast_node ? node->related_ast_node->line_number : 0;
        warning_message(NETLIST_ERROR, line_number, -1,
                        "Net %s driving node %s is itself undriven.",
                        net->name, node->name);

        fprintf(out, " %s", "unconn");
    } else if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED
               && net->driver_pin->node->related_ast_node != NULL) {
        fprintf(out, " %s^^%i-%i",
                net->driver_pin->node->name,
                net->driver_pin->node->related_ast_node->far_tag,
                net->driver_pin->node->related_ast_node->high_number);
    } else {
        if (net->driver_pin->name != NULL && ((net->driver_pin->node->type == MULTIPLY) || (net->driver_pin->node->type == HARD_IP) || (net->driver_pin->node->type == MEMORY) || (net->driver_pin->node->type == ADD) || (net->driver_pin->node->type == MINUS))) {
            fprintf(out, " %s", net->driver_pin->name);
        } else {
            fprintf(out, " %s", net->driver_pin->node->name);
        }
    }
}

static void print_output_pin(FILE* out, nnode_t* node) {
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

static void print_input_pin_list(FILE* out, nnode_t* node) {
    for (long i = 0; i < node->num_input_pins; i++) {
        print_input_pin(out, node, i);
    }
}

static void print_dot_names_header(FILE* out, nnode_t* node) {
    fprintf(out, ".names");
    print_input_pin_list(out, node);

    oassert(node->num_output_pins == 1);
    print_output_pin(out, node);
    fprintf(out, "\n");
}

/*---------------------------------------------------------------------------
 * (function: output_blif)
 * 	The function that prints out the details for a blif formatted file
 *-------------------------------------------------------------------------*/
void output_blif(const char* file_name, netlist_t* netlist) {
    FILE* out;

    /* open the file for output */
    if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED) {
        std::string out_file = "";
        out_file = out_file + file_name + "_" + global_args.high_level_block.value() + ".blif";
        out = fopen(out_file.c_str(), "w+");
    } else {
        out = fopen(file_name, "w+");
    }

    if (out == NULL) {
        error_message(NETLIST_ERROR, -1, -1, "Could not open output file %s\n", file_name);
    }

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
        if (top_output_node->input_pins[0]->net->driver_pin == NULL) {
            warning_message(NETLIST_ERROR,
                            top_output_node->related_ast_node->line_number,
                            top_output_node->related_ast_node->file_number,
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

    /* traverse the internals of the flat net-list */
    if (strcmp(configuration.output_type.c_str(), "blif") == 0) {
        depth_first_traversal_to_output(OUTPUT_TRAVERSE_VALUE, out, netlist);
    } else {
        error_message(NETLIST_ERROR, 0, -1, "%s", "Invalid output file type.");
    }

    /* connect all the outputs up to the last gate */
    for (long i = 0; i < netlist->num_top_output_nodes; i++) {
        nnode_t* node = netlist->top_output_nodes[i];

        fprintf(out, ".names");
        print_input_pin(out, node, 0);
        print_output_pin(out, node);
        fprintf(out, "\n");

        fprintf(out, "1 1\n\n");
    }

    /* finish off the top level module */
    fprintf(out, ".end\n");
    fprintf(out, "\n");

    /* Print out any hard block modules */
    add_the_blackbox_for_mults(out);
    add_the_blackbox_for_adds(out);

    output_hard_blocks(out);
    fclose(out);
}

/*---------------------------------------------------------------------------
 * (function: depth_first_traversal_to_parital_map()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_output(short marker_value, FILE* fp, netlist_t* netlist) {
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

/*--------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *------------------------------------------------------------------------*/
void depth_traverse_output_blif(nnode_t* node, int traverse_mark_number, FILE* fp) {
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
/*-------------------------------------------------------------------
 * (function: output_node)
 * 	Depending on node type, figures out what to print for this node
 *------------------------------------------------------------------*/
void output_node(nnode_t* node, short /*traverse_number*/, FILE* fp) {
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

        //case FULLADDER:
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
            error_message(NETLIST_ERROR, 0, -1, "%s", "Output blif: node should have been converted to softer version.");
            break;
    }
}

/*-------------------------------------------------------------------------
 * (function: define_logical_function)
 *-----------------------------------------------------------------------*/
void define_logical_function(nnode_t* node, FILE* out) {
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

/*------------------------------------------------------------------------
 * (function: define_set_input_logical_function)
 *----------------------------------------------------------------------*/
void define_set_input_logical_function(nnode_t* node, const char* bit_output, FILE* out) {
    oassert(node->num_input_pins >= 1);

    print_dot_names_header(out, node);

    /* print out the blif definition of this gate */
    if (bit_output != NULL) {
        fprintf(out, "%s", bit_output);
    }
    fprintf(out, "\n");
}

/*-------------------------------------------------------------------------
 * (function: define_ff)
 *-----------------------------------------------------------------------*/
void define_ff(nnode_t* node, FILE* out) {
    oassert(node->num_output_pins == 1);
    oassert(node->num_input_pins == 2);

    int initial_value = global_args.sim_initial_value;
    if (node->has_initial_value)
        initial_value = node->initial_value;

    /* By default, latches value are unknown, represented by 3 in a BLIF file
     * and by -1 internally in ODIN */
    // TODO switch to default!! to avoid confusion
    if (initial_value == -1)
        initial_value = 3;

    // grab the edge sensitivity of the flip flop
    const char* edge_type_str = edge_type_blif_str(node);

    std::string input;
    std::string output;
    std::string clock_driver;

    fprintf(out, ".latch");

    /* input */
    print_input_pin(out, node, 0);

    /* output */
    print_output_pin(out, node);

    /* sensitivity */
    fprintf(out, " %s", edge_type_str);

    /* clock */
    print_input_pin(out, node, 1);

    /* initial value */
    fprintf(out, " %d\n\n", initial_value);
}

/*--------------------------------------------------------------------------
 * (function: define_decoded_mux)
 *------------------------------------------------------------------------*/
void define_decoded_mux(nnode_t* node, FILE* out) {
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
