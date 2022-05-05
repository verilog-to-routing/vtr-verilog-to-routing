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
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <string>
#include "odin_types.h"
#include "odin_util.h"
#include "node_creation_library.h"
#include "multipliers.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "read_xml_arch_file.h"
#include "odin_globals.h"

#include "adders.h"

#include "vtr_memory.h"
#include "vtr_list.h"
#include "vtr_util.h"

using vtr::insert_in_vptr_list;
using vtr::t_linked_vptr;

t_model* hard_multipliers = NULL;
t_linked_vptr* mult_list = NULL;
int min_mult = 0;
int* mults = NULL;

void record_mult_distribution(nnode_t* node);
void terminate_mult_distribution();
void init_split_multiplier(nnode_t* node, nnode_t* ptr, int offa, int a, int offb, int b, nnode_t* node_a, nnode_t* node_b);
void init_multiplier_adder(nnode_t* node, nnode_t* parent, int a, int b);
void split_multiplier_a(nnode_t* node, int a0, int a1, int b);
void split_multiplier_b(nnode_t* node, int a, int b1, int b0);
void pad_multiplier(nnode_t* node, netlist_t* netlist);
void split_soft_multiplier(nnode_t* node, netlist_t* netlist);
static mult_port_stat_e is_constant_multipication(nnode_t* node, netlist_t* netlist);
static signal_list_t* implement_constant_multipication(nnode_t* node, mult_port_stat_e port_status, short mark, netlist_t* netlist);
static nnode_t* perform_const_mult_optimization(mult_port_stat_e mult_port_stat, nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void cleanup_mult_old_node(nnode_t* nodeo, netlist_t* netlist);

// data structure representing a row of bits an adder tree
struct AdderTreeRow {
    // the shift of this row from the least significant bit of the multiplier output
    size_t shift;
    // array representing the bits in the row, each bit is a node
    // pointer and the index of this bit in this node output array.
    std::vector<std::pair<nnode_t*, int>> bits;
};

/*---------------------------------------------------------------------------
 * (function: instantiate_simple_soft_multiplier )
 * Sample 4x4 multiplier to help understand logic.
 *
 * 					a3 	a2	a1	a0
 *					b3 	b2 	b1 	b0
 *					---------------------------
 *					c03	c02	c01	c00
 *			+	c13	c12	c11	c10
 *			-----------------------------------
 *			r14	r13	r12	r11	r10
 *		+	c23	c22	c21	c20
 *		-----------------------------------
 *		r24	r23	r22	r21	r20
 *	+	c33	c32	c31	c30
 *	------------------------------------
 *	o7	o6	o5	o4	o3	o2	o1	o0
 *
 *	In the first case will be c01
 *-------------------------------------------------------------------------*/
void instantiate_simple_soft_multiplier(nnode_t* node, short mark, netlist_t* netlist) {
    int width_a;
    int width_b;
    int width;
    int multiplier_width;
    int multiplicand_width;
    nnode_t** adders_for_partial_products;
    nnode_t*** partial_products;
    int multiplicand_offset_index;
    int multiplier_offset_index;
    int current_index;
    int i, j;

    /* need for an carry-ripple-adder for each of the bits of port B. */
    /* good question of which is better to put on the bottom of multiplier.  Larger means more smaller adds, or small is
     * less large adds */
    oassert(node->num_output_pins > 0);
    oassert(node->num_input_pins > 0);
    oassert(node->num_input_port_sizes == 2);
    oassert(node->num_output_port_sizes == 1);
    width_a = node->input_port_sizes[0];
    width_b = node->input_port_sizes[1];
    width = node->output_port_sizes[0];
    multiplicand_width = width_b;
    multiplier_width = width_a;
    /* offset is related to which multport is chosen as the multiplicand */
    multiplicand_offset_index = width_a;
    multiplier_offset_index = 0;

    adders_for_partial_products = (nnode_t**)vtr::malloc(sizeof(nnode_t*) * multiplicand_width - 1);

    /* need to generate partial products for each bit in width B. */
    partial_products = (nnode_t***)vtr::malloc(sizeof(nnode_t**) * multiplicand_width);

    /* generate the AND partial products */
    for (i = 0; i < multiplicand_width; i++) {
        /* create the memory for each AND gate needed for the levels of partial products */
        partial_products[i] = (nnode_t**)vtr::malloc(sizeof(nnode_t*) * multiplier_width);

        if (i < multiplicand_width - 1) {
            adders_for_partial_products[i] = make_2port_gate(ADD, multiplier_width + 1, multiplier_width + 1, multiplier_width + 1, node, mark);
        }

        for (j = 0; j < multiplier_width; j++) {
            /* create each one of the partial products */
            partial_products[i][j] = make_1port_logic_gate(LOGICAL_AND, 2, node, mark);
        }
    }

    /* generate the connections to the AND gates */
    for (i = 0; i < multiplicand_width; i++) {
        for (j = 0; j < multiplier_width; j++) {
            /* hookup the input of B to each AND gate */
            if (j == 0) {
                /* IF - this is the first time we are mapping multiplicand port then can remap */
                remap_pin_to_new_node(node->input_pins[i + multiplicand_offset_index], partial_products[i][j], 0);
            } else {
                /* ELSE - this needs to be a new output of the multiplicand port */
                add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[i][0]->input_pins[0]), 0);
            }

            /* hookup the input of the multiplier to each AND gate */
            if (i == 0) {
                /* IF - this is the first time we are mapping multiplier port then can remap */
                remap_pin_to_new_node(node->input_pins[j + multiplier_offset_index], partial_products[i][j], 1);
            } else {
                /* ELSE - this needs to be a new output of the multiplier port */
                add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[0][j]->input_pins[1]), 1);
            }
        }
    }

    /* hookup each of the adders */
    for (i = 0; i < multiplicand_width - 1; i++) // -1 since the first stage is a combo of partial products while all others are part of tree
    {
        for (j = 0; j < multiplier_width + 1; j++) // +1 since adders are one greater than multwidth to pass carry
        {
            /* join to port 1 of the add one of the partial products.  */
            if (i == 0) {
                /* IF - this is the first addition row, then adding two sets of partial products and first set is from the c0* */
                if (j < multiplier_width - 1) {
                    /* IF - we just take an element of the first list c[0][j+1]. */
                    connect_nodes(partial_products[i][j + 1], 0, adders_for_partial_products[i], j);
                } else {
                    /* ELSE - this is the last input to the first adder, then we pass in 0 since no carry yet */
                    add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j);
                }
            } else if (j < multiplier_width) {
                /* ELSE - this is the standard situation when we need to hookup this adder with a previous adder, r[i-1][j+1] */
                connect_nodes(adders_for_partial_products[i - 1], j + 1, adders_for_partial_products[i], j);
            } else {
                add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j);
            }

            if (j < multiplier_width) {
                /* IF - this is not most significant bit then just add current partial product */
                connect_nodes(partial_products[i + 1][j], 0, adders_for_partial_products[i], j + multiplier_width + 1);
            } else {
                add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j + multiplier_width + 1);
            }
        }
    }

    current_index = 0;
    /* hookup the outputs */
    for (i = 0; i < width; i++) {
        if (multiplicand_width == 1) {
            // this is undealt with
            error_message(AST, node->loc, "%s", "Cannot create soft multiplier with multiplicand width of 1.\n");
        } else if (i == 0) {
            /* IF - this is the LSbit, then we use a pass through from the partial product */
            remap_pin_to_new_node(node->output_pins[i], partial_products[0][0], 0);
        } else if (i < multiplicand_width - 1) {
            /* ELSE IF - these are the middle values that come from the LSbit of partial adders */
            remap_pin_to_new_node(node->output_pins[i], adders_for_partial_products[i - 1], 0);
        } else {
            if (current_index > multiplier_width) {
                /* output pins greater than 2X multiplier width will be driven with pad node */
                nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, mark);
                /* hook a pad pin into buf node */
                add_input_pin_to_node(buf_node, get_pad_pin(netlist), 0);
                /* hook the over size output pin to into the buf node */
                remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
            } else {
                /* ELSE - the final outputs are straight from the outputs of the last adder */
                remap_pin_to_new_node(node->output_pins[i], adders_for_partial_products[multiplicand_width - 2], current_index);
            }
            current_index++;
        }
    }

    /* soft map the adders if they need to be mapped */
    for (i = 0; i < multiplicand_width - 1; i++) {
        instantiate_add_w_carry(adders_for_partial_products[i], mark, netlist);
    }

    /* Cleanup everything */
    if (adders_for_partial_products != NULL) {
        for (i = 0; i < multiplicand_width - 1; i++) {
            free_nnode(adders_for_partial_products[i]);
        }
        vtr::free(adders_for_partial_products);
    }
    /* generate the AND partial products */
    for (i = 0; i < multiplicand_width; i++) {
        /* create the memory for each AND gate needed for the levels of partial products */
        if (partial_products[i] != NULL) {
            vtr::free(partial_products[i]);
        }
    }
    if (partial_products != NULL) {
        vtr::free(partial_products);
    }
}

/**
 * --------------------------------------------------------------------------
 * (function: implement_constant_multipication)
 * 
 * @brief implementing constant multipication utilizing shift and ADD operations
 * 
 * @note this function should call before partial mapping phase
 * since some logic need to be softened
 * 
 * @param node pointer to the multipication netlist node
 * @param port_status showing which value is constant, which is variable
 * @param mark a unique DFS traversal number
 * @param netlist pointer to the current netlist
 * 
 * @return output signal
 * -------------------------------------------------------------------------*/
static signal_list_t* implement_constant_multipication(nnode_t* node, mult_port_stat_e port_status, short mark, netlist_t* netlist) {
    /* validate the port sizes */
    oassert(node->num_input_port_sizes == 2);
    oassert(node->num_output_port_sizes == 1);

    signal_list_t* return_value = init_signal_list();

    /**
     * Multiply ports
     * IN1: (n bits)        input_port[0]
     * IN2: (m bits)        input_port[1]
     * OUT: min(m, n) bits  output_port[0]
     */

    int IN1_width = node->input_port_sizes[0];

    int i, j;
    int const_operand_offset = (port_status == mult_port_stat_e::MULTIPICAND_CONSTANT) ? IN1_width : 0;
    int const_operand_width = node->input_port_sizes[(port_status == mult_port_stat_e::MULTIPICAND_CONSTANT) ? 1 : 0];

    int variable_operand_offset = (port_status == mult_port_stat_e::MULTIPICAND_CONSTANT) ? 0 : IN1_width;
    int variable_operand_width = node->num_input_pins - const_operand_width;
    operation_list variable_operand_signedness = (port_status == mult_port_stat_e::MULTIPICAND_CONSTANT)
                                                     ? node->attributes->port_a_signed
                                                     : node->attributes->port_b_signed;

    /* after each level one bit will be added to the width of results */
    int width = node->num_output_pins;

    /* container for constatnt operand */
    signal_list_t* const_operand = init_signal_list();
    for (i = 0; i < const_operand_width; i++) {
        add_pin_to_signal_list(const_operand, node->input_pins[const_operand_offset + i]);
    }
    /* container for variable operand */
    signal_list_t* variable_operand = init_signal_list();
    for (i = 0; i < variable_operand_width; i++) {
        add_pin_to_signal_list(variable_operand, node->input_pins[variable_operand_offset + i]);
    }

    /* netlist GND and VCC net */
    nnet_t* gnd_net = netlist->zero_net;
    nnet_t* vcc_net = netlist->one_net;

    int internal_outputs_size = const_operand_width;
    /* to keep the record of internal outputs for connection purposes */
    signal_list_t** internal_outputs = (signal_list_t**)vtr::calloc(internal_outputs_size, sizeof(signal_list_t*));
    /* implementing the multipication using shift and add operation */
    for (i = 0; i < node->num_output_pins + 1; i++) {
        npin_t* pin;
        /* checking a couple conditions to avoid going further if there is not needed */
        if (i == node->num_output_pins || i == const_operand_width) {
            internal_outputs_size = i;
            /* initializing the return value */
            for (j = 0; j < internal_outputs[i - 1]->count; j++) {
                add_pin_to_signal_list(return_value, internal_outputs[i - 1]->pins[j]);
            }
            break;
        } else {
            pin = const_operand->pins[i];
        }
        /* init the interanl outputs signal list */
        internal_outputs[i] = init_signal_list();

        /* if the pin is GND we pass */
        if (!strcmp(pin->net->name, gnd_net->name)) {
            for (j = 0; j < width; j++) {
                /* if the first bit of const_operand is zero we need to initiate the multipication by zero pins */
                npin_t* internal_output_pin = (i == 0) ? get_zero_pin(netlist)
                                                       : internal_outputs[i - 1]->pins[j];
                add_pin_to_signal_list(internal_outputs[i], internal_output_pin);
            }
        }
        /* the const_operand pin is connected to VCC */
        else if (!strcmp(pin->net->name, vcc_net->name)) {
            /* for the first round we do not need to shift */
            if (i == 0) {
                for (j = 0; j < width; j++) {
                    if (j < variable_operand_width) {
                        add_pin_to_signal_list(internal_outputs[0], copy_input_npin(variable_operand->pins[j]));
                    } else {
                        add_pin_to_signal_list(internal_outputs[0], get_zero_pin(netlist));
                    }
                }
            } else {
                /*****************************************************************************************/
                /*************************************** SHIFT_NODE **************************************/
                /*****************************************************************************************/
                /**
                 * create a shift node to shift the variable port based on the i idx 
                 * 
                 * (shift node)
                 * IN1: variable_operand of the multiplier
                 * IN2: shift value (const_operand_width maximum size)
                 * OUT: shifted IN1 (width)
                 * 
                 */
                nnode_t* shift_node = make_2port_gate(SL, width, width, width, node, mark);
                /* connecting the shift value pins */
                signal_list_t* shift_value = create_constant_signal(i, width, netlist);

                /* keeping the shift output nodes for adding with the previous stage internal outputs */
                signal_list_t* shift_outputs = init_signal_list();

                int pad_pin = variable_operand->count - 1;
                for (j = 0; j < width; j++) {
                    if (j < variable_operand_width) {
                        /* connecing the first input of the shift node */
                        add_input_pin_to_node(shift_node,
                                              copy_input_npin(variable_operand->pins[j]),
                                              j);
                    } else {
                        add_input_pin_to_node(shift_node,
                                              (variable_operand_signedness == SIGNED)
                                                  ? copy_input_npin(variable_operand->pins[pad_pin])
                                                  : get_zero_pin(netlist),
                                              j);
                    }

                    /* hook shift value pins into the shift node */
                    add_input_pin_to_node(shift_node,
                                          shift_value->pins[j],
                                          width + j);

                    /* Specifying the level_muxes outputs */
                    // Connect output pin to related input pin
                    npin_t* var_op_out1 = allocate_npin();
                    npin_t* var_op_out2 = allocate_npin();
                    nnet_t* var_op_net = allocate_nnet();
                    var_op_net->name = make_full_ref_name(NULL, NULL, NULL, shift_node->name, j);
                    /* hook the output pin into the node */
                    add_output_pin_to_node(shift_node, var_op_out1, j);
                    /* hook up new pin 1 into the new net */
                    add_driver_pin_to_net(var_op_net, var_op_out1);
                    /* hook up the new pin 2 to this new net */
                    add_fanout_pin_to_net(var_op_net, var_op_out2);

                    /* adding the output pin to the shoft output signal container */
                    add_pin_to_signal_list(shift_outputs, var_op_out2);
                }

                /*****************************************************************************************/
                /**************************************** ADD_NODE ***************************************/
                /*****************************************************************************************/
                nnode_t* add_node = make_2port_gate(ADD, width, width, width, node, mark);
                add_list = insert_in_vptr_list(add_list, add_node);
                /* connecting add node input pins */
                for (j = 0; j < width; j++) {
                    /* connecting the previous stage internal outputs as the first add inputs */
                    add_input_pin_to_node(add_node, internal_outputs[i - 1]->pins[j], j);

                    /* connecting the shift output pins as the second input */
                    add_input_pin_to_node(add_node, shift_outputs->pins[j], width + j);

                    /* creating new output pins and adding to the internal outputs for next stages */
                    // Connect output pin to related input pin
                    npin_t* add_op_out1 = allocate_npin();
                    npin_t* add_op_out2 = allocate_npin();
                    nnet_t* add_op_net = allocate_nnet();
                    add_op_net->name = make_full_ref_name(NULL, NULL, NULL, add_node->name, j);
                    /* hook the output pin into the node */
                    add_output_pin_to_node(add_node, add_op_out1, j);
                    /* hook up new pin 1 into the new net */
                    add_driver_pin_to_net(add_op_net, add_op_out1);
                    /* hook up the new pin 2 to this new net */
                    add_fanout_pin_to_net(add_op_net, add_op_out2);

                    /* adding the output pin to the shoft output signal container */
                    add_pin_to_signal_list(internal_outputs[i], add_op_out2);
                }

                // CLEAN UP
                free_signal_list(shift_value);
                free_signal_list(shift_outputs);
            }
        }
    }

    // CLEAN UP
    free_signal_list(const_operand);
    free_signal_list(variable_operand);

    for (i = 0; i < internal_outputs_size; i++) {
        if (internal_outputs[i])
            free_signal_list(internal_outputs[i]);
    }
    vtr::free(internal_outputs);

    return (return_value);
}

/**
 * --------------------------------------------------------------------------
 * (function: connect_constant_mult_outputs)
 * 
 * @brief connecting the constant multipication
 * pins to the main mult node
 * 
 * @param node pointer to the multipication netlist node
 * @param output_signal_list list of pins
 * @param netlist pointer to the current netlist file
 * -------------------------------------------------------------------------*/
void connect_constant_mult_outputs(nnode_t* node, signal_list_t* output_signal_list) {
    /* validate the size of output width and num of signals */
    int output_width = node->num_output_pins;
    oassert(output_width == output_signal_list->count);

    int i;
    /* hook the output signals into the node output */
    for (i = 0; i < output_signal_list->count; i++) {
        npin_t* pin = output_signal_list->pins[i];
        /* join nets of the output pin and the calculated pin */
        nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, node->traverse_visited);

        /* connect the mults output pins as buf node driver */
        add_input_pin_to_node(buf_node, pin, 0);
        /* remap the main mult output pin to the buf node output pin */
        remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
    }

    // CLEAN UP
    free_signal_list(output_signal_list);
    for (i = 0; i < node->num_input_pins; i++) {
        npin_t* pin = node->input_pins[i];

        /* detach from input nets */
        remove_fanout_pins_from_net(pin->net, pin, pin->pin_net_idx);

        /* free pin */
        free_npin(node->input_pins[i]);
        node->input_pins[i] = NULL;
    }

    free_nnode(node);
}

/*---------------------------------------------------------------------------
 * (function: init_mult_distribution)
 *-------------------------------------------------------------------------*/
void init_mult_distribution() {
    oassert(hard_multipliers != NULL);
    int len = (1 + hard_multipliers->inputs->size) * (1 + hard_multipliers->inputs->next->size);
    mults = (int*)vtr::calloc(len, sizeof(int));
}

/*---------------------------------------------------------------------------
 * (function: record_mult_distribution)
 *-------------------------------------------------------------------------*/
void record_mult_distribution(nnode_t* node) {
    int a, b;

    oassert(hard_multipliers != NULL);
    oassert(node != NULL);

    a = node->input_port_sizes[0];
    b = node->input_port_sizes[1];

    mults[a * hard_multipliers->inputs->size + b] += 1;
    return;
}

/*---------------------------------------------------------------------------
 * (function: report_mult_distribution)
 *-------------------------------------------------------------------------*/
void report_mult_distribution() {
    long num_total = 0;

    if (hard_multipliers == NULL)
        return;

    printf("\nHard Multiplier Distribution\n");
    printf("============================\n");
    for (long i = 0; i <= hard_multipliers->inputs->size; i++) {
        for (long j = 1; j <= hard_multipliers->inputs->next->size; j++) {
            if (mults[i * hard_multipliers->inputs->size + j] != 0) {
                num_total += mults[i * hard_multipliers->inputs->size + j];
                printf("%ld X %ld => %d\n", i, j, mults[i * hard_multipliers->inputs->size + j]);
            }
        }
    }
    printf("\n");
    printf("\nTotal # of multipliers = %ld\n", num_total);
    vtr::free(mults);
}

/*---------------------------------------------------------------------------
 * (function: find_hard_multipliers)
 *-------------------------------------------------------------------------*/
void find_hard_multipliers() {
    hard_multipliers = Arch.models;
    min_mult = configuration.min_hard_multiplier;
    while (hard_multipliers != NULL) {
        if (strcmp(hard_multipliers->name, "multiply") == 0) {
            init_mult_distribution();
            return;
        } else {
            hard_multipliers = hard_multipliers->next;
        }
    }

    return;
}

/*---------------------------------------------------------------------------
 * (function: declare_hard_multiplier)
 *-------------------------------------------------------------------------*/
void declare_hard_multiplier(nnode_t* node) {
    t_multiplier* tmp;
    int width_a, width_b, width, swap;

    /* See if this size instance of multiplier exists? */
    if (hard_multipliers == NULL)
        warning_message(NETLIST, node->loc, "%s\n", "Instantiating Mulitpliers where hard multipliers do not exist");

    tmp = (t_multiplier*)hard_multipliers->instances;
    width_a = node->input_port_sizes[0];
    width_b = node->input_port_sizes[1];
    width = node->output_port_sizes[0];
    if (width_a < width_b) /* Make sure a is bigger than b */
    {
        swap = width_b;
        width_b = width_a;
        width_a = swap;
    }
    while (tmp != NULL) {
        if ((tmp->size_a == width_a) && (tmp->size_b == width_b) && (tmp->size_out == width))
            return;
        else
            tmp = tmp->next;
    }

    /* Does not exist - must create an instance */
    tmp = (t_multiplier*)vtr::malloc(sizeof(t_multiplier));
    tmp->next = (t_multiplier*)hard_multipliers->instances;
    hard_multipliers->instances = tmp;
    tmp->size_a = width_a;
    tmp->size_b = width_b;
    tmp->size_out = width;
    return;
}

/*---------------------------------------------------------------------------
 * (function: instantiate_hard_multiplier )
 *-------------------------------------------------------------------------*/
void instantiate_hard_multiplier(nnode_t* node, short mark, netlist_t* /*netlist*/) {
    oassert(node
            && "node is NULL to instanciate hard multiplier");

    declare_hard_multiplier(node);

    std::string node_name = "";
    if (node->name) {
        node_name = node->name;
        vtr::free(node->name);
        node->name = NULL;
    }

    if (node->num_output_pins <= 0) {
        /* wide input first :) */
        int portA = 0;
        int portB = 1;
        if (node->input_port_sizes[1] > node->input_port_sizes[0]) {
            portA = 1;
            portB = 0;
        }
        std::string tmp(
            node_name + "_" + std::to_string(node->input_port_sizes[portA]) + "_" + std::to_string(node->input_port_sizes[portB]) + "_" + std::to_string(node->output_port_sizes[0]));
        node->name = vtr::strdup(tmp.c_str());
    } else {
        /* Give names to the output pins */
        for (int i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->name) {
                vtr::free(node->output_pins[i]->name);
            }
            //build the output string
            std::string tmp(
                node_name + "[" + std::to_string(node->output_pins[i]->pin_node_idx) + "]");
            node->output_pins[i]->name = vtr::strdup(tmp.c_str());
        }
        node->name = vtr::strdup(node->output_pins[node->num_output_pins - 1]->name);
    }
    node->traverse_visited = mark;
    return;
}

/*----------------------------------------------------------------------------
 * function: add_the_blackbox_for_mults()
 *--------------------------------------------------------------------------*/
void add_the_blackbox_for_mults(FILE* out) {
    long i;
    int count;
    int hard_mult_inputs;
    t_multiplier* muls;
    t_model_ports* ports;
    char buffer[MAX_BUF];
    char *pa, *pb, *po;

    /* Check to make sure this target architecture has hard multipliers */
    if (hard_multipliers == NULL)
        return;

    /* Get the names of the ports for the multiplier */
    ports = hard_multipliers->inputs;
    pb = ports->name;
    ports = ports->next;
    pa = ports->name;
    po = hard_multipliers->outputs->name;

    /* find the multiplier devices in the tech library */
    muls = (t_multiplier*)(hard_multipliers->instances);
    if (muls == NULL) /* No multipliers instantiated */
        return;

    /* simplified way of getting the multsize, but fine for quick example */
    while (muls != NULL) {
        /* write out this multiplier model */
        if (configuration.fixed_hard_multiplier != 0)
            count = fprintf(out, ".model multiply\n");
        else
            count = fprintf(out, ".model mult_%d_%d_%d\n", muls->size_a, muls->size_b, muls->size_out);

        /* add the inputs */
        count = count + fprintf(out, ".inputs");
        hard_mult_inputs = muls->size_a + muls->size_b;
        for (i = 0; i < hard_mult_inputs; i++) {
            if (i < muls->size_a) {
                count = count + odin_sprintf(buffer, " %s[%ld]", pa, i);
            } else {
                count = count + odin_sprintf(buffer, " %s[%ld]", pb, i - muls->size_a);
            }

            if (count > 78)
                count = fprintf(out, " \\\n %s", buffer) - 3;
            else
                fprintf(out, " %s", buffer);
        }
        fprintf(out, "\n");

        /* add the outputs */
        count = fprintf(out, ".outputs");
        for (i = 0; i < muls->size_out; i++) {
            count = count + odin_sprintf(buffer, " %s[%ld]", po, i);
            if (count > 78) {
                fprintf(out, " \\\n%s", buffer);
                count = strlen(buffer);
            } else
                fprintf(out, "%s", buffer);
        }
        fprintf(out, "\n");
        fprintf(out, ".blackbox\n");
        fprintf(out, ".end\n");
        fprintf(out, "\n");

        muls = muls->next;
    }

    free_multipliers();
}

/*-------------------------------------------------------------------------
 * (function: define_mult_function)
 *-----------------------------------------------------------------------*/
void define_mult_function(nnode_t* node, FILE* out) {
    long i, j;
    int count;
    char buffer[MAX_BUF];

    count = fprintf(out, "\n.subckt");
    count--;
    oassert(node->input_port_sizes[0] > 0);
    oassert(node->input_port_sizes[1] > 0);
    oassert(node->output_port_sizes[0] > 0);

    int flip = false;

    if (configuration.fixed_hard_multiplier != 0) {
        count += fprintf(out, " multiply");
    } else {
        if (node->input_port_sizes[0] > node->input_port_sizes[1]) {
            count += fprintf(out, " mult_%d_%d_%d", node->input_port_sizes[0],
                             node->input_port_sizes[1], node->output_port_sizes[0]);

            flip = false;
        } else {
            count += fprintf(out, " mult_%d_%d_%d", node->input_port_sizes[1],
                             node->input_port_sizes[0], node->output_port_sizes[0]);

            flip = true;
        }
    }

    for (i = 0; i < node->num_input_pins; i++) {
        if (i < node->input_port_sizes[flip ? 1 : 0]) {
            int input_index = flip ? i + node->input_port_sizes[0] : i;
            nnet_t* net = node->input_pins[input_index]->net;
            oassert(net->num_driver_pins == 1);
            npin_t* driver_pin = net->driver_pins[0];

            if (!driver_pin->name)
                j = odin_sprintf(buffer, " %s[%ld]=%s", hard_multipliers->inputs->next->name, i, driver_pin->node->name);
            else
                j = odin_sprintf(buffer, " %s[%ld]=%s", hard_multipliers->inputs->next->name, i, driver_pin->name);
        } else {
            int input_index = flip ? i - node->input_port_sizes[1] : i;
            nnet_t* net = node->input_pins[input_index]->net;
            oassert(net->num_driver_pins == 1);
            npin_t* driver_pin = net->driver_pins[0];

            long index = flip
                             ? i - node->input_port_sizes[1]
                             : i - node->input_port_sizes[0];

            if (!driver_pin->name)
                j = odin_sprintf(buffer, " %s[%ld]=%s", hard_multipliers->inputs->name, index, driver_pin->node->name);
            else
                j = odin_sprintf(buffer, " %s[%ld]=%s", hard_multipliers->inputs->name, index, driver_pin->name);
        }

        if (count + j > 79) {
            fprintf(out, "\\\n");
            count = 0;
        }
        count += fprintf(out, "%s", buffer);
    }

    for (i = 0; i < node->num_output_pins; i++) {
        j = odin_sprintf(buffer, " %s[%ld]=%s", hard_multipliers->outputs->name, i, node->output_pins[i]->name);
        if (count + j > 79) {
            fprintf(out, "\\\n");
            count = 0;
        }
        count += fprintf(out, "%s", buffer);
    }

    fprintf(out, "\n\n");
    return;
}

/*-----------------------------------------------------------------------
 * (function: init_split_multiplier)
 *	Create a dummy multiplier when spliting. Inputs are connected
 *	to original pins, output pins are set to NULL for later connecting
 *	with temp pins to connect cascading multipliers/adders.
 *---------------------------------------------------------------------*/
void init_split_multiplier(nnode_t* node, nnode_t* ptr, int offa, int a, int offb, int b, nnode_t* node_a, nnode_t* node_b) {
    int i;

    /* Copy properties from original node */
    ptr->type = node->type;
    ptr->related_ast_node = node->related_ast_node;
    ptr->traverse_visited = node->traverse_visited;
    ptr->node_data = NULL;

    /* Set new port sizes and parameters */
    ptr->num_input_port_sizes = 2;
    ptr->input_port_sizes = (int*)vtr::malloc(2 * sizeof(int));
    ptr->input_port_sizes[0] = a;
    ptr->input_port_sizes[1] = b;
    ptr->num_output_port_sizes = 1;
    ptr->output_port_sizes = (int*)vtr::malloc(sizeof(int));
    ptr->output_port_sizes[0] = a + b;

    /* Set the number of pins and re-locate previous pin entries */
    ptr->num_input_pins = a + b;
    ptr->input_pins = (npin_t**)vtr::malloc(sizeof(void*) * (a + b));
    for (i = 0; i < a; i++) {
        if (node_a)
            add_input_pin_to_node(ptr, copy_input_npin(node_a->input_pins[i]), i);
        else
            remap_pin_to_new_node(node->input_pins[i + offa], ptr, i);
    }

    for (i = 0; i < b; i++) {
        if (node_b)
            add_input_pin_to_node(ptr, copy_input_npin(node_b->input_pins[i + node_b->input_port_sizes[0]]), i + a);
        else
            remap_pin_to_new_node(node->input_pins[i + node->input_port_sizes[0] + offb], ptr, i + a);
    }

    /* Prep output pins for connecting to cascaded multipliers */
    ptr->num_output_pins = a + b;
    ptr->output_pins = (npin_t**)vtr::malloc(sizeof(void*) * (a + b));
    for (i = 0; i < a + b; i++)
        ptr->output_pins[i] = NULL;

    return;
}

/*-------------------------------------------------------------------------
 * (function: init_multiplier_adder)
 *
 * This function is used to initialize an adder that is within
 * a split multiplier or a multiplier addition tree.
 *-----------------------------------------------------------------------*/
void init_multiplier_adder(nnode_t* node, nnode_t* parent, int a, int b) {
    int i, size;

    node->type = ADD;
    node->related_ast_node = parent->related_ast_node;
    node->traverse_visited = parent->traverse_visited;
    node->node_data = NULL;

    /* Set size to be the maximum input size */
    size = a;
    size = (size < b) ? b : size;

    /* Set new port sizes and parameters */
    node->num_input_port_sizes = 2;
    node->input_port_sizes = (int*)vtr::malloc(2 * sizeof(int));
    node->input_port_sizes[0] = a;
    node->input_port_sizes[1] = b;
    node->num_output_port_sizes = 1;
    node->output_port_sizes = (int*)vtr::malloc(sizeof(int));
    node->output_port_sizes[0] = size;

    /* Set the number of input pins and clear pin entries */
    node->num_input_pins = a + b;
    node->input_pins = (npin_t**)vtr::malloc(sizeof(void*) * (a + b));
    for (i = 0; i < a + b; i++)
        node->input_pins[i] = NULL;

    /* Set the number of output pins and clear pin entries */
    node->num_output_pins = size;
    node->output_pins = (npin_t**)vtr::malloc(sizeof(void*) * size);
    for (i = 0; i < size; i++)
        node->output_pins[i] = NULL;

    add_list = insert_in_vptr_list(add_list, node);
    return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier)
 *
 * This function works to split a multiplier into several smaller
 *  multipliers to better "fit" with the available resources in a
 *  targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a1a0 * b1b0 => a0 * b0 + a0 * b1 + a1 * b0 + a1 * b1 => c1c0 => c
 *
 * If we "balance" the additions, we can actually remove one of the
 * addition operations since we know that a0 * b0 and a1 * b1 will
 * not overlap in bits. This allows us to skip the addition between
 * these two terms and simply concat the results together. Giving us
 * the resulting logic:
 *
 * ((a1 * b1) . (a0 * b0)) + ((a0 * b1) + (a1 * b0)) ==> Result
 *
 * Note that for some of the additions we need to perform sign extensions,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier(nnode_t* node, int a0, int b0, int a1, int b1, netlist_t* netlist) {
    nnode_t *a0b0, *a0b1, *a1b0, *a1b1, *addsmall, *addbig;
    int i, size;

    /* Check for a legitimate split */
    oassert(node->input_port_sizes[0] == (a0 + a1));
    oassert(node->input_port_sizes[1] == (b0 + b1));

    /* New node for small multiply */
    a0b0 = allocate_nnode(node->loc);
    a0b0->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a0b0->name, node->name);
    strcat(a0b0->name, "-0");
    init_split_multiplier(node, a0b0, 0, a0, 0, b0, nullptr, nullptr);
    mult_list = insert_in_vptr_list(mult_list, a0b0);

    /* New node for big multiply */
    a1b1 = allocate_nnode(node->loc);
    a1b1->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a1b1->name, node->name);
    strcat(a1b1->name, "-3");
    init_split_multiplier(node, a1b1, a0, a1, b0, b1, nullptr, nullptr);
    mult_list = insert_in_vptr_list(mult_list, a1b1);

    /* New node for 2nd multiply */
    a0b1 = allocate_nnode(node->loc);
    a0b1->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a0b1->name, node->name);
    strcat(a0b1->name, "-1");
    init_split_multiplier(node, a0b1, 0, a0, b0, b1, a0b0, a1b1);
    mult_list = insert_in_vptr_list(mult_list, a0b1);

    /* New node for 3rd multiply */
    a1b0 = allocate_nnode(node->loc);
    a1b0->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a1b0->name, node->name);
    strcat(a1b0->name, "-2");
    init_split_multiplier(node, a1b0, a0, a1, 0, b0, a1b1, a0b0);
    mult_list = insert_in_vptr_list(mult_list, a1b0);

    /* New node for the initial add */
    addsmall = allocate_nnode(node->loc);
    addsmall->name = (char*)vtr::malloc(strlen(node->name) + 6);
    strcpy(addsmall->name, node->name);
    strcat(addsmall->name, "-add0");
    // this addition will have a carry out in the worst case, add to input pins and connect then to gnd
    init_multiplier_adder(addsmall, a1b0, a1b0->num_output_pins + 1, a0b1->num_output_pins + 1);

    /* New node for the BIG add */
    addbig = allocate_nnode(node->loc);
    addbig->name = (char*)vtr::malloc(strlen(node->name) + 6);
    strcpy(addbig->name, node->name);
    strcat(addbig->name, "-add1");
    init_multiplier_adder(addbig, addsmall, addsmall->num_output_pins, a0b0->num_output_pins - b0 + a1b1->num_output_pins);

    // connect inputs to port a of addsmall
    for (i = 0; i < a1b0->num_output_pins; i++)
        connect_nodes(a1b0, i, addsmall, i);
    add_input_pin_to_node(addsmall, get_zero_pin(netlist), a1b0->num_output_pins);
    // connect inputs to port b of addsmall
    for (i = 0; i < a0b1->num_output_pins; i++)
        connect_nodes(a0b1, i, addsmall, i + addsmall->input_port_sizes[0]);
    add_input_pin_to_node(addsmall, get_zero_pin(netlist), a0b1->num_output_pins + addsmall->input_port_sizes[0]);

    // connect inputs to port a of addbig
    size = addsmall->num_output_pins;
    for (i = 0; i < size; i++)
        connect_nodes(addsmall, i, addbig, i);

    // connect inputs to port b of addbig
    for (i = b0; i < a0b0->output_port_sizes[0]; i++)
        connect_nodes(a0b0, i, addbig, i - b0 + size);
    size = size + a0b0->output_port_sizes[0] - b0;
    for (i = 0; i < a1b1->output_port_sizes[0]; i++)
        connect_nodes(a1b1, i, addbig, i + size);

    // remap the multiplier outputs coming directly from a0b0
    for (i = 0; i < b0; i++) {
        remap_pin_to_new_node(node->output_pins[i], a0b0, i);
    }

    // remap the multiplier outputs coming from addbig
    for (i = 0; i < addbig->num_output_pins; i++) {
        remap_pin_to_new_node(node->output_pins[i + b0], addbig, i);
    }

    // CLEAN UP
    free_nnode(node);

    return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier_a)
 *
 * This function works to split the "a" input of a multiplier into
 *  several smaller multipliers to better "fit" with the available
 *  resources in a targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a1a0 * b => a0 * b + a1 * b => c
 *
 * Note that for the addition we need to perform sign extension,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier_a(nnode_t* node, int a0, int a1, int b) {
    nnode_t *a0b, *a1b, *addsmall;
    int i;

    /* Check for a legitimate split */
    oassert(node->input_port_sizes[0] == (a0 + a1));
    oassert(node->input_port_sizes[1] == b);

    /* New node for a0b multiply */
    a0b = allocate_nnode(node->loc);
    a0b->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a0b->name, node->name);
    strcat(a0b->name, "-0");
    init_split_multiplier(node, a0b, 0, a0, 0, b, nullptr, nullptr);
    mult_list = insert_in_vptr_list(mult_list, a0b);

    /* New node for a1b multiply */
    a1b = allocate_nnode(node->loc);
    a1b->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(a1b->name, node->name);
    strcat(a1b->name, "-1");
    init_split_multiplier(node, a1b, a0, a1, 0, b, nullptr, a0b);
    mult_list = insert_in_vptr_list(mult_list, a1b);

    /* New node for the add */
    addsmall = allocate_nnode(node->loc);
    addsmall->name = (char*)vtr::malloc(strlen(node->name) + 6);
    strcpy(addsmall->name, node->name);
    strcat(addsmall->name, "-add0");
    init_multiplier_adder(addsmall, a0b, b, a1b->num_output_pins);

    /* Connect pins for addsmall */
    for (i = a0; i < a0b->num_output_pins; i++)
        connect_nodes(a0b, i, addsmall, i - a0);
    for (i = 0; i < a1b->num_output_pins; i++)
        connect_nodes(a1b, i, addsmall, i + addsmall->input_port_sizes[0]);

    /* Move original output pins for multiply to new outputs */
    for (i = 0; i < a0; i++)
        remap_pin_to_new_node(node->output_pins[i], a0b, i);

    for (i = 0; i < addsmall->num_output_pins; i++)
        remap_pin_to_new_node(node->output_pins[i + a0], addsmall, i);

    // CLEAN UP
    free_nnode(node);

    return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier_b)
 *
 * This function works to split the "b" input of a multiplier into
 *  several smaller multipliers to better "fit" with the available
 *  resources in a targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a * b1b0 => a * b1 + a * b0 => c
 *
 * Note that for the addition we need to perform sign extension,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier_b(nnode_t* node, int a, int b1, int b0) {
    nnode_t *ab0, *ab1, *addsmall;
    int i;

    /* Check for a legitimate split */
    oassert(node->input_port_sizes[0] == a);
    oassert(node->input_port_sizes[1] == (b0 + b1));

    /* New node for ab0 multiply */
    ab0 = allocate_nnode(node->loc);
    ab0->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(ab0->name, node->name);
    strcat(ab0->name, "-0");
    init_split_multiplier(node, ab0, 0, a, 0, b0, nullptr, nullptr);
    mult_list = insert_in_vptr_list(mult_list, ab0);

    /* New node for ab1 multiply */
    ab1 = allocate_nnode(node->loc);
    ab1->name = (char*)vtr::malloc(strlen(node->name) + 3);
    strcpy(ab1->name, node->name);
    strcat(ab1->name, "-1");
    init_split_multiplier(node, ab1, 0, a, b0, b1, ab0, nullptr);
    mult_list = insert_in_vptr_list(mult_list, ab1);

    /* New node for the add */
    addsmall = allocate_nnode(node->loc);
    addsmall->name = (char*)vtr::malloc(strlen(node->name) + 6);
    strcpy(addsmall->name, node->name);
    strcat(addsmall->name, "-add0");
    init_multiplier_adder(addsmall, ab1, ab1->num_output_pins, a + b1);

    /* Connect pins for addsmall */
    for (i = b0; i < ab0->output_port_sizes[0]; i++)
        connect_nodes(ab0, i, addsmall, i - b0);
    for (i = ab0->output_port_sizes[0] - b0; i < a + b1; i++) /* Sign extend */
        connect_nodes(ab0, ab0->output_port_sizes[0] - 1, addsmall, i);
    for (i = b1 + a; i < (2 * (a + b1)); i++)
        connect_nodes(ab1, i - (b1 + a), addsmall, i);

    /* Move original output pins for multiply to new outputs */
    for (i = 0; i < b0; i++)
        remap_pin_to_new_node(node->output_pins[i], ab0, i);

    for (i = b0; i < node->num_output_pins; i++)
        remap_pin_to_new_node(node->output_pins[i], addsmall, i - b0);

    // CLEAN UP
    free_nnode(node);

    return;
}

/*-------------------------------------------------------------------------
 * (function: pad_multiplier)
 *
 * Fill out a multiplier to a fixed size. Size is retrieved from global
 *	hard_multipliers data.
 *
 * NOTE: The inputs are extended based on multiplier padding setting.
 *-----------------------------------------------------------------------*/
void pad_multiplier(nnode_t* node, netlist_t* netlist) {
    int diffa, diffb, diffout, i;
    int sizea, sizeb, sizeout;
    int ina, inb;

    int testa, testb;

    static int pad_pin_number = 0;

    oassert(node->type == MULTIPLY);
    oassert(hard_multipliers != NULL);

    sizea = node->input_port_sizes[0];
    sizeb = node->input_port_sizes[1];
    sizeout = node->output_port_sizes[0];
    record_mult_distribution(node);

    /* Calculate the BEST fit hard multiplier to use */
    ina = hard_multipliers->inputs->size;
    inb = hard_multipliers->inputs->next->size;
    if (ina < inb) {
        ina = hard_multipliers->inputs->next->size;
        inb = hard_multipliers->inputs->size;
    }
    diffa = ina - sizea;
    diffb = inb - sizeb;
    diffout = hard_multipliers->outputs->size - sizeout;

    if (configuration.split_hard_multiplier == 1) {
        t_linked_vptr* plist = hard_multipliers->pb_types;
        while ((diffa + diffb) && plist) {
            t_pb_type* physical = (t_pb_type*)(plist->data_vptr);
            plist = plist->next;
            testa = physical->ports[0].num_pins;
            testb = physical->ports[1].num_pins;
            if ((testa >= sizea) && (testb >= sizeb) && ((testa - sizea + testb - sizeb) < (diffa + diffb))) {
                diffa = testa - sizea;
                diffb = testb - sizeb;
                diffout = physical->ports[2].num_pins - sizeout;
            }
        }
    }

    /* Expand the inputs */
    if ((diffa != 0) || (diffb != 0)) {
        allocate_more_input_pins(node, diffa + diffb);

        /* Shift pins for expansion of first input pins */
        if (diffa != 0) {
            for (i = 1; i <= sizeb; i++) {
                move_input_pin(node, sizea + sizeb - i, node->num_input_pins - diffb - i);
            }

            /* Connect unused first input pins to zero/pad pin */
            for (i = 0; i < diffa; i++) {
                if (configuration.mult_padding == 0)
                    add_input_pin_to_node(node, get_zero_pin(netlist), i + sizea);
                else
                    add_input_pin_to_node(node, get_pad_pin(netlist), i + sizea);
            }

            node->input_port_sizes[0] = sizea + diffa;
        }

        if (diffb != 0) {
            /* Connect unused second input pins to zero/pad pin */
            for (i = 1; i <= diffb; i++) {
                if (configuration.mult_padding == 0)
                    add_input_pin_to_node(node, get_zero_pin(netlist), node->num_input_pins - i);
                else
                    add_input_pin_to_node(node, get_pad_pin(netlist), node->num_input_pins - i);
            }

            node->input_port_sizes[1] = sizeb + diffb;
        }
    }

    /* Expand the outputs */
    if (diffout != 0) {
        allocate_more_output_pins(node, diffout);
        for (i = 0; i < diffout; i++) {
            // Add new pins to the higher order spots.
            npin_t* new_pin = allocate_npin();
            // Pad outputs with a unique and descriptive name to avoid collisions.
            new_pin->name = append_string("", "unconnected_multiplier_output~%d", pad_pin_number++);
            add_output_pin_to_node(node, new_pin, i + sizeout);
        }
        node->output_port_sizes[0] = sizeout + diffout;
    }

    return;
}

/*-------------------------------------------------------------------------
 * (function: iterate_multipliers)
 *
 * This function will iterate over all of the multiply operations that
 *	exist in the netlist and perform a splitting so that they can
 *	fit into a basic hard multiplier block that exists on the FPGA.
 *	If the proper option is set, then it will be expanded as well
 *	to just use a fixed size hard multiplier.
 *-----------------------------------------------------------------------*/
void iterate_multipliers(netlist_t* netlist) {
    int sizea, sizeb, swap;
    int mula, mulb;
    int a0, a1, b0, b1;
    nnode_t* node;

    /* Can only perform the optimisation if hard multipliers exist! */
    if (hard_multipliers == NULL)
        return;

    sizea = hard_multipliers->inputs->size;
    sizeb = hard_multipliers->inputs->next->size;
    if (sizea < sizeb) {
        swap = sizea;
        sizea = sizeb;
        sizeb = swap;
    }

    while (mult_list != NULL) {
        node = (nnode_t*)mult_list->data_vptr;
        mult_list = delete_in_vptr_list(mult_list);

        oassert(node != NULL);

        if (node->type == HARD_IP)
            node->type = MULTIPLY;

        oassert(node->type == MULTIPLY);

        mula = node->input_port_sizes[0];
        mulb = node->input_port_sizes[1];
        int mult_size = std::max<int>(mula, mulb);
        if (mula < mulb) {
            swap = sizea;
            sizea = sizeb;
            sizeb = swap;
        }

        /* Do I need to split the multiplier on both inputs? */
        if ((mula > sizea) && (mulb > sizeb)) {
            a0 = sizea;
            a1 = mula - sizea;
            b0 = sizeb;
            b1 = mulb - sizeb;
            split_multiplier(node, a0, b0, a1, b1, netlist);
        } else if (mula > sizea) /* split multiplier on a input? */
        {
            a0 = sizea;
            a1 = mula - sizea;
            split_multiplier_a(node, a0, a1, mulb);
        } else if (mulb > sizeb) /* split multiplier on b input? */
        {
            b1 = sizeb;
            b0 = mulb - sizeb;
            split_multiplier_b(node, mula, b1, b0);
        }
        // if either of the multiplicands is larger than the
        // minimum hard multiplier size, use hard multiplier
        // TODO: implement multipliers where one of the operands is
        // 1 bit wide using soft logic
        else if (mult_size >= min_mult || mula == 1 || mulb == 1) {
            /* Check to ensure IF mult needs to be exact size */
            if (configuration.fixed_hard_multiplier != 0)
                pad_multiplier(node, netlist);

            /* Otherwise, we still want to record the multiplier node for
             * reporting later on (the pad_multiplier function does this for the
             * other case */
            else {
                record_mult_distribution(node);
            }
        } else if (hard_adders) {
            if (configuration.fixed_hard_multiplier != 0) {
                split_soft_multiplier(node, netlist);
            }
        }
    }
    return;
}

/*---------------------------------------------------------------------------
 * (function: split_soft_multiplier)
 *
 * This function splits the input multiplier (node) into partial products (AND gates) and
 * adders, as shown below. The partial products starts with "I", and all the partial products
 * generated are added together by implementing a balanced adder tree to produce the final product
 * Sample 4x4 multiplier to help understand logic:
 *
 * 	    				A3 	A2	A1	A0
 *	    				B3 	B2 	B1 	B0
 *	   	-------------------------------
 *	    				I03	I02 I01 I00
 *	   	+         	I13	I12	I11	I10
 *	    		I23	I22	I21	I20             Level 0
 *  	+	I23	I22	I21 I20
 *      -------------------------------
 *  		    C4	C3	C2	C1  C0
 * 	+	D4  D3	D2	D1  D0	I20             Level 1
 *  	-------------------------------
 *  	E5	E4  E3  E2	E1	E0	C0  I00     Level 2
 *
 *-------------------------------------------------------------------------*/
void split_soft_multiplier(nnode_t* node, netlist_t* netlist) {
    oassert(node->num_output_pins > 0);
    oassert(node->num_input_pins > 0);
    oassert(node->num_input_port_sizes == 2);
    oassert(node->num_output_port_sizes == 1);

    size_t multiplier_width = static_cast<size_t>(node->input_port_sizes[0]);
    size_t multiplicand_width = static_cast<size_t>(node->input_port_sizes[1]);

    // ODIN II doesn't work with multiplicand sizes of 1 since it assumes that the
    // output of the multiplier is still the sum of the operands sizes. However, it
    // should only be equal to the long operand since its an AND operation in this case.
    // If this is fixed, this assert statement should be removed and the code will work properly
    oassert(multiplicand_width > 1);

    // number of adders in a balanced tree of the partial product rows
    const int add_levels = std::ceil(std::log((double)multiplicand_width) / std::log(2.));

    // data structure holding the rows of output pins to be added in each addition stage
    // as well as the shift of each row from the position of the first output
    std::vector<std::vector<AdderTreeRow>> addition_stages(add_levels + 1);
    // 2-D array of adders, indexed by the level of the adder in the tree and the adder id within the level
    std::vector<std::vector<nnode_t*>> adders(add_levels);
    // array holding the adder width at each level in the adder tree
    std::vector<std::vector<size_t>> adder_widths(add_levels);

    // 2-D array of partial products. [0..multiplicand_width][0..multiplier_width]
    std::vector<std::vector<nnode_t*>> partial_products(multiplicand_width);

    addition_stages[0].resize(multiplicand_width);
    // initialize all the AND gates needed for the partial products
    for (size_t i = 0; i < multiplicand_width; i++) {
        std::vector<std::pair<nnode_t*, int>> pp_bits(multiplier_width);
        // resize the ith row of the partial products
        partial_products[i].resize(multiplier_width);
        for (size_t j = 0; j < multiplier_width; j++) {
            // create each one of the partial products
            partial_products[i][j] = make_1port_logic_gate(LOGICAL_AND, 2, node, node->traverse_visited);
            pp_bits[j] = {partial_products[i][j], 0};
        }
        // add the partial product rows the addition stages data structure
        addition_stages[0][i] = {i, pp_bits};
    }

    // generate the connections to the AND gates that generates the partial products of the multiplication
    for (size_t i = 0; i < multiplicand_width; i++) {
        for (size_t j = 0; j < multiplier_width; j++) {
            // hookup the multiplier bits to the AND gates
            if (i == 0) {
                // when connecting the input to an AND gate for the first time, remap the input
                remap_pin_to_new_node(node->input_pins[j], partial_products[i][j], 1);
            } else {
                // this input was remapped before, copy from the AND gate input instead
                add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[0][j]->input_pins[1]), 1);
            }
            // hookup the input multiplicand bits the AND gates
            if (j == 0) {
                // when connecting the input to an AND gate for the first time, remap the input
                remap_pin_to_new_node(node->input_pins[i + node->input_port_sizes[0]], partial_products[i][j], 0);
            } else {
                // this input was remapped before, copy from the AND gate input instead
                add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[i][0]->input_pins[0]), 0);
            }
        }
    }

    // iterate over all the levels of addition
    for (size_t level = 0; level < adders.size(); level++) {
        // the number of rows in the next stage is the ceiling of number of rows in this stage divided by 2
        addition_stages[level + 1].resize(std::ceil(addition_stages[level].size() / 2.));
        // the number of adders in this stage is the integer division of the number of rows in this stage
        adder_widths[level].resize(addition_stages[level].size() / 2);
        adders[level].resize(addition_stages[level].size() / 2);

        // iterate over every two rows
        for (size_t row = 0; row < addition_stages[level].size() - 1; row += 2) {
            auto& first_row = addition_stages[level][row];
            auto& second_row = addition_stages[level][row + 1];
            long shift_difference = second_row.shift - first_row.shift;
            auto add_id = row / 2;

            // get the widths of the adder, by finding the larger operand size
            adder_widths[level][add_id] = std::max<size_t>(first_row.bits.size() - shift_difference, second_row.bits.size());
            // first level of addition has a carry out that needs to be generated, so increase adder size by 1
            if (level == 0) adder_widths[level][add_id]++;
            // add one bit for carry out if that last bit of the addition is fed by both levels
            // (was found to be the only case were a carry out will be needed in this multiplier adder tree)
            if (first_row.bits.size() - shift_difference == second_row.bits.size()) adder_widths[level][add_id]++;

            // initialize this adder
            adders[level][add_id] = allocate_nnode(node->loc);
            init_multiplier_adder(adders[level][add_id], node, adder_widths[level][add_id], adder_widths[level][add_id]);
            adders[level][add_id]->name = node_name(adders[level][add_id], node->name);

            // initialize the output of this adder in the next stage
            addition_stages[level + 1][add_id].shift = first_row.shift;
            addition_stages[level + 1][add_id].bits.resize(shift_difference + adder_widths[level][add_id]);
            // copy the bits that weren't fed to adders in the previous stage
            for (size_t i = 0; (long)i < shift_difference; i++) {
                addition_stages[level + 1][add_id].bits[i] = first_row.bits[i];
            }
            // copy adder output bits to their row in next stage
            for (size_t i = 0; i < adder_widths[level][add_id]; i++) {
                addition_stages[level + 1][add_id].bits[i + shift_difference] = {adders[level][add_id], i};
            }

            // connect the bits in the rows to the adder inputs.
            for (size_t bit = 0; bit < adder_widths[level][add_id]; bit++) {
                // input port a of the adder
                if (bit < first_row.bits.size() - shift_difference) {
                    auto bit_a = first_row.bits[bit + shift_difference];
                    connect_nodes(bit_a.first, bit_a.second, adders[level][add_id], bit);
                } else {
                    // connect additional inputs to gnd
                    add_input_pin_to_node(adders[level][add_id], get_zero_pin(netlist), bit);
                }
                // input port b of the adder
                if (bit < second_row.bits.size()) {
                    connect_nodes(second_row.bits[bit].first, second_row.bits[bit].second, adders[level][add_id], bit + adder_widths[level][add_id]);
                } else {
                    // connect additional inputs to gnd
                    add_input_pin_to_node(adders[level][add_id], get_zero_pin(netlist), bit + adder_widths[level][add_id]);
                }
            }
        }

        // if this level have odd number of rows copy the last row to the next level to be added later
        if (addition_stages[level].size() % 2 == 1) {
            addition_stages[level + 1].back() = addition_stages[level].back();
        }
    }

    // the size of the last stage of the adder tree should match the output size of the multiplier
    oassert((long)addition_stages[add_levels][0].bits.size() == node->num_output_pins);

    // Remap the outputs of the multiplier
    for (size_t i = 0; i < addition_stages[add_levels][0].bits.size(); i++) {
        auto output_bit = addition_stages[add_levels][0].bits[i];
        remap_pin_to_new_node(node->output_pins[i], output_bit.first, output_bit.second);
    }

    // check that all connections and input/output remapping is done right
    // meaning all the inputs and outputs of the multiplier that is splitted are nullptrs
    // and all inputs and outputs of the AND gates and adders are not nullptrs

    // check that all the inputs/outputs of the multiplier are remapped
    for (long i = 0; i < node->num_input_pins; i++) {
        oassert(!node->input_pins[i]);
    }
    for (long i = 0; i < node->num_output_pins; i++) {
        oassert(!node->output_pins[i]);
    }

    // check that all the partial product gates have nets connected to their inputs/outputs
    for (size_t ilevel = 0; ilevel < partial_products.size(); ilevel++) {
        for (size_t depth = 0; depth < partial_products[ilevel].size(); depth++) {
            for (int i = 0; i < partial_products[ilevel][depth]->num_input_pins; i++) {
                oassert(partial_products[ilevel][depth]->input_pins[i]);
            }
            for (int i = 0; i < partial_products[ilevel][depth]->num_output_pins; i++) {
                oassert(partial_products[ilevel][depth]->output_pins[i]);
            }
        }
    }

    // check that all adders have nets connected to their inputs/outputs
    for (size_t ilevel = 0; ilevel < adders.size(); ilevel++) {
        for (size_t iadd = 0; iadd < adders[ilevel].size(); iadd++) {
            for (int i = 0; i < adders[ilevel][iadd]->num_input_pins; i++) {
                oassert(adders[ilevel][iadd]->input_pins[i]);
            }
            for (int i = 0; i < adders[ilevel][iadd]->num_output_pins; i++) {
                oassert(adders[ilevel][iadd]->output_pins[i]);
            }
        }
    }

    // CLEAN UP
    cleanup_mult_old_node(node, netlist);
}

/**
 * --------------------------------------------------------------------------
 * (function: is_constant_multipication)
 * 
 * @brief checking multipication ports to specify whether it 
 * is a constant multipication or not 
 * 
 * @param node pointer to the multipication netlist node
 * 
 * @return multipication ports status
 * -------------------------------------------------------------------------*/
mult_port_stat_e is_constant_multipication(nnode_t* node, netlist_t* netlist) {
    int i;
    mult_port_stat_e is_const = mult_port_stat_e::mult_port_stat_END;

    /**
     * Multiply ports
     * IN1: (n bits)        input_port[0]
     * IN2: (m bits)        input_port[1]
     * OUT: min(m, n) bits  output_port[0]
     */

    int IN1_width = node->input_port_sizes[0];
    int IN2_width = node->input_port_sizes[1];

    bool multiplier_const = true;
    /* going through the IN1 port */
    for (i = 0; i < IN1_width; i++) {
        /* corresponding pin of the port */
        npin_t* pin = node->input_pins[i];
        /* atleast equal to VCC or GND */
        if (!strcmp(pin->net->name, netlist->zero_net->name) || !strcmp(pin->net->name, netlist->one_net->name))
            continue;
        else {
            multiplier_const = false;
            break;
        }
    }

    bool multiplicand_const = true;
    /* going through the IN1 port */
    for (i = 0; i < IN2_width; i++) {
        /* corresponding pin of the port */
        npin_t* pin = node->input_pins[IN1_width + i];
        /* atleast equal to VCC or GND */
        if (!strcmp(pin->net->name, netlist->zero_net->name) || !strcmp(pin->net->name, netlist->one_net->name))
            continue;
        else {
            multiplicand_const = false;
            break;
        }
    }

    if (multiplier_const && multiplicand_const)
        is_const = mult_port_stat_e::CONSTANT;
    else if (multiplier_const)
        is_const = mult_port_stat_e::MULTIPLIER_CONSTANT;
    else if (multiplicand_const)
        is_const = mult_port_stat_e::MULTIPICAND_CONSTANT;
    else
        is_const = mult_port_stat_e::NOT_CONSTANT;

    return (is_const);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: check_constant_multipication )
 * 
 * @brief checking for constant multipication. If one port is constant,
 * the multipication node will explode into multiple adders 
 * 
 * @param node pointing to the mul node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
bool check_constant_multipication(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /* to calculate return value */
    mult_port_stat_e is_const;

    /* checking multipication ports to specify whether it is constant or not */
    if ((is_const = is_constant_multipication(node, netlist)) != mult_port_stat_e::NOT_CONSTANT) {
        /* performaing optimization on the constant multiplication ports */
        node = perform_const_mult_optimization(is_const, node, traverse_mark_number, netlist);
        /* implementation of constant multipication which is actually cascading adders */
        signal_list_t* output_signals = implement_constant_multipication(node, is_const, static_cast<short>(traverse_mark_number), netlist);

        /* connecting the output pins */
        connect_constant_mult_outputs(node, output_signals);
    }

    return (is_const != mult_port_stat_e::NOT_CONSTANT);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: perform_const_mult_optimization )
 * 
 * @brief checking for constant multipication constant port size. 
 * if possible the extra unneccessary pins of constant port will 
 * be reduced
 * 
 * @param node pointing to the mul node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static nnode_t* perform_const_mult_optimization(mult_port_stat_e mult_port_stat, nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    int i;
    /* constatnt and variable port of the given multipication */
    signal_list_t* const_port = init_signal_list();
    signal_list_t* var_port = init_signal_list();
    operation_list const_signedness = UNSIGNED;
    operation_list var_signedness = UNSIGNED;

    /* initialize const and var port signals */
    if (mult_port_stat == mult_port_stat_e::MULTIPICAND_CONSTANT) {
        /* adding var port pins to signal list */
        for (i = 0; i < node->input_port_sizes[0]; i++) {
            add_pin_to_signal_list(var_port, node->input_pins[i]);
        }
        var_signedness = node->attributes->port_a_signed;
        /* adding const port pins to signal list */
        for (i = node->input_port_sizes[0]; i < node->num_input_pins; i++) {
            add_pin_to_signal_list(const_port, node->input_pins[i]);
        }
        const_signedness = node->attributes->port_b_signed;
    } else if (mult_port_stat == mult_port_stat_e::MULTIPLIER_CONSTANT) {
        /* adding var port pins to signal list */
        for (i = 0; i < node->input_port_sizes[0]; i++) {
            add_pin_to_signal_list(const_port, node->input_pins[i]);
        }
        const_signedness = node->attributes->port_a_signed;
        /* adding const port pins to signal list */
        for (i = node->input_port_sizes[0]; i < node->num_input_pins; i++) {
            add_pin_to_signal_list(var_port, node->input_pins[i]);
        }
        var_signedness = node->attributes->port_b_signed;
    }

    int idx = -1;
    signal_list_t* new_const_port = init_signal_list();
    /* iterating over const port to determine useless ports */
    for (i = const_port->count; i > 0; i--) {
        npin_t* pin = const_port->pins[i - 1];
        /* starting from the end and prune pins connected to GND */
        if (!strcmp(pin->net->name, netlist->one_net->name)) {
            idx = i;
            break;
        } else {
            /* detach from the old mult node and free pin */
            delete_npin(pin);
        }
    }
    /* initializing new const port */
    for (i = 0; i < idx; i++) {
        npin_t* pin = const_port->pins[i];
        add_pin_to_signal_list(new_const_port, pin);
    }

    signal_list_t* first_port = (mult_port_stat == mult_port_stat_e::MULTIPLIER_CONSTANT) ? new_const_port : var_port;
    signal_list_t* second_port = (mult_port_stat == mult_port_stat_e::MULTIPLIER_CONSTANT) ? var_port : new_const_port;
    /* creating new mult node */
    int offset = 0;
    nnode_t* new_node = make_2port_gate(node->type, first_port->count, second_port->count, node->num_output_pins, node, traverse_mark_number);
    /* copy attributes */
    if (mult_port_stat == mult_port_stat_e::MULTIPLIER_CONSTANT) {
        new_node->attributes->port_a_signed = const_signedness;
        new_node->attributes->port_b_signed = var_signedness;
    } else {
        new_node->attributes->port_a_signed = var_signedness;
        new_node->attributes->port_b_signed = const_signedness;
    }
    /* adding first port */
    for (i = 0; i < first_port->count; i++) {
        remap_pin_to_new_node(first_port->pins[i], new_node, offset + i);
    }
    offset += first_port->count;
    /* adding second port */
    for (i = 0; i < second_port->count; i++) {
        remap_pin_to_new_node(second_port->pins[i], new_node, offset + i);
    }
    /* remap output ports */
    for (i = 0; i < node->num_output_pins; i++) {
        remap_pin_to_new_node(node->output_pins[i], new_node, i);
    }

    // CLEAN UP
    free_signal_list(const_port);
    free_signal_list(var_port);
    free_signal_list(new_const_port);
    free_nnode(node);
    node = NULL;

    return (new_node);
}

bool is_ast_multiplier(ast_node_t* node) {
    bool is_mult;
    ast_node_t* instance = node->children[0];
    is_mult = (!strcmp(node->children[0]->types.identifier, "multiply"))
              && (instance->children[0]->num_children == 3);

    ast_node_t* connect_list = instance->children[0];
    if (is_mult && connect_list->children[0]->identifier_node) {
        /* port connections were passed by name; verify port names */
        for (int i = 0; i < connect_list->num_children && is_mult; i++) {
            char* id = connect_list->children[i]->children[0]->types.identifier;

            if ((strcmp(id, "a") != 0) && (strcmp(id, "b") != 0) && (strcmp(id, "out") != 0)) {
                is_mult = false;
                break;
            }
        }
    }

    return is_mult;
}
/**
 * -------------------------------------------------------------------------
 * (function: check_multiplier_port_size)
 *
 * If output size is less than the sum of input sizes, 
 * we need to expand output pins with pad pins
 * 
 * @param node pointer to the multiplication node
 * -----------------------------------------------------------------------
 */
void check_multiplier_port_size(nnode_t* node) {
    /* Can only perform the optimisation if hard multipliers exist! */
    if (hard_multipliers == NULL)
        return;

    int mula = node->input_port_sizes[0];
    int mulb = node->input_port_sizes[1];
    int sizeout = node->num_output_pins;
    int limit = mula + mulb;

    /* check the output port size */
    if (node->num_output_pins < limit) {
        // Set the limit value as the number of output pins
        node->num_output_pins = limit;
        node->output_port_sizes[0] = limit;
        // Keep record of old output pins pointer for cleaning up later
        npin_t** old_output_pins = node->output_pins;
        node->output_pins = (npin_t**)calloc(node->num_output_pins, sizeof(npin_t*));

        // Move output pins to new array and adding pad pins in extra spots
        for (int i = 0; i < node->num_output_pins; i++) {
            if (i < sizeout)
                node->output_pins[i] = old_output_pins[i];
            else {
                npin_t* new_pin = allocate_npin();
                new_pin->name = append_string("", "%s~dummy_output~%d", node->name, 0);
                nnet_t* new_net = allocate_nnet();

                // hook the output pin into the node
                add_output_pin_to_node(node, new_pin, i);
                // hook up new pin 1 into the new net
                add_driver_pin_to_net(new_net, new_pin);
            }
        }
        // CLEAN UP
        vtr::free(old_output_pins);
    }
}
/*-------------------------------------------------------------------------
 * (function: clean_multipliers)
 *
 * Clean up the memory by deleting the list structure of multipliers
 *	during optimization
 *-----------------------------------------------------------------------*/
void clean_multipliers() {
    while (mult_list != NULL)
        mult_list = delete_in_vptr_list(mult_list);
    return;
}

/**
 * -------------------------------------------------------------------------
 * (function: cleanup_mult_old_node)
 *
 * @brief <clean up nodeo, a high level MULT node> 
 * In split_soft_multplier function, nodeo is splitted to small multipliers, 
 * while because of the complexity of input pin connections they have not been 
 * remapped to new nodes, they just copied and added to new nodes. This function 
 * will detach input pins from the nodeo. Moreover, it will connect the net of 
 * unconnected output signals to the GND node, detach the pin from nodeo and 
 * free the output pins to avoid memory leak.
 * 
 * @param nodeo representing the old adder node
 * @param netlist representing the current netlist
 *-----------------------------------------------------------------------*/
static void cleanup_mult_old_node(nnode_t* nodeo, netlist_t* netlist) {
    int i;
    /* Disconnecting input pins from the old node side */
    for (i = 0; i < nodeo->num_input_pins; i++) {
        nodeo->input_pins[i] = NULL;
    }

    /* connecting the extra output pins to the gnd node */
    for (i = 0; i < nodeo->num_output_pins; i++) {
        npin_t* output_pin = nodeo->output_pins[i];

        if (output_pin && output_pin->node) {
            /* for now we just pass the signals directly through */
            npin_t* zero_pin = get_zero_pin(netlist);
            int idx_2_buffer = zero_pin->pin_net_idx;

            // Dont eliminate the buffer if there are multiple drivers or the AST included it
            if (output_pin->net->num_driver_pins <= 1) {
                /* join all fanouts of the output net with the input pins net */
                join_nets(zero_pin->net, output_pin->net);

                /* erase the pointer to this buffer */
                zero_pin->net->fanout_pins[idx_2_buffer] = NULL;
            }

            free_npin(zero_pin);
            free_npin(output_pin);

            /* Disconnecting output pins from the old node side */
            nodeo->output_pins[i] = NULL;
        }
    }

    // CLEAN UP
    free_nnode(nodeo);
}

void free_multipliers() {
    if (hard_multipliers && hard_multipliers->instances) {
        t_multiplier* tmp = (t_multiplier*)hard_multipliers->instances;

        while (tmp != NULL) {
            t_multiplier* tmp2 = tmp->next;
            vtr::free(tmp);
            tmp = tmp2;
        }

        hard_multipliers->instances = NULL;
    }
}
