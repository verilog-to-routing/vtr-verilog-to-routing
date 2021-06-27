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
 */

#include "FlipFlop.hh"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: resolve_dff_node)
 * 
 * @brief split the dff node read from yosys blif to
 * FF nodes with input/output width one
 * 
 * @param node pointing to the dff node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_dff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_output_pins + 1 == node->num_input_pins);

    /**
     * need to reformat the order of pins and the number of ports
     * since odin ff_node has only one port which the last pin is clk
    */
    int i;
    int width = node->num_output_pins;
    nnode_t* ff_node = make_2port_gate(FF_NODE, width, 1, width, node, traverse_mark_number);
    ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;
    /* remap the last bit of the clk pin to the last input pin since the resit does not work at all */
    int clk_width = node->input_port_sizes[0];
    remap_pin_to_new_node(node->input_pins[0], ff_node, width);

    /* remapping the D inputs to [1..n] */
    for (i = 0; i < width; i++) {
        remap_pin_to_new_node(node->input_pins[clk_width + i], ff_node, i);
    }

    /* hook the output pins into the new ff_node */
    for (i = 0; i < width; i++) {
        remap_pin_to_new_node(node->output_pins[i], ff_node, i);
    }

    /** if it is multibit we will leave it for partial mapping phase,
     * otherwise we need to add single bit ff node to the ff_node 
     * list of the current netlist 
    */
    /* single bit dff, [0]: clk, [1]: D */
    if (node->num_input_pins == 2) {
        /* adding the new generated node to the ff node list of the enetlist */
        add_node_to_netlist(netlist, ff_node, FF_NODE);
    }

    // CLEAN UP
    free_nnode(node);
}

/**
 * (function: resolve_adff_node)
 * 
 * @brief resolving the dff node by connecting 
 * the multiplexing the D input with reset value
 * 
 * @param node pointing to a dff node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_adff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the arst input is 1 bit */
    oassert(node->input_port_sizes[1] == 1);

    /**
     * ARST: input port 0
     * CLK:  input port 1
     * D:    input port 2
     * Q:    output port 0
    */
    int i;
    int ARST_width = node->input_port_sizes[0]; // == 1
    int CLK_width = node->input_port_sizes[1];  // == 1
    int D_width = node->input_port_sizes[2];

    /*****************************************************************************************/
    /*************************************** ARST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* areset_value = create_constant_signal(node->attributes->areset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting ARST pin */
    remap_pin_to_new_node(node->input_pins[0],
                          select_reset,
                          0);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** ARST_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** FF_NODE *****************************************/
        /*****************************************************************************************/
        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[ARST_width + CLK_width + i],
                              FF_nodes[i],
                              0);

        if (i == D_width -1) {
            /* connecting the port 1: clk pin */
            remap_pin_to_new_node(node->input_pins[ARST_width],
                                  FF_nodes[i],
                                  1);
        } else {
            /* connecting the port 1: clk pin */
            add_input_pin_to_node(FF_nodes[i],
                                copy_input_npin(node->input_pins[ARST_width]),
                                1);
        }

        /* Specifying the level_muxes outputs */
        signal_list_t* FF_output = make_output_pins_for_existing_node(FF_nodes[i], 1);
        npin_t* FF_output_pin = FF_output->pins[0];

        /*****************************************************************************************/
        /************************************** ARST_MUX *****************************************/
        /*****************************************************************************************/
        ARST_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect mux_set selector based on the SET polarity
        if (node->attributes->areset_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_reset, 0, ARST_muxes[i], 1);
            connect_nodes(not_select_reset, 0, ARST_muxes[i], 0);

        } else if (node->attributes->areset_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_reset, 0, ARST_muxes[i], 0);
            connect_nodes(not_select_reset, 0, ARST_muxes[i], 1);
        }

        /* connecting the D_ff pin to 1: mux input */
        add_input_pin_to_node(ARST_muxes[i],
                              FF_output_pin,
                              2);

        /* connecting th reset value as the D of the RST_ff_node */
        add_input_pin_to_node(ARST_muxes[i],
                              areset_value->pins[i],
                              3);

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ARST_muxes[i],
                              0);

        // CLEAN UP
        free_signal_list(FF_output);
    }

    // CLEAN UP
    free_signal_list(areset_value);
    free_nnode(node);

    vtr::free(ARST_muxes);
    vtr::free(FF_nodes);
}

/**
 * (function: resolve_sdff_node)
 * 
 * @brief resolving the sdff node by connecting 
 * the multiplexing the D input with reset value
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_sdff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the srst input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK:  input port 0
     * D:    input port 1
     * SRST: input port 2
     * Q:    output port 3
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];

    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /*****************************************************************************************/
    /*************************************** SRST_CHECK **************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_reset,
                          0);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** SRST_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** SRST_MUX ****************************************/
        /*****************************************************************************************/
        SRST_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect mux rst selector based on the SRST polarity
        if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_reset, 0, SRST_muxes[i], 1);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 0);

        } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_reset, 0, SRST_muxes[i], 0);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 1);
        }

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              SRST_muxes[i],
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(SRST_muxes[i],
                              sreset_value->pins[i],
                              3);

        // specify mux_set[i] output pin
        signal_list_t* SRST_mux_outputs = make_output_pins_for_existing_node(SRST_muxes[i], 1);
        npin_t* SRST_mux_output_pin = SRST_mux_outputs->pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_nodes[i],
                              SRST_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              FF_nodes[i],
                              0);
        // CLEAN UP
        free_signal_list(SRST_mux_outputs);
    }

    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);

    vtr::free(SRST_muxes);
    vtr::free(FF_nodes);
}

/**
 * (function: resolve_dffe_node)
 * 
 * @brief resolving the dffe node by connecting 
 * multiplexed D input with Q as the output 
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_dffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the enable input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK: input port 0
     * D:   input port 1
     * EN:  input port 2
     * Q:   output port 3
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_enable,
                          0);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** EN_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUXES ****************************************/
        /*****************************************************************************************/
        EN_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 1);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 0);

        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 0);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 1);
        }

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              EN_muxes[i],
                              3);

        // specify mux_set[i] output pin
        signal_list_t* EN_mux_outputs = make_output_pins_for_existing_node(EN_muxes[i], 1);
        npin_t* EN_mux_output_pin = EN_mux_outputs->pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * (EN == EN_POLARITY) ? D : Q
         */
        add_input_pin_to_node(FF_nodes[i],
                              EN_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              FF_nodes[i],
                              0);
        // CLEAN UP
        free_signal_list(EN_mux_outputs);
    }

    // CLEAN UP
    free_nnode(node);

    vtr::free(EN_muxes);
    vtr::free(FF_nodes);
}

/**
 * (function: resolve_adffe_node)
 * 
 * @brief resolving the adffe node by connecting 
 * the multiplexing the D input with Q ff_node output 
 * and enabling reset value 
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_adffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the arst input is 1 bit */
    oassert(node->input_port_sizes[1] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[3] == 1);

    /**
     * ARST: input port 0
     * CLK:  input port 1
     * D:    input port 2
     * EN:   input port 3
     * Q:    output port 0
    */
    int i;
    int ARST_width = node->input_port_sizes[0]; // == 1
    int CLK_width = node->input_port_sizes[1];  // == 1
    int D_width = node->input_port_sizes[2];

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[ARST_width + CLK_width + D_width],
                          select_enable,
                          0);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /*****************************************************************************************/
    /*************************************** ARST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* areset_value = create_constant_signal(node->attributes->areset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting ARST pin */
    remap_pin_to_new_node(node->input_pins[0],
                          select_reset,
                          0);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);


    /* creating a internal nodes to initialize the value of D register */
    nnode_t** EN_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ARST_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        EN_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 1);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 0);

        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 0);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 1);
        }

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + ARST_width + CLK_width],
                              EN_muxes[i],
                              3);

        // specify mux_set[i] output pin
        signal_list_t* EN_muxes_outputs = make_output_pins_for_existing_node(EN_muxes[i], 1);
        npin_t* EN_muxes_output_pin = EN_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /*************************************** FF_NODE *****************************************/
        /*****************************************************************************************/
        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(FF_nodes[i],
                              EN_muxes_output_pin,
                              0);

        /* connecting the port 1: clk pin */
        if (i == D_width - 1) {
            remap_pin_to_new_node(node->input_pins[ARST_width],
                                  FF_nodes[i],
                                  1);
        } else {   
            add_input_pin_to_node(FF_nodes[i],
                                    copy_input_npin(node->input_pins[ARST_width]),
                                    1);
        }

        /* Specifying the level_muxes outputs */
        signal_list_t* FF_nodes_outputs = make_output_pins_for_existing_node(FF_nodes[i], 1);
        npin_t* FF_nodes_output_pin = FF_nodes_outputs->pins[0];        

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        ARST_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->areset_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, ARST_muxes[i], 1);
            connect_nodes(not_select_reset, 0, ARST_muxes[i], 0);

        } else if (node->attributes->areset_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, ARST_muxes[i], 0);
            connect_nodes(not_select_reset, 0, ARST_muxes[i], 1);
        }

        /* connecting the D_ff pin to 1: mux input */
        add_input_pin_to_node(ARST_muxes[i],
                              FF_nodes_output_pin,
                              2);

        /* connecting the RST_ff pin to 0: mux input  */
        add_input_pin_to_node(ARST_muxes[i],
                              areset_value->pins[i],
                              3);

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ARST_muxes[i],
                              0);

        // CLEAN UP
        free_signal_list(EN_muxes_outputs);
        free_signal_list(FF_nodes_outputs);
    }

    // CLEAN UP
    free_signal_list(areset_value);
    free_nnode(node);

    vtr::free(EN_muxes);
    vtr::free(ARST_muxes);
    vtr::free(FF_nodes);
}

/**
 * (function: resolve_sdffe_node)
 * 
 * @brief resolving the sdffe node by multiplexing the D input with pad
 * using enable as selector and then multiplexing the result with reset value 
 * 
 * @param node pointing to a sdffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_sdffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the srst input is 1 bit */
    oassert(node->input_port_sizes[3] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK:  input port 0
     * D:    input port 1
     * EN:   input port 2
     * SRST: input port 3
     * Q:    output port 0
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];
    int EN_width = node->input_port_sizes[2]; // == 1

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_enable,
                          0);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /*****************************************************************************************/
    /*************************************** SRST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width + EN_width],
                          select_reset,
                          0);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** EN_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** SRST_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        EN_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 1);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 0);

        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 0);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 1);
        }

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              EN_muxes[i],
                              3);

        // specify mux_en[i] output pin
        signal_list_t* EN_muxes_outputs = make_output_pins_for_existing_node(EN_muxes[i], 1);
        npin_t* EN_muxes_output_pin = EN_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /************************************** SRST_MUX *****************************************/
        /*****************************************************************************************/
        SRST_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, SRST_muxes[i], 1);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 0);

        } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, SRST_muxes[i], 0);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 1);
        }

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(SRST_muxes[i],
                              EN_muxes_output_pin,
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(SRST_muxes[i],
                              sreset_value->pins[i],
                              3);

        // specify mux_set[i] output pin
        signal_list_t* SRST_muxes_outputs = make_output_pins_for_existing_node(SRST_muxes[i], 1);
        npin_t* SRST_muxes_output_pin = SRST_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_nodes[i],
                              SRST_muxes_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              FF_nodes[i],
                              0);

        // CLEAN UP
        free_signal_list(EN_muxes_outputs);
        free_signal_list(SRST_muxes_outputs);
    }
    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);

    vtr::free(EN_muxes);
    vtr::free(SRST_muxes);
    vtr::free(FF_nodes);
}

/**
 * (function: resolve_sdffce_node)
 * 
 * @brief resolving the sdffce node by multiplexing the D input with
 * reset value and multiplexing the result with pad using enable as selector
 * 
 * @param node pointing to a sdffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_sdffce_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the srst input is 1 bit */
    oassert(node->input_port_sizes[3] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK:  input port 0
     * D:    input port 1
     * EN:   input port 2
     * SRST: input port 3
     * Q:    output port 0
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];
    int EN_width = node->input_port_sizes[2]; // == 1

    /*****************************************************************************************/
    /*************************************** SRST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width + EN_width],
                          select_reset,
                          0);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_enable,
                          0);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);


    /* creating a internal nodes to initialize the value of D register */
    nnode_t** EN_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** SRST_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /************************************** SRST_MUX *****************************************/
        /*****************************************************************************************/
        SRST_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, SRST_muxes[i], 1);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 0);

        } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_reset, 0, SRST_muxes[i], 0);
            connect_nodes(not_select_reset, 0, SRST_muxes[i], 1);
        }

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              SRST_muxes[i],
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(SRST_muxes[i],
                              sreset_value->pins[i],
                              3);

        // specify mux_set[i] output pin
        signal_list_t* SRST_muxes_outputs = make_output_pins_for_existing_node(SRST_muxes[i], 1);
        npin_t* SRST_muxes_output_pin = SRST_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        EN_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 1);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 0);

        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable, 0, EN_muxes[i], 0);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 1);
        }

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              SRST_muxes_output_pin,
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              get_pad_pin(netlist),
                              3);

        // specify EN_muxes[i] output pin
        signal_list_t* EN_muxes_outputs = make_output_pins_for_existing_node(EN_muxes[i], 1);
        npin_t* EN_muxes_output_pin = EN_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /*************************************** FF_NODEs ****************************************/
        /*****************************************************************************************/

        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_nodes[i],
                              EN_muxes_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              FF_nodes[i],
                              0);
                              
        // CLEAN UP
        free_signal_list(SRST_muxes_outputs);
        free_signal_list(EN_muxes_outputs);
    }

    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);

    vtr::free(EN_muxes);
    vtr::free(SRST_muxes);
    vtr::free(FF_nodes);
}


/**
 * (function: resolve_dffsr_node)
 * 
 * @brief resolving the dffsr node by connecting 
 * the ouput pins[1..n] to GND/VCC/D based on 
 * the clr/set edge 
 * 
 * @param node pointing to a dffsr node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_dffsr_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /**
     * CLK: input port 0
     * CLR: input port 1
     * D:   input port 2
     * SET: input port 3
    */
    int width = node->num_output_pins;
    int CLK_width = node->input_port_sizes[0];
    int CLR_width = node->input_port_sizes[1];
    int D_width = node->input_port_sizes[2];

    nnode_t** FF_nodes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** SET_muxes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** CLR_muxes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));

    int i;
    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** SET_CHECK **************************************/
        /*****************************************************************************************/
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_set = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // SET[i] === SET_muxes selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + i], select_set, 0);

        // make a not of selector
        nnode_t* not_select_set = make_not_gate(select_set, traverse_mark_number);
        connect_nodes(select_set, 0, not_select_set, 0);

        /*****************************************************************************************/
        /**************************************** SET_MUXES **************************************/
        /*****************************************************************************************/
        SET_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect SET_muxes selector based on the SET polarity
        if (node->attributes->set_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_set, 0, SET_muxes[i], 1);
            connect_nodes(not_select_set, 0, SET_muxes[i], 0);

        } else if (node->attributes->set_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_set, 0, SET_muxes[i], 0);
            connect_nodes(not_select_set, 0, SET_muxes[i], 1);
        }

        // remap D[i] to the SET_muxes[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + i],
                              SET_muxes[i],
                              2);

        // connect VCC to the SET_muxes[i]
        add_input_pin_to_node(SET_muxes[i],
                              get_one_pin(netlist),
                              3);

        // specify SET_muxes[i] output pin
        signal_list_t* SET_muxes_outputs = make_output_pins_for_existing_node(SET_muxes[i], 1);
        npin_t* SET_muxes_output_pin = SET_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /*************************************** CLR_CHECK ***************************************/
        /*****************************************************************************************/
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_clr = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // CLR[i] === CLR_muxes selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + i], select_clr, 0);

        // make a not of selector
        nnode_t* not_select_clr = make_not_gate(select_clr, traverse_mark_number);
        connect_nodes(select_clr, 0, not_select_clr, 0);

        /*****************************************************************************************/
        /*************************************** CLR_MUXES ***************************************/
        /*****************************************************************************************/
        CLR_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect CLR_muxes selector based on the CLR polarity
        if (node->attributes->clr_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_clr, 0, CLR_muxes[i], 1);
            connect_nodes(not_select_clr, 0, CLR_muxes[i], 0);

        } else if (node->attributes->clr_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_clr, 0, CLR_muxes[i], 0);
            connect_nodes(not_select_clr, 0, CLR_muxes[i], 1);
        }

        // connect SET_muxes[i] output pin to the CLR_muxes[i]
        add_input_pin_to_node(CLR_muxes[i],
                              SET_muxes_output_pin,
                              2);

        // connect GND to the CLR_muxes[i]
        add_input_pin_to_node(CLR_muxes[i],
                              get_zero_pin(netlist),
                              3);

        // specify CLR_muxes[i] output pin
        signal_list_t* CLR_muxes_outputs = make_output_pins_for_existing_node(CLR_muxes[i], 1);
        npin_t* CLR_muxes_output_pin = CLR_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODES ***************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * remap D[i] from the dffsr node to the ff node 
         * since we do not need it in dff node anymore 
         **/
        add_input_pin_to_node(FF_nodes[i],
                              CLR_muxes_output_pin,
                              0);

        /* remap the CLK pin from the dffsr node to the ff node */
        if (i == width - 1) {
            /* remap the CLK pin from the dffsr node to the new  */
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        }

        // remap node's output pin to CLR_muxes
        remap_pin_to_new_node(node->output_pins[i],
                              FF_nodes[i],
                              0);

        // CLEAN UP
        free_signal_list(SET_muxes_outputs);
        free_signal_list(CLR_muxes_outputs);
    }

    // CLEAN UP
    vtr::free(FF_nodes);
    vtr::free(SET_muxes);
    vtr::free(CLR_muxes);
    free_nnode(node);
}

/**
 * (function: resolve_dffsre_node)
 * 
 * @brief resolving the dffsr node by connecting 
 * the ouput pins[1..n] to GND/VCC/D based on 
 * the clr/set edge and adding enable mux
 * 
 * @param node pointing to a dffsre node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_dffsre_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 5);
    oassert(node->num_output_port_sizes == 1);

    /**
     * CLK: input port 0
     * CLR: input port 1
     * D:   input port 2
     * EN:  input port 3
     * Q:   output port 0
     * SET: input port 4
    */
    int width = node->num_output_pins;
    int CLK_width = node->input_port_sizes[0];
    int CLR_width = node->input_port_sizes[1];
    int D_width = node->input_port_sizes[2];
    int EN_width = node->input_port_sizes[3];

    nnode_t** select_enable = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** FF_nodes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** EN_muxes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** SET_muxes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** CLR_muxes = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));

    int i;
    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        FF_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remap D[i] from the dffsr node to the ff node */
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + i],
                              FF_nodes[i],
                              0);

        /**
         * remap the CLK pin from the dffsr node to the ff node
         * since we do not need it in dff node anymore 
         **/
        if (i == width - 1) {
            /* remap the CLK pin from the dffsr node to the new  */
            remap_pin_to_new_node(node->input_pins[0],
                                  FF_nodes[i],
                                  1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(FF_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        }

        // specify mux_en[i] output pin
        signal_list_t* FF_nodes_outputs = make_output_pins_for_existing_node(FF_nodes[i], 1);
        npin_t* FF_nodes_output_pin = FF_nodes_outputs->pins[0];

        /*****************************************************************************************/
        /**************************************** EN_CHECK ***************************************/
        /*****************************************************************************************/
        /* creating equal node to compare the value of enable */
        select_enable[i] = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        /* connecting EN pin */
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + i],
                              select_enable[i],
                              0);

        // make a not of selector
        nnode_t* not_select_enable = make_not_gate(select_enable[i], traverse_mark_number);
        connect_nodes(select_enable[i], 0, not_select_enable, 0);

        /*****************************************************************************************/
        /*************************************** EN_MUXES ****************************************/
        /*****************************************************************************************/
        EN_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable[i], 0, EN_muxes[i], 1);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 0);

        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            /* connecting selectors */
            connect_nodes(select_enable[i], 0, EN_muxes[i], 0);
            connect_nodes(not_select_enable, 0, EN_muxes[i], 1);
        }

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(EN_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(EN_muxes[i],
                              FF_nodes_output_pin,
                              3);

        // specify EN_muxes[i] output pin
        signal_list_t* EN_muxes_outputs = make_output_pins_for_existing_node(EN_muxes[i], 1);
        npin_t* EN_muxes_output_pin = EN_muxes_outputs->pins[0];
        

        /*****************************************************************************************/
        /*************************************** SET_CHECK ***************************************/
        /*****************************************************************************************/
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_set = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // SET[i] === mux_set selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + EN_width + i],
                              select_set,
                              0);

        // make a not of selector
        nnode_t* not_select_set = make_not_gate(select_set, traverse_mark_number);
        connect_nodes(select_set, 0, not_select_set, 0);

        /*****************************************************************************************/
        /*************************************** SET_MUXES ***************************************/
        /*****************************************************************************************/
        SET_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect SET_muxes selector based on the SET polarity
        if (node->attributes->set_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_set, 0, SET_muxes[i], 1);
            connect_nodes(not_select_set, 0, SET_muxes[i], 0);

        } else if (node->attributes->set_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_set, 0, SET_muxes[i], 0);
            connect_nodes(not_select_set, 0, SET_muxes[i], 1);
        }

        // add en_mux output pin to the SET_muxes[i]
        add_input_pin_to_node(SET_muxes[i],
                              EN_muxes_output_pin,
                              2);

        // connect VCC to the SET_muxes[i]
        add_input_pin_to_node(SET_muxes[i],
                              get_one_pin(netlist),
                              3);

        // specify SET_muxes[i] output pin
        signal_list_t* SET_muxes_outputs = make_output_pins_for_existing_node(SET_muxes[i], 1);
        npin_t* SET_muxes_output_pin = SET_muxes_outputs->pins[0];

        /*****************************************************************************************/
        /*************************************** CLR_CHECK ***************************************/
        /*****************************************************************************************/
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_clr = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // CLR[i] === mux_clr selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + i], select_clr, 0);

        // make a not of selector
        nnode_t* not_select_clr = make_not_gate(select_clr, traverse_mark_number);
        connect_nodes(select_clr, 0, not_select_clr, 0);

        /*****************************************************************************************/
        /*************************************** CLR_MUXES ***************************************/
        /*****************************************************************************************/
        CLR_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect mux_clr selector based on the CLR polarity
        if (node->attributes->clr_polarity == ACTIVE_HIGH_SENSITIVITY) {
            connect_nodes(select_clr, 0, CLR_muxes[i], 1);
            connect_nodes(not_select_clr, 0, CLR_muxes[i], 0);

        } else if (node->attributes->clr_polarity == ACTIVE_LOW_SENSITIVITY) {
            connect_nodes(select_clr, 0, CLR_muxes[i], 0);
            connect_nodes(not_select_clr, 0, CLR_muxes[i], 1);
        }

        // connect mux_set[i] output pin to the CLR_muxes[i]
        add_input_pin_to_node(CLR_muxes[i],
                              SET_muxes_output_pin,
                              2);

        // connect GND to the CLR_muxes[i]
        add_input_pin_to_node(CLR_muxes[i],
                              get_zero_pin(netlist),
                              3);

        // remap CLR_muxes[i] output pin to dffsre output pin
        remap_pin_to_new_node(node->output_pins[i],
                              CLR_muxes[i],
                              i);

        

        // CLEAN UP
        free_signal_list(FF_nodes_outputs);
        free_signal_list(EN_muxes_outputs);
        free_signal_list(SET_muxes_outputs);
    }

    // CLEAN UP
    vtr::free(select_enable);
    vtr::free(FF_nodes);
    vtr::free(EN_muxes);
    vtr::free(SET_muxes);
    vtr::free(CLR_muxes);
    free_nnode(node);
}
