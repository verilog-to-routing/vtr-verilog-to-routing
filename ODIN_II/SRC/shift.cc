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

#include "shift.hh"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: resolve_shift_node)
 * 
 * @brief resolving the shift nodes by making
 * the input port sizes the same
 * 
 * @param node pointing to a shift node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void equalize_shift_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /**
     * (SHIFT ports)
     * INPUTS
     *  A: (width_a)
     *  B: (width_b)
     * OUTPUT
     *  Y: width_y
    */
    int port_a_size = node->input_port_sizes[0];
    int port_b_size = node->input_port_sizes[1];
    int port_y_size = node->output_port_sizes[0];

    /* no change is needed */
    if (port_a_size == port_b_size)
        return;

    /* new port size */
    int max_size = std::max(port_a_size, port_b_size);

    /* creating the new node */
    nnode_t* new_shift_node = make_2port_gate(node->type, max_size, max_size, port_y_size, node, traverse_mark_number);

    int i;
    npin_t* extension_pin = NULL;

    for (i = 0; i < max_size; i++) {
        /* port a needs to be extended */
        if (port_a_size < max_size) {
            /* remapping the a pins + adding extension pin */
            if (i < port_a_size) {
                /* need to remap existing pin */
                remap_pin_to_new_node(node->input_pins[i],
                                      new_shift_node,
                                      i);
            } else {
                /* need to add extension pin */
                extension_pin = (node->attributes->port_a_signed == SIGNED) ? copy_input_npin(new_shift_node->input_pins[port_a_size - 1]) : get_zero_pin(netlist);
                add_input_pin_to_node(new_shift_node,
                                      extension_pin,
                                      i);
            }

            /* remapping the b pins untouched */
            remap_pin_to_new_node(node->input_pins[i + max_size],
                                  new_shift_node,
                                  i + max_size);

        }
        /* port b needs to be extended */
        else if (port_b_size < max_size) {
            /* remapping the a pins untouched */
            remap_pin_to_new_node(node->input_pins[i],
                                  new_shift_node,
                                  i);

            /* remapping the b pins + adding extension pin */
            if (i < port_b_size) {
                /* need to remap existing pin */
                remap_pin_to_new_node(node->input_pins[i + max_size],
                                      new_shift_node,
                                      i + max_size);
            } else {
                /* need to add extension pin */
                extension_pin = (node->attributes->port_b_signed == SIGNED) ? copy_input_npin(new_shift_node->input_pins[port_b_size - 1 + max_size]) : get_zero_pin(netlist);
                add_input_pin_to_node(new_shift_node,
                                      extension_pin,
                                      i + max_size);
            }
        }
    }

    /* Connecting output pins */
    for (i = 0; i < port_y_size; i++) {
        remap_pin_to_new_node(node->output_pins[i],
                              new_shift_node,
                              i);
    }

    // CLEAN UP
    free_nnode(node);
}

/**
 * (function: constant_shift)
 * 
 * @brief performing constant shift operation on given signal_list
 * 
 * @param input_signals input to be shifted 
 * @param shift_size vonstant shift size
 * @param shift_type shift type: SL, SR, ASL or ASR
 * @param assignment_size width of the output
 * @param netlist pointer to the current netlist file
 */
signal_list_t* constant_shift(signal_list_t* input_signals, const int shift_size, const operation_list shift_type, const int assignment_size, netlist_t* netlist) {
    signal_list_t* return_list = init_signal_list();

    /* record the size of the shift */
    int input_width = input_signals->count;
    int output_width = assignment_size;
    // int pad_bit = input_width - 1;

    int i;
    switch (shift_type) {
        case SL:
        case ASL: {
            /* connect ZERO to outputs that don't have inputs connected */
            for (i = 0; i < shift_size; i++) {
                if (i < output_width) {
                    // connect 0 to lower outputs
                    npin_t* zero_pin = allocate_npin();
                    add_fanout_pin_to_net(netlist->zero_net, zero_pin);

                    add_pin_to_signal_list(return_list, zero_pin);
                    zero_pin->node = NULL;
                }
            }

            /* connect inputs to outputs */
            for (i = 0; i < output_width - shift_size; i++) {
                if (i < input_width) {
                    // connect higher output pin to lower input pin
                    add_pin_to_signal_list(return_list, input_signals->pins[i]);
                    input_signals->pins[i]->node = NULL;
                } else {
                    npin_t* extension_pin = get_zero_pin(netlist);

                    add_pin_to_signal_list(return_list, extension_pin);
                    extension_pin->node = NULL;
                }
            }
            break;
        }
        case SR: //fallthrough
        case ASR: {
            for (i = shift_size; i < input_width; i++) {
                // connect higher output pin to lower input pin
                if (i - shift_size < output_width) {
                    add_pin_to_signal_list(return_list, input_signals->pins[i]);
                    input_signals->pins[i]->node = NULL;
                }
            }

            /* Extend pad_bit to outputs that don't have inputs connected */
            for (i = output_width - 1; i >= input_width - shift_size; i--) {
                npin_t* extension_pin = NULL;
                // [TODO]: Extra potential feature, check for signedness
                // if (op->children[0]->types.variable.signedness == SIGNED && operation_node->type == ASR) {
                //     extension_pin = copy_input_npin(input_signals->pins[pad_bit]);
                // } else {
                extension_pin = get_zero_pin(netlist);
                // }

                add_pin_to_signal_list(return_list, extension_pin);
                extension_pin->node = NULL;
            }
            break;
        }
        default:
            error_message(NETLIST, unknown_location, "%s", "Internal error, operation is not supported by Odin!\n");
            break;
    }

    //CLEAN UP
    free_signal_list(input_signals);

    return (return_list);
}