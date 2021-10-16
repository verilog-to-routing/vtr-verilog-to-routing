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
#include "node_creation_library.h"
#include "vtr_util.h"
#include "vtr_memory.h"

long unique_node_name_id = 0;

/*-----------------------------------------------------------------------
 * (function: get_a_pad_pin)
 * 	this allows us to attach to the constant netlist driving hb_pad
 *---------------------------------------------------------------------*/
npin_t* get_pad_pin(netlist_t* netlist) {
    npin_t* pad_fanout_pin = allocate_npin();
    pad_fanout_pin->name = vtr::strdup(pad_string);
    add_fanout_pin_to_net(netlist->pad_net, pad_fanout_pin);
    return pad_fanout_pin;
}

/*-----------------------------------------------------------------------
 * (function: get_a_zero_pin)
 * 	this allows us to attach to the constant netlist driving zero
 *---------------------------------------------------------------------*/
npin_t* get_zero_pin(netlist_t* netlist) {
    npin_t* zero_fanout_pin = allocate_npin();
    zero_fanout_pin->name = vtr::strdup(zero_string);
    add_fanout_pin_to_net(netlist->zero_net, zero_fanout_pin);
    return zero_fanout_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_a_one_pin)
 * 	this allows us to attach to the constant netlist driving one
 *-------------------------------------------------------------------------------------------*/
npin_t* get_one_pin(netlist_t* netlist) {
    npin_t* one_fanout_pin = allocate_npin();
    one_fanout_pin->name = vtr::strdup(one_string);
    add_fanout_pin_to_net(netlist->one_net, one_fanout_pin);
    return one_fanout_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_not_gate_with_input)
 * 	Creates a not gate and attaches it to the inputs
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_not_gate_with_input(npin_t* input_pin, nnode_t* node, short mark) {
    nnode_t* logic_node;

    logic_node = make_not_gate(node, mark);

    /* add the input ports as needed */
    add_input_pin_to_node(logic_node, input_pin, 0);

    return logic_node;
}

/*-------------------------------------------------------------------------
 * (function: make_not_gate)
 * 	Just make a not gate
 *-----------------------------------------------------------------------*/
nnode_t* make_not_gate(nnode_t* node, short mark) {
    nnode_t* logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = LOGICAL_NOT;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    allocate_more_input_pins(logic_node, 1);
    allocate_more_output_pins(logic_node, 1);

    return logic_node;
}

/**
 * -------------------------------------------------------------------------
 * (function: make_not_gate)
 * @brief Making a not gate with the given pin as input 
 * and a new pin allocated as the output
 *
 * @param pin input pin
 * @param node related netlist node
 * @param mark netlist traversal number 
 *
 * @return not node
 *-----------------------------------------------------------------------*/
nnode_t* make_inverter(npin_t* pin, nnode_t* node, short mark) {
    /* validate the input pin */
    oassert(pin->type == INPUT);

    nnode_t* logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = LOGICAL_NOT;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    allocate_more_input_pins(logic_node, 1);
    allocate_more_output_pins(logic_node, 1);

    if (pin->node)
        pin->node->input_pins[pin->pin_node_idx] = NULL;

    /* hook the pin into the not node */
    add_input_pin_to_node(logic_node, pin, 0);

    /* connecting the not_node output pin */
    npin_t* new_pin1 = allocate_npin();
    npin_t* new_pin2 = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, logic_node->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(logic_node, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, new_pin2);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_1port_gate)
 * 	Make a 1 port gate with variable sizes
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_1port_gate(operation_list type, int width_input, int width_output, nnode_t* node, short mark) {
    nnode_t* logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_input);
    add_input_port_information(logic_node, width_input);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: make_1port_logic_gate)
 * 	Make a gate with variable sized inputs and 1 output
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_1port_logic_gate(operation_list type, int width, nnode_t* node, short mark) {
    nnode_t* logic_node = make_1port_gate(type, width, 1, node, mark);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_1port_logic_gate_with_inputs)
 * 	Make a gate with variable sized inputs, 1 output, and connect to the supplied inputs
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_1port_logic_gate_with_inputs(operation_list type, int width, signal_list_t* pin_list, nnode_t* node, short mark) {
    nnode_t* logic_node;
    int i;

    logic_node = make_1port_gate(type, width, 1, node, mark);

    /* hookup all the pins */
    for (i = 0; i < width; i++) {
        add_input_pin_to_node(logic_node, pin_list->pins[i], i);
    }

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_3port_logic_gates)
 * 	Make a 3 port gate all variable port widths.
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t* node, short mark) {
    nnode_t* logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_port1);
    add_input_port_information(logic_node, width_port1);
    allocate_more_input_pins(logic_node, width_port2);
    add_input_port_information(logic_node, width_port2);
    allocate_more_input_pins(logic_node, width_port3);
    add_input_port_information(logic_node, width_port3);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_2port_logic_gates)
 * 	Make a 2 port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t* node, short mark) {
    nnode_t* logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_port1);
    add_input_port_information(logic_node, width_port1);
    allocate_more_input_pins(logic_node, width_port2);
    add_input_port_information(logic_node, width_port2);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_nport_logic_gates)
 * 	Make a n port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t* make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t* node, short mark) {
    int i;
    nnode_t* logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    for (i = 0; i < port_sizes; i++) {
        allocate_more_input_pins(logic_node, width);
        add_input_port_information(logic_node, width);
    }
    //allocate_more_input_pins(logic_node, width_port2);
    //add_input_port_information(logic_node, width_port2);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: make chain)
 * 
 * @brief create a chain of the given TYPE gates for the given pins
 * 
 * @param inputs signal list of pins
 * @param node netlist node that input pins come from it
 * 
 * @return the last output pin of the chain
 * -------------------------------------------------------------------------------------------
 */
signal_list_t* make_chain(operation_list type, signal_list_t* inputs, nnode_t* node) {
    int i;
    int width = inputs->count;

    if (width == 1)
        return (inputs);

    signal_list_t* output = init_signal_list();

    /* detach pins from their node */
    for (i = 0; i < width; i++) {
        npin_t* pin = inputs->pins[i];
        if (pin->node)
            pin->node->input_pins[pin->pin_node_idx] = NULL;
    }

    nnode_t** gates = (nnode_t**)vtr::calloc(width - 1, sizeof(nnode_t*));
    signal_list_t* internal_outputs = init_signal_list();

    /* ending in (width - 1) since we take first two pins in the beginning */
    for (i = 0; i < width - 1; i++) {
        gates[i] = make_2port_gate(type, 1, 1, 1, node, node->traverse_visited);

        if (i == 0) {
            /* taking the first two pins */
            add_input_pin_to_node(gates[i], inputs->pins[0], 0);
            add_input_pin_to_node(gates[i], inputs->pins[1], 1);

        } else {
            /* taking the first two pins */
            add_input_pin_to_node(gates[i], inputs->pins[i + 1], 0);
            add_input_pin_to_node(gates[i], internal_outputs->pins[i - 1], 1);
        }

        // specify the output pin
        npin_t* new_pin1 = allocate_npin();
        npin_t* new_pin2 = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        new_net->name = make_full_ref_name(NULL, NULL, NULL, gates[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(gates[i], new_pin1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* adding to interanl outputs signal list */
        add_pin_to_signal_list(internal_outputs, new_pin2);
    }

    /* the output pin of the last or gate */
    add_pin_to_signal_list(output, internal_outputs->pins[width - 2]);

    // CLEAN UP
    free_signal_list(internal_outputs);
    vtr::free(gates);

    return (output);
}

const char* edge_type_blif_str(edge_type_e edge_type, loc_t loc) {
    switch (edge_type) {
        case FALLING_EDGE_SENSITIVITY:
            return "fe";
        case RISING_EDGE_SENSITIVITY:
            return "re";
        case ACTIVE_HIGH_SENSITIVITY:
            return "ah";
        case ACTIVE_LOW_SENSITIVITY:
            return "al";
        case ASYNCHRONOUS_SENSITIVITY:
            return "as";
        default:
            error_message(NETLIST, loc,
                          "undefined sensitivity kind for flip flop %s", edge_type_e_STR[edge_type]);

            return NULL;
    }
}

edge_type_e edge_type_blif_enum(std::string edge_kind_str, loc_t loc) {
    if (edge_kind_str == "fe")
        return FALLING_EDGE_SENSITIVITY;
    else if (edge_kind_str == "re")
        return RISING_EDGE_SENSITIVITY;
    else if (edge_kind_str == "ah")
        return ACTIVE_HIGH_SENSITIVITY;
    else if (edge_kind_str == "al")
        return ACTIVE_LOW_SENSITIVITY;
    else if (edge_kind_str == "as")
        return ASYNCHRONOUS_SENSITIVITY;
    else {
        error_message(NETLIST, loc,
                      "undefined sensitivity kind for flip flop %s", edge_kind_str.c_str());

        return UNDEFINED_SENSITIVITY;
    }
}

/*----------------------------------------------------------------------------
 * (function: hard_node_name)
 * 	This creates the unique node name for a hard block
 *--------------------------------------------------------------------------*/
char* hard_node_name(nnode_t* /*node*/, char* instance_name_prefix, char* hb_name, char* hb_inst) {
    char* return_node_name;

    /* create the unique name for this node */
    return_node_name = make_full_ref_name(instance_name_prefix, hb_name, hb_inst, NULL, -1);

    unique_node_name_id++;

    return return_node_name;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name)
 * 	This creates the unique node name
 *-------------------------------------------------------------------------------------------*/
char* node_name(nnode_t* node, char* instance_name_prefix) {
    return op_node_name(node->type, instance_name_prefix);
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name)
 * 	This creates the unique node name
 *-------------------------------------------------------------------------------------------*/
char* op_node_name(operation_list op, char* instance_prefix_name) {
    char* return_node_name;

    /* create the unique name for this node */
    return_node_name = make_full_ref_name(instance_prefix_name, NULL, NULL, name_based_on_op(op), unique_node_name_id);

    unique_node_name_id++;

    return return_node_name;
}

/*-------------------------------------------------------------------------
 * (function: make_mult_block)
 * 	Just make a multiplier hard block
 *-----------------------------------------------------------------------*/
nnode_t* make_mult_block(nnode_t* node, short mark) {
    nnode_t* logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = MULTIPLY;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;
    logic_node->input_port_sizes = node->input_port_sizes;
    logic_node->num_input_port_sizes = node->num_input_port_sizes;
    logic_node->output_port_sizes = node->output_port_sizes;
    logic_node->num_output_port_sizes = node->num_output_port_sizes;

    allocate_more_input_pins(logic_node, node->num_input_pins);
    allocate_more_output_pins(logic_node, node->num_output_pins);

    return logic_node;
}

/**
 * (function: make_ff_node)
 * 
 * @brief make a flip-flop, if Q is null it will create a new pin
 * 
 * @param D input
 * @param clk clock
 * @param Q qutput
 * @param node pointing to the related node
 * @param netlist pointing to the related netlist
 * @return smux node
 */
extern nnode_t* make_ff_node(npin_t* D, npin_t* clk, npin_t* Q, nnode_t* node, netlist_t* netlist) {
    /* validate clk */
    oassert(clk->net->num_driver_pins == 1);

    npin_t* FF_input = D;
    /* detach from old node */
    if (FF_input->node)
        FF_input->node->input_pins[FF_input->pin_node_idx] = NULL;
    npin_t* FF_clk = clk;
    if (FF_clk->node) {
        FF_clk->node->input_pins[FF_clk->pin_node_idx] = NULL;
    }

    bool latch = false;
    nnode_t* clock_node = clk->net->driver_pins[0]->node;
    if (clock_node->type != CLOCK_NODE && clock_node->type != INPUT_NODE)
        latch = true;

    /* legalize ff */
    if (configuration.fflegalize) {
        if (clk->sensitivity == FALLING_EDGE_SENSITIVITY) {
            nnode_t* not_node = make_1port_gate(LOGICAL_NOT, 1, 1, node, node->traverse_visited);
            /* hook the input into not node */
            add_input_pin_to_node(not_node, clk, 0);
            /* create output pin */
            signal_list_t* not_outputs = make_output_pins_for_existing_node(not_node, 1);
            FF_clk = not_outputs->pins[0];

            /* create clk node */
            nnode_t* clk_node = make_1port_gate(CLOCK_NODE, 1, 1, node, node->traverse_visited);
            /* hook the input into not node */
            add_input_pin_to_node(clk_node, FF_clk, 0);
            /* create output pin */
            signal_list_t* clk_outputs = make_output_pins_for_existing_node(clk_node, 1);
            FF_clk = clk_outputs->pins[0];

            /* change clk sensitivity */
            FF_clk->sensitivity = RISING_EDGE_SENSITIVITY;
            // CLEAN UP
            free_signal_list(not_outputs);
            free_signal_list(clk_outputs);
        }
    }

    /* create FF node */
    nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, node->traverse_visited);
    /* hook clk into FF node */
    FF_node->attributes->clk_edge_type = FF_clk->sensitivity;
    add_input_pin_to_node(FF_node, FF_clk, 1);
    /* hook D into FF node */
    add_input_pin_to_node(FF_node, FF_input, 0);

    if (Q) {
        /* detach from old node */
        if (Q->node)
            Q->node->output_pins[Q->pin_node_idx] = NULL;
        /* connect Q to FF node */
        add_output_pin_to_node(FF_node, Q, 0);
    } else {
        /* hook the output pin into the new ff_node */
        signal_list_t* FF_outputs = make_output_pins_for_existing_node(FF_node, 1);
        FF_input = FF_outputs->pins[0];
        // CLEAN UP
        free_signal_list(FF_outputs);
    }

    /* adding the new generated node to the ff node list of the netlist */
    if (!latch)
        add_node_to_netlist(netlist, FF_node, FF_NODE);

    return (FF_node);
}

/**
 * (function: smux_with_sel_polarity)
 * 
 * @brief smux pins based on the selector polarity
 * 
 * @param pin1 input mux 1
 * @param pin2 input mux 2
 * @param inputs mux selector
 * @param node pointing to the related node
 * 
 * @return smux node
 */
nnode_t* smux_with_sel_polarity(npin_t* pin1, npin_t* pin2, npin_t* sel, nnode_t* node) {
    /* validation */
    oassert(sel->sensitivity == ACTIVE_HIGH_SENSITIVITY || sel->sensitivity == ACTIVE_LOW_SENSITIVITY);

    int idx1 = 0, idx2 = 0;
    nnode_t* smux = make_2port_gate(SMUX_2, 1, 2, 1, node, node->traverse_visited);

    /* hook selector pin into SMUX_2 */
    if (sel->node)
        remap_pin_to_new_node(sel, smux, 0);
    else
        add_input_pin_to_node(smux, sel, 0);

    /* to check the polarity and connect the correspondence signal */
    if (sel->sensitivity == ACTIVE_HIGH_SENSITIVITY) {
        idx1 = 1;
        idx2 = 2;

    } else if (sel->sensitivity == ACTIVE_LOW_SENSITIVITY) {
        idx1 = 2;
        idx2 = 1;
    }

    /* connecting the first input pin to 0: smux input */
    if (pin1->node)
        remap_pin_to_new_node(pin1, smux, idx1);
    else
        add_input_pin_to_node(smux, pin1, idx1);

    /* connecting the second pin to 1: mux input  */
    if (pin2->node)
        remap_pin_to_new_node(pin2, smux, idx2);
    else
        add_input_pin_to_node(smux, pin2, idx2);

    // specify  output pin
    signal_list_t* outputs = make_output_pins_for_existing_node(smux, 1);

    // CLEAN UP
    free_signal_list(outputs);

    return (smux);
}

/**
 * (function: make_multiport_mux)
 * 
 * @brief make a multiport mux with given selector and signals lists
 * 
 * @param inputs list of input signal lists
 * @param selector signal list of selector pins
 * @param num_muxed_inputs num of inputs to be muxxed
 * @param outs list of outputs signals 
 * @param node pointing to the src node
 * @param netlist pointer to the netlist
 * 
 * @return mux node
 */
nnode_t* make_multiport_smux(signal_list_t** inputs, signal_list_t* selector, int num_muxed_inputs, signal_list_t* outs, nnode_t* node, netlist_t* netlist) {
    /* validation */
    int valid_num_mux_inputs = shift_left_value_with_overflow_check(0X1, selector->count, node->loc);
    oassert(valid_num_mux_inputs >= num_muxed_inputs);

    int i, j;
    int offset = 0;

    nnode_t* mux = allocate_nnode(node->loc);
    mux->type = MULTIPORT_nBIT_SMUX;
    mux->name = node_name(mux, node->name);
    mux->traverse_visited = node->traverse_visited;

    /* add selector signal */
    add_input_port_information(mux, selector->count);
    allocate_more_input_pins(mux, selector->count);
    for (i = 0; i < selector->count; i++) {
        npin_t* sel = selector->pins[i];
        /* hook selector into mux node as first port */
        if (sel->node)
            remap_pin_to_new_node(sel, mux, i);
        else
            add_input_pin_to_node(mux, sel, i);
    }
    offset += selector->count;

    int max_width = 0;
    for (i = 0; i < num_muxed_inputs; i++) {
        /* keep the size of max input to allocate equal output */
        if (inputs[i]->count > max_width)
            max_width = inputs[i]->count;
    }

    for (i = 0; i < num_muxed_inputs; i++) {
        /* add input port data */
        add_input_port_information(mux, max_width);
        allocate_more_input_pins(mux, max_width);

        for (j = 0; j < inputs[i]->count; j++) {
            npin_t* pin = inputs[i]->pins[j];
            /* hook inputs into mux node */
            if (j < max_width) {
                if (pin->node)
                    remap_pin_to_new_node(pin, mux, j + offset);
                else
                    add_input_pin_to_node(mux, pin, j + offset);
            } else {
                /* pad with PAD node */
                add_input_pin_to_node(mux, get_pad_pin(netlist), j + offset);
            }
        }
        /* record offset to have correct idx for adding pins */
        offset += max_width;
    }

    /* add output info */
    add_output_port_information(mux, max_width);
    allocate_more_output_pins(mux, max_width);

    // specify output pin
    if (outs != NULL) {
        for (i = 0; i < outs->count; i++) {
            npin_t* output_pin;
            if (i < max_width) {
                output_pin = outs->pins[i];
                if (output_pin->node)
                    remap_pin_to_new_node(output_pin, mux, i);
                else
                    add_output_pin_to_node(mux, output_pin, i);
            } else {
                output_pin = allocate_npin();
                nnet_t* output_net = allocate_nnet();
                /* add output driver to the output net */
                add_driver_pin_to_net(output_net, output_pin);
                /* add output pin to the mux node */
                add_output_pin_to_node(mux, output_pin, i);
            }
        }
    } else {
        signal_list_t* outputs = make_output_pins_for_existing_node(mux, max_width);
        // CLEAN UP
        free_signal_list(outputs);
    }

    return (mux);
}
