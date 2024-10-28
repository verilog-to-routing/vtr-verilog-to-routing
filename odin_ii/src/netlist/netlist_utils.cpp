/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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
 */

#include <cstring>
#include <cstdlib>

#include "odin_types.h"
#include "odin_globals.h"
#include "netlist_utils.h"
#include "odin_util.h"

#include "vtr_util.h"
#include "vtr_memory.h"

/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnode)
 *-------------------------------------------------------------------------------------------*/
nnode_t* allocate_nnode(loc_t loc) {
    nnode_t* new_node = (nnode_t*)my_malloc_struct(sizeof(nnode_t));

    new_node->loc = loc;
    new_node->name = NULL;
    new_node->type = NO_OP;
    new_node->bit_width = 0;
    new_node->related_ast_node = NULL;
    new_node->traverse_visited = -1;

    new_node->input_pins = NULL;
    new_node->num_input_pins = 0;
    new_node->output_pins = NULL;
    new_node->num_output_pins = 0;

    new_node->input_port_sizes = NULL;
    new_node->num_input_port_sizes = 0;
    new_node->output_port_sizes = NULL;
    new_node->num_output_port_sizes = 0;

    new_node->node_data = NULL;
    new_node->unique_node_data_id = -1;

    new_node->forward_level = -1;
    new_node->backward_level = -1;
    new_node->sequential_level = -1;
    new_node->sequential_terminator = false;

    new_node->internal_netlist = NULL;

    new_node->associated_function = NULL;

    new_node->simulate_block_cycle = NULL;

    new_node->bit_map = NULL;
    new_node->bit_map_line_count = 0;

    new_node->in_queue = false;

    new_node->undriven_pins = 0;
    new_node->num_undriven_pins = 0;

    new_node->ratio = 1;

    new_node->attributes = init_attribute();

    new_node->initial_value = init_value_e::undefined;

    new_node->generic_output = -1;

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_nnode)
 *-------------------------------------------------------------------------------------------*/
nnode_t* free_nnode(nnode_t* to_free) {
    if (to_free) {
        /* need to free node_data */

        for (int i = 0; i < to_free->num_input_pins; i++) {
            if (to_free->input_pins[i] && to_free->input_pins[i]->name) {
                vtr::free(to_free->input_pins[i]->name);
                to_free->input_pins[i]->name = NULL;
            }
            to_free->input_pins[i] = (npin_t*)vtr::free(to_free->input_pins[i]);
        }

        to_free->input_pins = (npin_t**)vtr::free(to_free->input_pins);

        for (int i = 0; i < to_free->num_output_pins; i++) {
            if (to_free->output_pins[i] && to_free->output_pins[i]->name) {
                vtr::free(to_free->output_pins[i]->name);
                to_free->output_pins[i]->name = NULL;
            }
            to_free->output_pins[i] = (npin_t*)vtr::free(to_free->output_pins[i]);
        }

        to_free->output_pins = (npin_t**)vtr::free(to_free->output_pins);

        vtr::free(to_free->input_port_sizes);
        vtr::free(to_free->output_port_sizes);
        vtr::free(to_free->undriven_pins);

        free_attribute(to_free->attributes);

        if (to_free->name) {
            vtr::free(to_free->name);
            to_free->name = NULL;
        }

        /* now free the node */
    }
    return (nnode_t*)vtr::free(to_free);
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_input_pins)
 * 	Makes more space in the node for pin connections ...
 *-----------------------------------------------------------------------*/
void allocate_more_input_pins(nnode_t* node, int width) {
    int i;

    if (width <= 0) {
        error_message(NETLIST, node->loc, "tried adding input pins for width %d <= 0 %s\n", width, node->name);
        return;
    }

    node->input_pins = (npin_t**)vtr::realloc(node->input_pins, sizeof(npin_t*) * (node->num_input_pins + width));
    for (i = 0; i < width; i++) {
        node->input_pins[node->num_input_pins + i] = NULL;
    }
    node->num_input_pins += width;
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_output_pins)
 * 	Makes more space in the node for pin connections ...
 *-----------------------------------------------------------------------*/
void allocate_more_output_pins(nnode_t* node, int width) {
    int i;

    if (width <= 0) {
        error_message(NETLIST, node->loc, "tried adding output pins for width %d <= 0 %s\n", width, node->name);
        return;
    }

    node->output_pins = (npin_t**)vtr::realloc(node->output_pins, sizeof(npin_t*) * (node->num_output_pins + width));
    for (i = 0; i < width; i++) {
        node->output_pins[node->num_output_pins + i] = NULL;
    }
    node->num_output_pins += width;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_output_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_output_port_information(nnode_t* node, int port_width) {
    node->output_port_sizes
        = (int*)vtr::realloc(node->output_port_sizes, sizeof(int) * (node->num_output_port_sizes + 1));
    node->output_port_sizes[node->num_output_port_sizes] = port_width;
    node->num_output_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_input_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_input_port_information(nnode_t* node, int port_width) {
    node->input_port_sizes = (int*)vtr::realloc(node->input_port_sizes, sizeof(int) * (node->num_input_port_sizes + 1));
    node->input_port_sizes[node->num_input_port_sizes] = port_width;
    node->num_input_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_npin)
 *-------------------------------------------------------------------------------------------*/
npin_t* allocate_npin() {
    npin_t* new_pin;

    new_pin = (npin_t*)my_malloc_struct(sizeof(npin_t));

    new_pin->name = NULL;
    new_pin->type = NO_ID;
    new_pin->net = NULL;
    new_pin->pin_net_idx = -1;
    new_pin->node = NULL;
    new_pin->pin_node_idx = -1;
    new_pin->mapping = NULL;

    new_pin->values = NULL;

    new_pin->coverage = 0;

    new_pin->is_default = false;
    new_pin->is_implied = false;

    return new_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_npin)
 *-------------------------------------------------------------------------------------------*/
npin_t* free_npin(npin_t* to_free) {
    if (to_free) {
        if (to_free->name) vtr::free(to_free->name);

        to_free->name = NULL;

        if (to_free->mapping) vtr::free(to_free->mapping);

        to_free->mapping = NULL;

        /* now free the pin */
    }
    return (npin_t*)vtr::free(to_free);
}

/*-------------------------------------------------------------------------
 * (function: copy_npin)
 * 	Copies a pin
 *  Should only be called by the parent functions,
 *  copy_input_npin & copy_output_npin
 *-----------------------------------------------------------------------*/
static npin_t* copy_npin(npin_t* copy_pin) {
    npin_t* new_pin = allocate_npin();
    new_pin->name = copy_pin->name ? vtr::strdup(copy_pin->name) : NULL;
    new_pin->type = copy_pin->type;
    new_pin->mapping = copy_pin->mapping ? vtr::strdup(copy_pin->mapping) : NULL;
    new_pin->is_default = copy_pin->is_default;
    new_pin->sensitivity = copy_pin->sensitivity;

    return new_pin;
}

/*-------------------------------------------------------------------------
 * (function: copy_output_npin)
 * 	Copies an output pin
 *-----------------------------------------------------------------------*/
npin_t* copy_output_npin(npin_t* copy_pin) {
    npin_t* new_pin = copy_npin(copy_pin);
    oassert(copy_pin->type == OUTPUT);
    new_pin->net = copy_pin->net;

    return new_pin;
}

/*-------------------------------------------------------------------------
 * (function: copy_input_npin)
 * 	Copies an input pin and potentially adds to the net
 *-----------------------------------------------------------------------*/
npin_t* copy_input_npin(npin_t* copy_pin) {
    npin_t* new_pin = copy_npin(copy_pin);
    oassert(copy_pin->type == INPUT);
    if (copy_pin->net != NULL) { add_fanout_pin_to_net(copy_pin->net, new_pin); }

    return new_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnet)
 *-------------------------------------------------------------------------------------------*/
nnet_t* allocate_nnet() {
    nnet_t* new_net = (nnet_t*)my_malloc_struct(sizeof(nnet_t));

    new_net->name = NULL;
    new_net->driver_pins = NULL;
    new_net->num_driver_pins = 0;
    new_net->fanout_pins = NULL;
    new_net->num_fanout_pins = 0;
    new_net->combined = false;

    new_net->net_data = NULL;
    new_net->unique_net_data_id = -1;

    new_net->initial_value = init_value_e::undefined;

    return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_nnet)
 *-------------------------------------------------------------------------------------------*/
nnet_t* free_nnet(nnet_t* to_free) {
    if (to_free) {
        to_free->fanout_pins = (npin_t**)vtr::free(to_free->fanout_pins);

        if (to_free->name) vtr::free(to_free->name);

        if (to_free->num_driver_pins) vtr::free(to_free->driver_pins);

        /* now free the net */
    }
    return (nnet_t*)vtr::free(to_free);
}

/*---------------------------------------------------------------------------
 * (function: move_a_output_pin)
 *-------------------------------------------------------------------------*/
void move_output_pin(nnode_t* node, int old_idx, int new_idx) {
    npin_t* pin;

    oassert(node != NULL);
    oassert(((old_idx >= 0) && (old_idx < node->num_output_pins)));
    oassert(((new_idx >= 0) && (new_idx < node->num_output_pins)));
    /* assumes the pin spots have been allocated and the pin */
    pin = node->output_pins[old_idx];
    node->output_pins[new_idx] = node->output_pins[old_idx];
    node->output_pins[old_idx] = NULL;
    /* record the node and pin spot in the pin */
    pin->type = OUTPUT;
    pin->node = node;
    pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------
 * (function: move_a_input_pin)
 *-------------------------------------------------------------------------*/
void move_input_pin(nnode_t* node, int old_idx, int new_idx) {
    npin_t* pin;

    oassert(node != NULL);
    oassert(((old_idx >= 0) && (old_idx < node->num_input_pins)));
    oassert(((new_idx >= 0) && (new_idx < node->num_input_pins)));
    /* assumes the pin spots have been allocated and the pin */
    node->input_pins[new_idx] = node->input_pins[old_idx];
    pin = node->input_pins[old_idx];
    node->input_pins[old_idx] = NULL;
    /* record the node and pin spot in the pin */
    pin->type = INPUT;
    pin->node = node;
    pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_input_pin_to_node(nnode_t* node, npin_t* pin, int pin_idx) {
    oassert(node != NULL);
    oassert(pin != NULL);
    oassert(pin_idx < node->num_input_pins);
    /* assumes the pin spots have been allocated and the pin */
    node->input_pins[pin_idx] = pin;
    /* record the node and pin spot in the pin */
    pin->type = INPUT;
    pin->node = node;
    pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_fanout_pin_to_net(nnet_t* net, npin_t* pin) {
    oassert(net != NULL);
    oassert(pin != NULL);
    oassert(pin->type != OUTPUT);
    /* assumes the pin spots have been allocated and the pin */
    net->fanout_pins = (npin_t**)vtr::realloc(net->fanout_pins, sizeof(npin_t*) * (net->num_fanout_pins + 1));
    net->fanout_pins[net->num_fanout_pins] = pin;
    net->num_fanout_pins++;
    /* record the node and pin spot in the pin */
    pin->net = net;
    pin->pin_net_idx = net->num_fanout_pins - 1;
    pin->type = INPUT;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_output_pin_to_node(nnode_t* node, npin_t* pin, int pin_idx) {
    oassert(node != NULL);
    oassert(pin != NULL);
    oassert(pin_idx < node->num_output_pins);
    /* assumes the pin spots have been allocated and the pin */
    node->output_pins[pin_idx] = pin;
    /* record the node and pin spot in the pin */
    pin->type = OUTPUT;
    pin->node = node;
    pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_driver_pin_to_net(nnet_t* net, npin_t* pin) {
    oassert(net != NULL);
    oassert(pin != NULL);
    oassert(pin->type != INPUT);
    /* assumes the pin spots have been allocated and the pin */
    net->num_driver_pins++;
    net->driver_pins = (npin_t**)vtr::realloc(net->driver_pins, net->num_driver_pins * sizeof(npin_t*));
    net->driver_pins[net->num_driver_pins - 1] = pin;
    /* record the node and pin spot in the pin */
    pin->net = net;
    pin->type = OUTPUT;
    pin->pin_net_idx = net->num_driver_pins - 1;
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_nets)
 * 	// output net is a net with a driver
 * 	// input net is a net with all the fanouts
 * 	The lasting one is input, and output disappears
 *-------------------------------------------------------------------------------------------*/
void combine_nets(nnet_t* output_net, nnet_t* input_net, netlist_t* netlist) {
    /* copy the driver over to the new_net */
    for (int i = 0; i < output_net->num_driver_pins; i++) {
        /* IF - there is a pin assigned to this net, then copy it */
        add_driver_pin_to_net(input_net, output_net->driver_pins[i]);
    }
    /* keep initial value */
    init_value_e output_initial_value = output_net->initial_value;
    /* in case there are any fanouts in output net (should only be zero and one nodes) */
    join_nets(input_net, output_net);
    /* mark that this is combined */
    input_net->combined = true;

    /* Need to keep the initial value data when we combine the nets */
    oassert(input_net->initial_value == init_value_e::undefined
            || input_net->initial_value == output_net->initial_value);
    input_net->initial_value = output_initial_value;

    /* special cases for global nets */
    if (output_net == netlist->zero_net) {
        netlist->zero_net = input_net;
    } else if (output_net == netlist->one_net) {
        netlist->one_net = input_net;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_nets_with_spot_copy)
 * // output net is a net with a driver
 * // input net is a net with all the fanouts
 * In addition to combining two nets, it will change the value of 
 * all input nets driven by the output_net previously.
 *-------------------------------------------------------------------------------------------*/
void combine_nets_with_spot_copy(nnet_t* output_net, nnet_t* input_net, long sc_spot_output, netlist_t* netlist) {
    long in_net_idx;
    long idx_array_size = 0;
    long* idx_array = NULL;

    // check to see if any matching input_net exist, then save its index
    for (in_net_idx = 0; in_net_idx < input_nets_sc->size; in_net_idx++) {
        if (input_nets_sc->data[in_net_idx] == output_net) {
            idx_array = (long*)vtr::realloc(idx_array, sizeof(long*) * (idx_array_size + 1));
            idx_array[idx_array_size] = in_net_idx;
            idx_array_size += 1;
        }
    }

    combine_nets(output_net, input_net, netlist);
    output_net = NULL;
    /* since the driver net is deleted, copy the spot of the input_net over */
    output_nets_sc->data[sc_spot_output] = (void*)input_net;

    // copy the spot of input_nets for other inputs driven by the output_net
    for (in_net_idx = 0; in_net_idx < idx_array_size; in_net_idx++) {
        char* net_name = input_nets_sc->string[idx_array[in_net_idx]];
        input_nets_sc->data[idx_array[in_net_idx]] = (void*)input_net;
        // check to see if there is any matching output net too.
        if ((sc_spot_output = sc_lookup_string(output_nets_sc, net_name)) != -1)
            output_nets_sc->data[sc_spot_output] = (void*)input_net;
    }

    vtr::free(idx_array);
}

/*---------------------------------------------------------------------------------------------
 * (function: join_nets)
 * 	Copies the fanouts from input net into net
 * TODO: improve error message
 *-------------------------------------------------------------------------------------------*/
void join_nets(nnet_t* join_to_net, nnet_t* other_net) {
    if (join_to_net == other_net) {
        for (int i = 0; i < join_to_net->num_driver_pins; i++) {
            const char* pin_name = join_to_net->driver_pins[i]->name ? join_to_net->driver_pins[i]->name : "unknown";
            if ((join_to_net->driver_pins[i]->node != NULL))
                warning_message(NETLIST, join_to_net->driver_pins[i]->node->loc, "%s %s\n",
                                "Combinational loop with driver pin", pin_name);
            else
                warning_message(NETLIST, unknown_location, "%s %s\n", "Combinational loop with driver pin", pin_name);
        }
        for (int i = 0; i < join_to_net->num_fanout_pins; i++) {
            const char* pin_name = join_to_net->fanout_pins[i]->name ? join_to_net->fanout_pins[i]->name : "unknown";
            if ((join_to_net->fanout_pins[i] != NULL) && (join_to_net->fanout_pins[i]->node != NULL))
                warning_message(NETLIST, join_to_net->fanout_pins[i]->node->loc, "%s %s\n",
                                "Combinational loop with fanout pin", pin_name);
            else
                warning_message(NETLIST, unknown_location, "%s %s\n", "Combinational loop with fanout pin", pin_name);
        }

        error_message(NETLIST, unknown_location, "%s", "Found a combinational loop");
    } else if (other_net->num_driver_pins > 1) {
        if (other_net->name && join_to_net->name)
            error_message(NETLIST, unknown_location,
                          "Tried to join net %s to %s but this would lose %d drivers for net %s", other_net->name,
                          join_to_net->name, other_net->num_driver_pins - 1, other_net->name);
        else
            error_message(NETLIST, unknown_location, "Tried to join nets but this would lose %d drivers",
                          other_net->num_driver_pins - 1);
    }

    /* copy the driver over to the new_net */
    for (int i = 0; i < other_net->num_fanout_pins; i++) {
        if (other_net->fanout_pins[i]) { add_fanout_pin_to_net(join_to_net, other_net->fanout_pins[i]); }
    }

    // CLEAN UP
    free_nnet(other_net);
}

/*---------------------------------------------------------------------------------------------
 * (function: integrate_nets)
 * processing the integration of the input net, named with the 
 * full_name string (if not exist in input_nets_sc then use driver_net), 
 * with the alias net (a related module/function/task instance connection).
 *-------------------------------------------------------------------------------------------*/
void integrate_nets(char* alias_name, char* full_name, nnet_t* driver_net) {
    long sc_spot_output;
    long sc_spot_input_old;
    long sc_spot_input_new;

    sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name);
    oassert(sc_spot_input_old != -1);

    /* CMM - Check if this pin should be driven by the top level VCC or GND drivers	*/
    if (strstr(full_name, ONE_VCC_CNS)) {
        join_nets(syn_netlist->one_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
        input_nets_sc->data[sc_spot_input_old] = (void*)syn_netlist->one_net;
    } else if (strstr(full_name, ZERO_GND_ZERO)) {
        join_nets(syn_netlist->zero_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
        input_nets_sc->data[sc_spot_input_old] = (void*)syn_netlist->zero_net;
    } else if (strstr(full_name, ZERO_PAD_ZERO)) {
        join_nets(syn_netlist->pad_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
        input_nets_sc->data[sc_spot_input_old] = (void*)syn_netlist->pad_net;
    }
    /* check if the instantiation pin exists. */
    else if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
        /* IF - no driver, then assume that it needs to be aliased to move up as an input */
        if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
            /* if this input is not yet used in this module then we'll add it */
            sc_spot_input_new = sc_add_string(input_nets_sc, full_name);

            if (driver_net == NULL) {
                /* copy the pin to the old spot */
                input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
            } else {
                /* copy the pin to the old spot */
                input_nets_sc->data[sc_spot_input_new] = (void*)driver_net;
                nnet_t* old_in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];
                join_nets((nnet_t*)input_nets_sc->data[sc_spot_input_new], old_in_net);
                //net = NULL;
                input_nets_sc->data[sc_spot_input_old] = (void*)driver_net;
            }
        } else {
            /* already exists so we'll join the nets */
            combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old],
                         (nnet_t*)input_nets_sc->data[sc_spot_input_new], syn_netlist);
            input_nets_sc->data[sc_spot_input_old] = NULL;
        }
    } else {
        /* ELSE - we've found a matching net, so add this pin to the net */
        nnet_t* out_net = (nnet_t*)output_nets_sc->data[sc_spot_output];
        nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

        if ((out_net != in_net) && (out_net->combined == true)) {
            /* if they haven't been combined already, then join the inputs and output */
            join_nets(out_net, in_net);
            /* since the driver net is deleted, copy the spot of the in_net over */
            input_nets_sc->data[sc_spot_input_old] = (void*)out_net;
        } else if ((out_net != in_net) && (out_net->combined == false)) {
            // merge the out_net into the in_net and alter related string cache data for all nets driven by the out_net
            combine_nets_with_spot_copy(out_net, in_net, sc_spot_output, syn_netlist);
        }
    }
}

/*-------------------------------------------------------------------------
 * (function: remap_pin_to_new_node)
 *-----------------------------------------------------------------------*/
void remap_pin_to_new_node(npin_t* pin, nnode_t* new_node, int pin_idx) {
    if (pin->type == INPUT) {
        /* clean out the entry in the old net */
        pin->node->input_pins[pin->pin_node_idx] = NULL;
        /* do the new addition */
        add_input_pin_to_node(new_node, pin, pin_idx);
    } else if (pin->type == OUTPUT) {
        /* clean out the entry in the old net */
        pin->node->output_pins[pin->pin_node_idx] = NULL;
        /* do the new addition */
        add_output_pin_to_node(new_node, pin, pin_idx);
    }
}

/*------------------------------------------------------------------------
 * (function: connect_nodes)
 * 	Connect one output node to the inputs of the input node
 *----------------------------------------------------------------------*/
void connect_nodes(nnode_t* out_node, int out_idx, nnode_t* in_node, int in_idx) {
    npin_t* new_in_pin;

    oassert(out_node->num_output_pins > out_idx);
    oassert(in_node->num_input_pins > in_idx);

    new_in_pin = allocate_npin();

    /* create the pin that hooks up to the input */
    add_input_pin_to_node(in_node, new_in_pin, in_idx);

    if (out_node->output_pins[out_idx] == NULL) {
        /* IF - this node has no output net or pin */
        npin_t* new_out_pin;
        nnet_t* new_net;
        new_net = allocate_nnet();
        new_out_pin = allocate_npin();

        new_net->name = vtr::strdup(out_node->name);
        /* create the pin that hooks up to the input */
        add_output_pin_to_node(out_node, new_out_pin, out_idx);
        /* hook up in pin out of the new net */
        add_fanout_pin_to_net(new_net, new_in_pin);
        /* hook up the new pin 2 to this new net */
        add_driver_pin_to_net(new_net, new_out_pin);
    } else {
        /* ELSE - there is a net so we just add a fanout */
        /* hook up in pin out of the new net */
        add_fanout_pin_to_net(out_node->output_pins[out_idx]->net, new_in_pin);
    }
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: init_attribute_structure)
 * 
 * @brief Initializes the netlist node attributes 
 * including edge sensitivies and reset value
 *-------------------------------------------------------------------------------------------*/
attr_t* init_attribute() {
    attr_t* attribute;
    attribute = (attr_t*)vtr::malloc(sizeof(attr_t));

    attribute->clk_edge_type = UNDEFINED_SENSITIVITY;
    attribute->clr_polarity = UNDEFINED_SENSITIVITY;
    attribute->set_polarity = UNDEFINED_SENSITIVITY;
    attribute->areset_polarity = UNDEFINED_SENSITIVITY;
    attribute->sreset_polarity = UNDEFINED_SENSITIVITY;
    attribute->enable_polarity = UNDEFINED_SENSITIVITY;

    attribute->areset_value = 0;
    attribute->sreset_value = 0;

    attribute->port_a_signed = UNSIGNED;
    attribute->port_b_signed = UNSIGNED;

    /* memory node attributes */
    attribute->size = 0;
    attribute->offset = 0;
    attribute->memory_id = NULL;

    attribute->RD_CLK_ENABLE = UNDEFINED_SENSITIVITY;
    attribute->WR_CLK_ENABLE = UNDEFINED_SENSITIVITY;
    attribute->RD_CLK_POLARITY = UNDEFINED_SENSITIVITY;
    attribute->WR_CLK_POLARITY = UNDEFINED_SENSITIVITY;

    attribute->RD_PORTS = 0;
    attribute->WR_PORTS = 0;
    attribute->DBITS = 0;
    attribute->ABITS = 0;

    return (attribute);
}

/*---------------------------------------------------------------------------------------------
 * (function: init_signal_list_structure)
 * 	Initializes the list structure which describes inputs and outputs of elements
 * 	as they coneect to other elements in the graph.
 *-------------------------------------------------------------------------------------------*/
signal_list_t* init_signal_list() {
    signal_list_t* list;
    list = (signal_list_t*)vtr::malloc(sizeof(signal_list_t));

    list->count = 0;
    list->pins = NULL;
    list->is_memory = false;
    list->is_adder = false;

    return list;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: is_constant_signal)
 *
 * @brief showing that a given signal list has a constant value or not
 * 
 * @param signal list of pins
 * @param netlist pointer to the current netlist file
 * 
 * @return is it constant or not
 *-------------------------------------------------------------------------------------------*/
bool is_constant_signal(signal_list_t* signal, netlist_t* netlist) {
    int i;
    bool is_constant = true;

    for (i = 0; i < signal->count; i++) {
        nnet_t* net = signal->pins[i]->net;
        /* neither connected to GND nor VCC */
        if (strcmp(net->name, netlist->zero_net->name) && strcmp(net->name, netlist->one_net->name)) {
            is_constant = false;
            break;
        }
    }

    return (is_constant);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: constant_signal_value)
 *
 * @brief calculating the value of a constant signal list
 * 
 * @param signal list of pins
 * @param netlist pointer to the current netlist file
 * 
 * @return the integer value of the constant signal
 *-------------------------------------------------------------------------------------------*/
long constant_signal_value(signal_list_t* signal, netlist_t* netlist) {
    oassert(is_constant_signal(signal, netlist));

    long return_value = 0;

    int i;
    for (i = 0; i < signal->count; i++) {
        nnet_t* net = signal->pins[i]->net;
        /* if the pin is connected to VCC */
        if (!strcmp(net->name, netlist->one_net->name)) {
            return_value += shift_left_value_with_overflow_check(0X1, i, unknown_location);
        }
    }

    return (return_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: add_inpin_to_signal_list)
 * 	Stores a pin in the signal list
 *-------------------------------------------------------------------------------------------*/
void add_pin_to_signal_list(signal_list_t* list, npin_t* pin) {
    list->pins = (npin_t**)vtr::realloc(list->pins, sizeof(npin_t*) * (list->count + 1));
    list->pins[list->count] = pin;
    list->count++;
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_lists)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* combine_lists(signal_list_t** signal_lists, int num_signal_lists) {
    int i;
    for (i = 1; i < num_signal_lists; i++) {
        if (signal_lists[i]) {
            int j;
            for (j = 0; j < signal_lists[i]->count; j++) {
                int k;
                bool pin_already_added = false;
                for (k = 0; k < signal_lists[0]->count; k++) {
                    if (!strcmp(signal_lists[0]->pins[k]->name, signal_lists[i]->pins[j]->name))
                        pin_already_added = true;
                }

                if (!pin_already_added) add_pin_to_signal_list(signal_lists[0], signal_lists[i]->pins[j]);
            }

            free_signal_list(signal_lists[i]);
        }
    }

    return signal_lists[0];
}

/**
 * (function: split_list)
 * 
 * @brief split signals list to a list of signal list with requested width
 * 
 * @param signalsvar signal list of pins (may include one signal or two signals)
 * @param width the width of each signal in signalsvar
 * 
 * @return splitted signal list
 */
signal_list_t** split_signal_list(signal_list_t* signalsvar, const int width) {
    signal_list_t** splitted_signals = NULL;

    /* check if split is needed */
    if (signalsvar->count == width) {
        splitted_signals = (signal_list_t**)vtr::calloc(1, sizeof(signal_list_t*));
        splitted_signals[0] = signalsvar;
        return (splitted_signals);
    }

    /* validate signals list size */
    oassert(width != 0);
    oassert(signalsvar->count % width == 0);

    int i, j;
    int offset = 0;
    int num_chunk = signalsvar->count / width;

    /* initialize splitted signals */
    splitted_signals = (signal_list_t**)vtr::calloc(num_chunk, sizeof(signal_list_t*));
    for (i = 0; i < num_chunk; i++) {
        splitted_signals[i] = init_signal_list();
        for (j = 0; j < width; j++) {
            npin_t* pin = signalsvar->pins[j + offset];
            /* add to splitted signals list */
            add_pin_to_signal_list(splitted_signals[i], pin);
        }
        offset += width;
    }

    return (splitted_signals);
}

/**
 * (function: sigcmp)
 * 
 * @brief to check if sig is the same as be_checked
 * 
 * @param sig first signals
 * @param be_checked second signals
 */
bool sigcmp(signal_list_t* sig, signal_list_t* be_checked) {
    /* validate signal sizes */
    oassert(sig->count == be_checked->count);

    int i;
    for (i = 0; i < sig->count; i++) {
        /* checking their net */
        if (sig->pins[i]->net != be_checked->pins[i]->net) { return (false); }
    }
    return (true);
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_lists_without_freeing_originals)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* combine_lists_without_freeing_originals(signal_list_t** signal_lists, int num_signal_lists) {
    signal_list_t* return_list = init_signal_list();

    int i;
    for (i = 0; i < num_signal_lists; i++) {
        int j;
        if (signal_lists[i])
            for (j = 0; j < signal_lists[i]->count; j++)
                add_pin_to_signal_list(return_list, signal_lists[i]->pins[j]);
    }

    return return_list;
}

signal_list_t* copy_input_signals(signal_list_t* signalsvar) {
    signal_list_t* duplicate_signals = init_signal_list();
    int i;
    for (i = 0; i < signalsvar->count; i++) {
        npin_t* pin = signalsvar->pins[i];
        pin = copy_input_npin(pin);
        add_pin_to_signal_list(duplicate_signals, pin);
    }
    return duplicate_signals;
}

signal_list_t* copy_output_signals(signal_list_t* signalsvar) {
    signal_list_t* duplicate_signals = init_signal_list();
    int i;
    for (i = 0; i < signalsvar->count; i++) {
        npin_t* pin = signalsvar->pins[i];
        add_pin_to_signal_list(duplicate_signals, copy_output_npin(pin));
    }
    return duplicate_signals;
}

/*---------------------------------------------------------------------------------------------
 * (function: sort_signal_list_alphabetically)
 * Bubble sort alphabetically
 * Andrew: changed to a quick sort because this function
 *         was consuming 99.6% of compile time for mcml.v
 *-------------------------------------------------------------------------------------------*/
static int compare_npin_t_names(const void* p1, const void* p2) {
    npin_t* pin1 = *(npin_t* const*)p1;
    npin_t* pin2 = *(npin_t* const*)p2;
    return strcmp(pin1->name, pin2->name);
}

void sort_signal_list_alphabetically(signal_list_t* list) {
    if (list) qsort(list->pins, list->count, sizeof(npin_t*), compare_npin_t_names);
}

/*---------------------------------------------------------------------------------------------
 * (function: make_output_pins_for_existing_node)
 * 	Looks at a node and extracts the output pins into a signal list so they can be accessed
 * 	in this form
 *-------------------------------------------------------------------------------------------*/
signal_list_t* make_output_pins_for_existing_node(nnode_t* node, int width) {
    signal_list_t* return_list = init_signal_list();
    int i;

    oassert(node->num_output_pins == width);

    for (i = 0; i < width; i++) {
        npin_t* new_pin1;
        npin_t* new_pin2;
        nnet_t* new_net;
        new_pin1 = allocate_npin();
        new_pin2 = allocate_npin();
        new_net = allocate_nnet();
        new_net->name = node->name;
        /* hook the output pin into the node */
        add_output_pin_to_node(node, new_pin1, i);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* add the new_pin2 to the list of pins */
        add_pin_to_signal_list(return_list, new_pin2);
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: clean_signal_list_structure)
 *-------------------------------------------------------------------------------------------*/
void free_signal_list(signal_list_t* list) {
    if (list) {
        vtr::free(list->pins);
        list->count = 0;
    }
    vtr::free(list);
    list = NULL;
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: free_attribute)
 *
 * @brief clean the given attribute structure to avoid mem leaks
 *
 * @param attribute the given attribute structure
 *-------------------------------------------------------------------------------------------*/
void free_attribute(attr_t* attribute) {
    if (attribute) {
        vtr::free(attribute->memory_id);
        attribute->memory_id = NULL;
    }

    vtr::free(attribute);
    attribute = NULL;
}

/*---------------------------------------------------------------------------
 * (function: hookup_input_pins_from_signal_list)
 * 	For each pin in this list hook it up to the inputs according to indexes and width
 *--------------------------------------------------------------------------*/
void hookup_input_pins_from_signal_list(nnode_t* node,
                                        int n_start_idx,
                                        signal_list_t* input_list,
                                        int il_start_idx,
                                        int width,
                                        netlist_t* netlist) {
    int i;

    for (i = 0; i < width; i++) {
        if (il_start_idx + i < input_list->count) {
            oassert(input_list->count > (il_start_idx + i));
            npin_t* pin = input_list->pins[il_start_idx + i];
            add_input_pin_to_node(node, pin, n_start_idx + i);

        } else {
            /* pad with 0's */
            add_input_pin_to_node(node, get_zero_pin(netlist), n_start_idx + i);

            if (global_args.all_warnings)
                warning_message(NETLIST, node->loc, "padding an input port with 0 for node %s\n", node->name);
        }
    }
}

/*---------------------------------------------------------------------------
 * (function: hookup_hb_input_pins_from_signal_list)
 *   For each pin in this list hook it up to the inputs according to
 *   indexes and width. Extra pins are tied to PAD for later resolution.
 *--------------------------------------------------------------------------*/
void hookup_hb_input_pins_from_signal_list(nnode_t* node,
                                           int n_start_idx,
                                           signal_list_t* input_list,
                                           int il_start_idx,
                                           int width,
                                           netlist_t* netlist) {
    int i;

    for (i = 0; i < width; i++) {
        if (il_start_idx + i < input_list->count) {
            oassert(input_list->count > (il_start_idx + i));
            add_input_pin_to_node(node, input_list->pins[il_start_idx + i], n_start_idx + i);
        } else {
            /* connect with "pad" signal for later resolution */
            add_input_pin_to_node(node, get_pad_pin(netlist), n_start_idx + i);

            if (global_args.all_warnings)
                warning_message(NETLIST, node->loc, "padding an input port with HB_PAD for node %s\n", node->name);
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: hookup_output_pouts_from_signal_list)
 * 	hooks the pin into the output net, by checking for a driving net
 *-------------------------------------------------------------------------------------------*/
void hookup_output_pins_from_signal_list(nnode_t* node,
                                         int n_start_idx,
                                         signal_list_t* output_list,
                                         int ol_start_idx,
                                         int width) {
    int i;
    long sc_spot_output;

    for (i = 0; i < width; i++) {
        oassert(output_list->count > (ol_start_idx + i));

        /* hook outpin to the node */
        add_output_pin_to_node(node, output_list->pins[ol_start_idx + i], n_start_idx + i);

        if ((sc_spot_output = sc_lookup_string(output_nets_sc, output_list->pins[ol_start_idx + i]->name)) == -1) {
            /* this output pin does not have a net OR we couldn't find it */
            error_message(NETLIST, node->loc, "Net for driver (%s) doesn't exist for node %s\n",
                          output_list->pins[ol_start_idx + i]->name, node->name);
        }

        /* hook the outpin into the net */
        add_driver_pin_to_net(((nnet_t*)output_nets_sc->data[sc_spot_output]), output_list->pins[ol_start_idx + i]);
    }
}

void depth_traverse_count(nnode_t* node, int* count, uintptr_t traverse_mark_number);

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_traverse_count(nnode_t* node, int* count, uintptr_t traverse_mark_number) {
    int i, j;
    nnode_t* next_node;
    nnet_t* next_net;

    if (node->traverse_visited == traverse_mark_number) {
        return;
    } else {
        /* ELSE - this is a new node so depth visit it */
        (*count)++;

        /* mark that we have visitied this node now */
        node->traverse_visited = traverse_mark_number;

        for (i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL) continue;

            next_net = node->output_pins[i]->net;
            for (j = 0; j < next_net->num_fanout_pins; j++) {
                if (next_net->fanout_pins[j] == NULL) continue;
                next_node = next_net->fanout_pins[j]->node;
                if (next_node == NULL) continue;

                /* recursive call point */
                depth_traverse_count(next_node, count, traverse_mark_number);
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function:  allocate_netlist)
 *-------------------------------------------------------------------------------------------*/
netlist_t* allocate_netlist() {
    netlist_t* new_netlist;

    new_netlist = (netlist_t*)my_malloc_struct(sizeof(netlist_t));

    new_netlist->gnd_node = NULL;
    new_netlist->vcc_node = NULL;
    new_netlist->pad_node = NULL;
    new_netlist->zero_net = NULL;
    new_netlist->one_net = NULL;
    new_netlist->pad_net = NULL;
    new_netlist->top_input_nodes = NULL;
    new_netlist->num_top_input_nodes = 0;
    new_netlist->top_output_nodes = NULL;
    new_netlist->num_top_output_nodes = 0;
    new_netlist->ff_nodes = NULL;
    new_netlist->num_ff_nodes = 0;
    new_netlist->internal_nodes = NULL;
    new_netlist->num_internal_nodes = 0;
    new_netlist->clocks = NULL;
    new_netlist->num_clocks = 0;

    new_netlist->forward_levels = NULL;
    new_netlist->num_forward_levels = 0;
    new_netlist->num_at_forward_level = NULL;
    new_netlist->backward_levels = NULL;
    new_netlist->num_backward_levels = 0;
    new_netlist->num_at_backward_level = NULL;
    new_netlist->sequential_level_nodes = NULL;
    new_netlist->num_sequential_levels = 0;
    new_netlist->num_at_sequential_level = NULL;
    new_netlist->sequential_level_combinational_termination_node = NULL;
    new_netlist->num_sequential_level_combinational_termination_nodes = 0;
    new_netlist->num_at_sequential_level_combinational_termination_node = NULL;

    /* initialize the string chaches */
    new_netlist->nets_sc = sc_new_string_cache();
    new_netlist->out_pins_sc = sc_new_string_cache();
    new_netlist->nodes_sc = sc_new_string_cache();

    return new_netlist;
}

/*---------------------------------------------------------------------------------------------
 * (function:  free_netlist)
 *-------------------------------------------------------------------------------------------*/
void free_netlist(netlist_t* to_free) {
    if (!to_free) return;

    sc_free_string_cache(to_free->nets_sc);
    sc_free_string_cache(to_free->out_pins_sc);
    sc_free_string_cache(to_free->nodes_sc);
}

/*
 * Gets the index of the first output pin with the given mapping
 * on the given node.
 */
int get_output_pin_index_from_mapping(nnode_t* node, const char* name) {
    int i;
    for (i = 0; i < node->num_output_pins; i++) {
        npin_t* pin = node->output_pins[i];
        if (!strcmp(pin->mapping, name)) return i;
    }

    return -1;
}

/*
 * Gets the index of the first output port containing a pin with the given
 * mapping.
 */
int get_output_port_index_from_mapping(nnode_t* node, const char* name) {
    int i;
    int pin_number = 0;
    for (i = 0; i < node->num_output_port_sizes; i++) {
        int j;
        for (j = 0; j < node->output_port_sizes[i]; j++, pin_number++) {
            npin_t* pin = node->output_pins[pin_number];
            if (!strcmp(pin->mapping, name)) return i;
        }
    }
    return -1;
}

/*
 * Gets the index of the first pin with the given mapping.
 */
int get_input_pin_index_from_mapping(nnode_t* node, const char* name) {
    int i;
    for (i = 0; i < node->num_input_pins; i++) {
        npin_t* pin = node->input_pins[i];
        if (!strcmp(pin->mapping, name)) return i;
    }

    return -1;
}

/*
 * Gets the port index of the first port containing a pin with
 * the given mapping.
 */
int get_input_port_index_from_mapping(nnode_t* node, const char* name) {
    int i;
    int pin_number = 0;
    for (i = 0; i < node->num_input_port_sizes; i++) {
        int j;
        for (j = 0; j < node->input_port_sizes[i]; j++, pin_number++) {
            npin_t* pin = node->input_pins[pin_number];
            if (!strcmp(pin->mapping, name)) return i;
        }
    }
    return -1;
}

chain_information_t* allocate_chain_info() {
    chain_information_t* new_node;

    new_node = (chain_information_t*)my_malloc_struct(sizeof(chain_information_t));

    new_node->name = NULL;
    new_node->count = 0;

    return new_node;
}

void remove_fanout_pins_from_net(nnet_t* net, npin_t* /*pin*/, int id) {
    int i;
    for (i = id; i < net->num_fanout_pins - 1; i++) {
        net->fanout_pins[i] = net->fanout_pins[i + 1];
        if (net->fanout_pins[i] != NULL) net->fanout_pins[i]->pin_net_idx = i;
    }
    net->fanout_pins[i] = NULL;
    net->num_fanout_pins--;
}

void delete_npin(npin_t* pin) {
    if (pin->type == INPUT) {
        /* detach from its node */
        pin->node->input_pins[pin->pin_node_idx] = NULL;
        /* detach from its net */
        pin->net->fanout_pins[pin->pin_net_idx] = NULL;
    } else if (pin->type == OUTPUT) {
        /* detach from its node */
        pin->node->output_pins[pin->pin_node_idx] = NULL;
        /* detach from its net */
        pin->net->driver_pins[pin->pin_net_idx] = NULL;
    }
    // CLEAN UP
    free_npin(pin);
}

/**
 * TODO: these are unused in the code. is this functional and/or is it ripe to remove?
 */

// void function_to_print_node_and_its_pin(npin_t * temp_pin);
// void function_to_print_node_and_its_pin(npin_t * temp_pin)
// {
// 	int i;
// 	nnode_t *node;
// 	npin_t *pin;

// 	printf("\n-------Printing the related net driver pin info---------\n");
// 	node=temp_pin->node;

//   	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   	for(i=0;i<node->num_input_port_sizes;i++)
//   	{
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   	}
//  	 for(i=0;i<node->num_input_pins;i++)
//   	{
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   	}
//   	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//  	 for(i=0;i<node->num_output_port_sizes;i++)
//   	{
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   	}
//   	for(i=0;i<node->num_output_pins;i++)
//   	{
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//  	}

// }
// void print_netlist_for_checking (netlist_t *netlist, char *name)
// {
//   int i,j,k;
//   npin_t * pin;
//   nnet_t * net;
//   nnode_t * node;

//   printf("printing the netlist : %s\n",name);
//   /* gnd_node */
//   node=netlist->gnd_node;
//   net=netlist->zero_net;
//   printf("--------gnd_node-------\n");
//   printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   for(i=0;i<node->num_input_port_sizes;i++)
//   {
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   }
//   for(i=0;i<node->num_input_pins;i++)
//   {
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);

//   }
//   printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//   for(i=0;i<node->num_output_port_sizes;i++)
//   {
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   }
//   for(i=0;i<node->num_output_pins;i++)
//   {
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node : %s",pin->net->name,pin->node->name);
//   }

//   printf("\n----------zero net----------\n");
//   printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
//   for(i=0;i<net->num_fanout_pins;i++)
//   {
//     printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
//   }

//    /* vcc_node */
//   node=netlist->vcc_node;
//   net=netlist->one_net;
//   printf("\n--------vcc_node-------\n");
//   printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   for(i=0;i<node->num_input_port_sizes;i++)
//   {
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   }
//   for(i=0;i<node->num_input_pins;i++)
//   {
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   }
//   printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//   for(i=0;i<node->num_output_port_sizes;i++)
//   {
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   }
//   for(i=0;i<node->num_output_pins;i++)
//   {
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   }

//   printf("\n----------one net----------\n");
//   printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
//   for(i=0;i<net->num_fanout_pins;i++)
//   {
//     printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
//   }

//   /* pad_node */
//   node=netlist->pad_node;
//   net=netlist->pad_net;
//   printf("\n--------pad_node-------\n");
//   printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   for(i=0;i<node->num_input_port_sizes;i++)
//   {
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   }
//   for(i=0;i<node->num_input_pins;i++)
//   {
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   }
//   printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//   for(i=0;i<node->num_output_port_sizes;i++)
//   {
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   }
//   for(i=0;i<node->num_output_pins;i++)
//   {
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   }

//   printf("\n----------pad net----------\n");
//   printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
//   for(i=0;i<net->num_fanout_pins;i++)
//   {
//     printf("fanout_pins %ld : %s",i,net->fanout_pins[i]->name);
//   }

//   /* top input nodes */
//   printf("\n--------------Printing the top input nodes--------------------------- \n");
//   printf("num_top_input_nodes: %ld",netlist->num_top_input_nodes);
//   for(j=0;j<netlist->num_top_input_nodes;j++)
//   {
//   	node=netlist->top_input_nodes[j];
// 	printf("\ttop input nodes : %ld\n",j);
//   	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   	for(i=0;i<node->num_input_port_sizes;i++)
//   	{
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   	}
//  	 for(i=0;i<node->num_input_pins;i++)
//   	{
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   	}
//   	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//  	 for(i=0;i<node->num_output_port_sizes;i++)
//   	{
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   	}
//   	for(i=0;i<node->num_output_pins;i++)
//   	{
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//  	}
// }

//   /* top output nodes */
//   printf("\n--------------Printing the top output nodes--------------------------- \n");
//   printf("num_top_output_nodes: %ld",netlist->num_top_output_nodes);
//   for(j=0;j<netlist->num_top_output_nodes;j++)
//   {
//   	node=netlist->top_output_nodes[j];
// 	printf("\ttop output nodes : %ld\n",j);
//   	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   	for(i=0;i<node->num_input_port_sizes;i++)
//   	{
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   	}
//  	 for(i=0;i<node->num_input_pins;i++)
//   	{
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
// 		net=pin->net;
// 		printf("\n\t-------printing the related net info-----\n");
// 		printf("unique_id : %ld name: %s combined: %ld \n",net->unique_id,net->name,net->combined);
//   printf("driver_pin name : %s num_fanout_pins: %ld\n",net->driver_pin->name,net->num_fanout_pins);
//   		for(k=0;k<net->num_fanout_pins;k++)
//   		{
//     		printf("fanout_pins %ld : %s",k,net->fanout_pins[k]->name);
//   		}
// 		 function_to_print_node_and_its_pin(net->driver_pin);
//   	}
//   	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//  	 for(i=0;i<node->num_output_port_sizes;i++)
//   	{
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   	}
//   	for(i=0;i<node->num_output_pins;i++)
//   	{
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//  	}

//   }

//   /* internal nodes */
//   printf("\n--------------Printing the internal nodes--------------------------- \n");
//   printf("num_internal_nodes: %ld",netlist->num_internal_nodes);
//   for(j=0;j<netlist->num_internal_nodes;j++)
//   {
//   	node=netlist->internal_nodes[j];
// 	printf("\tinternal nodes : %ld\n",j);
//   	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   	for(i=0;i<node->num_input_port_sizes;i++)
//   	{
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   	}
//  	 for(i=0;i<node->num_input_pins;i++)
//   	{
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   	}
//   	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//  	 for(i=0;i<node->num_output_port_sizes;i++)
//   	{
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   	}
//   	for(i=0;i<node->num_output_pins;i++)
//   	{
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//  	}
// }

//   /* ff nodes */
//   printf("\n--------------Printing the ff nodes--------------------------- \n");
//   printf("num_ff_nodes: %ld",netlist->num_ff_nodes);
//   for(j=0;j<netlist->num_ff_nodes;j++)
//   {
//   	node=netlist->ff_nodes[j];
// 	printf("\tff nodes : %ld\n",j);
//   	printf(" unique_id: %ld   name: %s type: %ld\n",node->unique_id,node->name,node->type);
//   	printf(" num_input_pins: %ld  num_input_port_sizes: %ld",node->num_input_pins,node->num_input_port_sizes);
//   	for(i=0;i<node->num_input_port_sizes;i++)
//   	{
// 	printf("input_port_sizes %ld : %ld\n",i,node->input_port_sizes[i]);
//   	}
//  	 for(i=0;i<node->num_input_pins;i++)
//   	{
// 	pin=node->input_pins[i];
// 	printf("input_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s",pin->net->name,pin->node->name);
//   	}
//   	printf(" num_output_pins: %ld  num_output_port_sizes: %ld",node->num_output_pins,node->num_output_port_sizes);
//  	 for(i=0;i<node->num_output_port_sizes;i++)
//   	{
// 	printf("output_port_sizes %ld : %ld\n",i,node->output_port_sizes[i]);
//   	}
//   	for(i=0;i<node->num_output_pins;i++)
//   	{
// 	pin=node->output_pins[i];
// 	printf("output_pins %ld : unique_id : %ld type :%ld \n \tname :%s pin_net_idx :%ld \n\tpin_node_idx:%ld mapping:%s\n",i,pin->unique_id,pin->type,pin->name,pin->pin_net_idx,pin->pin_node_idx,pin->mapping);
// 	printf("\t related net : %s related node: %s\n",pin->net->name,pin->node->name);
//  	}
// }
// }
