/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
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
 * @file: this file provides a routine to utilize the constant shift 
 * operation used in other node instantiation processes, such as multiplication.
 */

#include "Shift.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: constant_shift)
 * 
 * @brief performing constant shift operation on a given signal_list
 * 
 * @param input_signals input to be shifted 
 * @param shift_size constant shift size
 * @param shift_type shift type: SL, SR, ASL or ASR
 * @param assignment_size width of the output
 * @param netlist pointer to the current netlist file
 */
signal_list_t* constant_shift(signal_list_t* input_signals, const int shift_size, const operation_list shift_type, const int assignment_size, operation_list signedness, netlist_t* netlist) {
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

            int pad_bit = input_width - 1;
            /* Extend pad_bit to outputs that don't have inputs connected */
            for (i = output_width - 1; i >= input_width - shift_size; i--) {
                npin_t* extension_pin = NULL;
                if (signedness == SIGNED && shift_type == ASR) {
                    extension_pin = copy_input_npin(input_signals->pins[pad_bit]);
                } else {
                    extension_pin = get_zero_pin(netlist);
                }

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
