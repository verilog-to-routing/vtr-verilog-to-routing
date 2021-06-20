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

#include "latch.hh"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: resolve_dlatch_node)
 * 
 * @brief split the dlatch node read from yosys blif to
 * latch nodes with input/output width one
 * 
 * @param node pointing to the dlatch node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_dlatch_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == node->num_output_port_sizes + 1);

    /* making sure the enable input is 1 bit */
    oassert(node->input_port_sizes[1] == 1);

    /**
     * D:   input port 0
     * EN:  input port 1
     * Q:   output port 0
    */
    int i;
    int D_width = node->input_port_sizes[0];
    // int EN_width = node->input_port_sizes[1]; // == 1

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[D_width],
                          select_enable,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1 = allocate_npin();
    npin_t* select_enable_output = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, select_enable->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_enable, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, select_enable_output);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** buf_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** VALUE_MUX ***************************************/
        /*****************************************************************************************/
        muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, muxes[i], 1);
        connect_nodes(not_select_enable, 0, muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i],
                              muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_mux = allocate_npin();
        npin_t* mux_output_pin = allocate_npin();
        nnet_t* new_net_mux = allocate_nnet();
        new_net_mux->name = make_full_ref_name(NULL, NULL, NULL, muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(muxes[i], new_pin1_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_mux, new_pin1_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_mux, mux_output_pin);

        /*****************************************************************************************/
        /**************************************** BUF_NODE ***************************************/
        /*****************************************************************************************/

        buf_nodes[i] = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        /**
         * connecting the data input of 
         * (EN == EN_POLARITY) ? D : Q
         */
        add_input_pin_to_node(buf_nodes[i],
                              mux_output_pin,
                              0);

        /* connectiong the buf output pin to the buf_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              buf_nodes[i],
                              0);
    }

    // CLEAN UP
    free_nnode(node);

    vtr::free(muxes);
    vtr::free(buf_nodes);
}

/**
 * (function: resolve_adlatch_node)
 * 
 * @brief split the adlatch node read from yosys blif to
 * latch nodes with input/output width one
 * 
 * @param node pointing to the adlatch node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_adlatch_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the arst input is 1 bit */
    oassert(node->input_port_sizes[0] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * ARST: input port 0
     * D:    input port 1
     * EN:   input port 2
     * Q:    output port 0
    */
    int i;
    int ARST_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[ARST_width + D_width],
                          select_enable,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_en = allocate_npin();
    npin_t* select_enable_output = allocate_npin();
    nnet_t* new_net_en = allocate_nnet();
    new_net_en->name = make_full_ref_name(NULL, NULL, NULL, select_enable->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_enable, new_pin1_en, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_en, new_pin1_en);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_en, select_enable_output);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /*****************************************************************************************/
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* reset_value = create_constant_signal(node->attributes->areset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting ARST pin */
    remap_pin_to_new_node(node->input_pins[0],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->areset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->areset_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_rst = allocate_npin();
    npin_t* select_reset_output = allocate_npin();
    nnet_t* new_net_rst = allocate_nnet();
    new_net_rst->name = make_full_ref_name(NULL, NULL, NULL, select_reset->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_reset, new_pin1_rst, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_rst, new_pin1_rst);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_rst, select_reset_output);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** en_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** rst_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** buf_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        en_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, en_muxes[i], 1);
        connect_nodes(not_select_enable, 0, en_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(en_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + ARST_width],
                              en_muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_en_mux = allocate_npin();
        npin_t* en_mux_output_pin = allocate_npin();
        nnet_t* new_net_en_mux = allocate_nnet();
        new_net_en_mux->name = make_full_ref_name(NULL, NULL, NULL, en_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(en_muxes[i], new_pin1_en_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_en_mux, new_pin1_en_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_en_mux, en_mux_output_pin);

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(rst_muxes[i],
                              en_mux_output_pin,
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(rst_muxes[i],
                              reset_value->pins[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_rst_mux = allocate_npin();
        npin_t* rst_mux_output_pin = allocate_npin();
        nnet_t* new_net_rst_mux = allocate_nnet();
        new_net_rst_mux->name = make_full_ref_name(NULL, NULL, NULL, rst_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(rst_muxes[i], new_pin1_rst_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_rst_mux, new_pin1_rst_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_rst_mux, rst_mux_output_pin);

        /*****************************************************************************************/
        /*************************************** BUF_NODE ****************************************/
        /*****************************************************************************************/

        buf_nodes[i] = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(buf_nodes[i],
                              rst_mux_output_pin,
                              0);

        /* connectiong the dffe output pin to the buf_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              buf_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(buf_nodes);
}
