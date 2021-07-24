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
    /* store reset sensitivity in rst pin */
    node->input_pins[0]->sensitivity = node->attributes->areset_polarity;
    /* keep record of reset pin */
    npin_t* select_reset = copy_input_npin(node->input_pins[0]);
    signal_list_t* areset_value = create_constant_signal(node->attributes->areset_value, D_width, netlist);

    /* merge clk and arst pins as a new ff clk */
    npin_t* FF_merged_clk_arst = merge_polarity(node->input_pins[ARST_width],      // CLK
                                                node->attributes->clk_edge_type,   // CLK_POLARITY
                                                node->input_pins[0],               // ARST
                                                node->attributes->areset_polarity, // ARST_POLARITY
                                                node,                              // NODE
                                                netlist);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /************************************** ARST_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* ARST_mux = smux_with_sel_polarity(node->input_pins[ARST_width + CLK_width + i], // D[i]
                                                   areset_value->pins[i],                        // reset value
                                                   copy_input_npin(select_reset),                // reset selector
                                                   node                                          // node
        );

        npin_t* ARST_mux_output_pin = ARST_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** FF_NODE *****************************************/
        /*****************************************************************************************/
        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);

        add_input_pin_to_node(FF_node, ARST_mux_output_pin, 0);

        /* hook new clk pin into FF node */
        add_input_pin_to_node(FF_node, copy_input_npin(FF_merged_clk_arst), 1);
        FF_node->attributes->clk_edge_type = FF_merged_clk_arst->sensitivity;

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
    free_signal_list(areset_value);
    free_nnode(node);
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

    /*****************************************************************************************/
    /*************************************** SRST_CHECK **************************************/
    /*****************************************************************************************/
    /* store reset sensitivity in rst pin */
    node->input_pins[CLK_width + D_width]->sensitivity = node->attributes->sreset_polarity;
    /* keep record of reset pin */
    npin_t* select_reset = copy_input_npin(node->input_pins[CLK_width + D_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width]);
    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** SRST_MUX ****************************************/
        /*****************************************************************************************/
        nnode_t* SRST_mux = smux_with_sel_polarity(node->input_pins[i + CLK_width], // D[i]
                                                   sreset_value->pins[i],           // sreset value
                                                   copy_input_npin(select_reset),   // sreset selector
                                                   node                             // node
        );

        npin_t* SRST_mux_output_pin = SRST_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_node, SRST_mux_output_pin, 0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        } else {
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);
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
    /* store enable sensitivity in enable pin */
    node->input_pins[CLK_width + D_width]->sensitivity = node->attributes->enable_polarity;
    /* keep record of enable pin */
    npin_t* select_enable = copy_input_npin(node->input_pins[CLK_width + D_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width]);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUXES ****************************************/
        /*****************************************************************************************/
        nnode_t* EN_mux = smux_with_sel_polarity(get_pad_pin(netlist),            // connect to pad while is not enable
                                                 node->input_pins[i + CLK_width], // D[i]
                                                 copy_input_npin(select_enable),  // enable selector
                                                 node                             // node
        );

        npin_t* EN_mux_output_pin = EN_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * (EN == EN_POLARITY) ? D : Q
         */
        add_input_pin_to_node(FF_node, EN_mux_output_pin, 0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        } else {
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
    free_nnode(node);
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
    /* store enable sensitivity in enable pin */
    node->input_pins[ARST_width + CLK_width + D_width]->sensitivity = node->attributes->enable_polarity;
    /* keep record of enable pin */
    npin_t* select_enable = copy_input_npin(node->input_pins[ARST_width + CLK_width + D_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[ARST_width + CLK_width + D_width]);

    /*****************************************************************************************/
    /*************************************** ARST_CHECK **************************************/
    /*****************************************************************************************/
    /* store areset sensitivity in areset pin */
    node->input_pins[0]->sensitivity = node->attributes->areset_polarity;
    /* keep record of reset pin */
    npin_t* select_reset = copy_input_npin(node->input_pins[0]);
    signal_list_t* areset_value = create_constant_signal(node->attributes->areset_value, D_width, netlist);

    /* merge clk and arst pins as a new ff clk */
    npin_t* FF_merged_clk_arst = merge_polarity(node->input_pins[ARST_width],      // CLK
                                                node->attributes->clk_edge_type,   // CLK_POLARITY
                                                node->input_pins[0],               // ARST
                                                node->attributes->areset_polarity, // ARST_POLARITY
                                                node,                              // NODE
                                                netlist);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* EN_mux = smux_with_sel_polarity(get_pad_pin(netlist),                         // pad while not enable
                                                 node->input_pins[i + ARST_width + CLK_width], // D[i]
                                                 copy_input_npin(select_enable),               // enable selector
                                                 node                                          // node
        );

        npin_t* EN_muxes_output_pin = EN_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* ARST_mux = smux_with_sel_polarity(EN_muxes_output_pin,           // enable mux output
                                                   areset_value->pins[i],         // areset value
                                                   copy_input_npin(select_reset), // areset selector
                                                   node                           // node
        );

        npin_t* ARST_muxes_output_pin = ARST_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** FF_NODE *****************************************/
        /*****************************************************************************************/
        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(FF_node, ARST_muxes_output_pin, 0);

        /* hook new clk pin into FF node */
        add_input_pin_to_node(FF_node, copy_input_npin(FF_merged_clk_arst), 1);
        FF_node->attributes->clk_edge_type = FF_merged_clk_arst->sensitivity;

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
    free_signal_list(areset_value);
    free_nnode(node);
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
    /* store reset sensitivity in rst pin */
    node->input_pins[CLK_width + D_width]->sensitivity = node->attributes->enable_polarity;
    /* keep record of enable pin */
    npin_t* select_enable = copy_input_npin(node->input_pins[CLK_width + D_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width]);

    /*****************************************************************************************/
    /*************************************** SRST_CHECK **************************************/
    /*****************************************************************************************/
    /* store reset sensitivity in rst pin */
    node->input_pins[CLK_width + D_width + EN_width]->sensitivity = node->attributes->sreset_polarity;
    /* keep record of reset pin */
    npin_t* select_reset = copy_input_npin(node->input_pins[CLK_width + D_width + EN_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width + EN_width]);
    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* EN_mux = smux_with_sel_polarity(get_pad_pin(netlist),            // pad while is not enable
                                                 node->input_pins[i + CLK_width], // D[i]
                                                 copy_input_npin(select_enable),  // enable selector
                                                 node                             // node
        );

        // specify mux_en[i] output pin
        npin_t* EN_muxes_output_pin = EN_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /************************************** SRST_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* SRST_mux = smux_with_sel_polarity(EN_muxes_output_pin,           // enable mux output
                                                   sreset_value->pins[i],         // sreset value
                                                   copy_input_npin(select_reset), // sreset selector
                                                   node                           // node
        );

        // specify mux_set[i] output pin
        npin_t* SRST_muxes_output_pin = SRST_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_node, SRST_muxes_output_pin, 0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        } else {
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }
    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);
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
    /* store sreset sensitivity in srst pin */
    node->input_pins[CLK_width + D_width + EN_width]->sensitivity = node->attributes->sreset_polarity;
    /* keep record of reset pin */
    npin_t* select_reset = copy_input_npin(node->input_pins[CLK_width + D_width + EN_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width + EN_width]);
    signal_list_t* sreset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* store enable sensitivity in enable pin */
    node->input_pins[CLK_width + D_width]->sensitivity = node->attributes->enable_polarity;
    /* keep record of enable pin */
    npin_t* select_enable = copy_input_npin(node->input_pins[CLK_width + D_width]);
    /* delete to avoid mem leaks */
    delete_npin(node->input_pins[CLK_width + D_width]);

    /* creating a internal nodes to initialize the value of D register */
    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /************************************** SRST_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* SRST_mux = smux_with_sel_polarity(node->input_pins[i + CLK_width], // D[i]
                                                   sreset_value->pins[i],           // sreset value
                                                   copy_input_npin(select_reset),   // sreset selector
                                                   node                             // node
        );

        // specify mux_set[i] output pin
        npin_t* SRST_muxes_output_pin = SRST_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        nnode_t* EN_mux = smux_with_sel_polarity(SRST_muxes_output_pin,          // pad node while is not enable
                                                 get_pad_pin(netlist),           // sreset mux output
                                                 copy_input_npin(select_enable), // enable selecor
                                                 node                            // node
        );

        // specify EN_muxes[i] output pin
        npin_t* EN_muxes_output_pin = EN_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** FF_NODEs ****************************************/
        /*****************************************************************************************/

        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(FF_node, EN_muxes_output_pin, 0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        } else {
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
    free_signal_list(sreset_value);
    free_nnode(node);
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
    int i;
    int width = node->num_output_pins;
    int CLK_width = node->input_port_sizes[0];
    int CLR_width = node->input_port_sizes[1];
    int D_width = node->input_port_sizes[2];

    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** SET_CHECK **************************************/
        /*****************************************************************************************/
        /* store set sensitivity in set pin */
        node->input_pins[CLK_width + CLR_width + D_width + i]->sensitivity = node->attributes->set_polarity;
        /* keep record of set pin */
        npin_t* select_set = copy_input_npin(node->input_pins[CLK_width + CLR_width + D_width + i]);
        /* delete to avoid mem leaks */
        delete_npin(node->input_pins[CLK_width + CLR_width + D_width + i]);

        /*****************************************************************************************/
        /**************************************** SET_MUXES **************************************/
        /*****************************************************************************************/
        nnode_t* SET_mux = smux_with_sel_polarity(node->input_pins[CLK_width + CLR_width + i], // D[i]
                                                  get_one_pin(netlist),                        // VCC (set value)
                                                  copy_input_npin(select_set),                 // set selector
                                                  node                                         // node
        );

        // specify SET_muxes[i] output pin
        npin_t* SET_muxes_output_pin = SET_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** CLR_CHECK ***************************************/
        /*****************************************************************************************/
        /* store reset sensitivity in rst pin */
        node->input_pins[CLK_width + i]->sensitivity = node->attributes->clr_polarity;
        /* keep record of clr pin */
        npin_t* select_clr = copy_input_npin(node->input_pins[CLK_width + i]);
        /* delete to avoid mem leaks */
        delete_npin(node->input_pins[CLK_width + i]);

        /*****************************************************************************************/
        /*************************************** CLR_MUXES ***************************************/
        /*****************************************************************************************/
        nnode_t* CLR_mux = smux_with_sel_polarity(SET_muxes_output_pin,        // set mux output
                                                  get_zero_pin(netlist),       // GND (clr value)
                                                  copy_input_npin(select_clr), // clr selector
                                                  node                         // node
        );

        // specify CLR_muxes[i] output pin
        npin_t* CLR_muxes_output_pin = CLR_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODES ***************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * remap D[i] from the dffsr node to the ff node 
         * since we do not need it in dff node anymore 
         **/
        add_input_pin_to_node(FF_node, CLR_muxes_output_pin, 0);

        /* remap the CLK pin from the dffsr node to the ff node */
        if (i == width - 1) {
            /* remap the CLK pin from the dffsr node to the new  */
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        }

        // remap node's output pin to CLR_muxes
        remap_pin_to_new_node(node->output_pins[i], FF_node, 0);
    }

    // CLEAN UP
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
    int i;
    int width = node->num_output_pins;
    int CLK_width = node->input_port_sizes[0];
    int CLR_width = node->input_port_sizes[1];
    int D_width = node->input_port_sizes[2];
    int EN_width = node->input_port_sizes[3];

    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** EN_CHECK ***************************************/
        /*****************************************************************************************/
        /* store enable sensitivity in en pin */
        node->input_pins[CLK_width + CLR_width + D_width + i]->sensitivity = node->attributes->enable_polarity;
        /* keep record of enable pin */
        npin_t* select_enable = copy_input_npin(node->input_pins[CLK_width + CLR_width + D_width + i]);
        /* delete to avoid mem leaks */
        delete_npin(node->input_pins[CLK_width + CLR_width + D_width + i]);

        /*****************************************************************************************/
        /*************************************** EN_MUXES ****************************************/
        /*****************************************************************************************/
        nnode_t* EN_mux = smux_with_sel_polarity(get_pad_pin(netlist),                        // pad while not enable
                                                 node->input_pins[CLK_width + CLR_width + i], // D[i]
                                                 copy_input_npin(select_enable),              // enable selector
                                                 node                                         // node
        );

        // specify EN_muxes[i] output pin
        npin_t* EN_muxes_output_pin = EN_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** SET_CHECK ***************************************/
        /*****************************************************************************************/
        /* store set sensitivity in set pin */
        node->input_pins[CLK_width + CLR_width + D_width + EN_width + i]->sensitivity = node->attributes->set_polarity;
        /* keep record of set pin */
        npin_t* select_set = copy_input_npin(node->input_pins[CLK_width + CLR_width + D_width + EN_width + i]);
        /* delete to avoid mem leaks */
        delete_npin(node->input_pins[CLK_width + CLR_width + D_width + EN_width + i]);

        /*****************************************************************************************/
        /*************************************** SET_MUXES ***************************************/
        /*****************************************************************************************/
        nnode_t* SET_mux = smux_with_sel_polarity(EN_muxes_output_pin,         // enable output
                                                  get_one_pin(netlist),        // VCC (set value)
                                                  copy_input_npin(select_set), // set selector
                                                  node                         // node
        );

        // specify SET_muxes[i] output pin
        npin_t* SET_muxes_output_pin = SET_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /*************************************** CLR_CHECK ***************************************/
        /*****************************************************************************************/
        /* store reset sensitivity in rst pin */
        node->input_pins[CLK_width + i]->sensitivity = node->attributes->clr_polarity;
        /* keep record of clr pin */
        npin_t* select_clr = copy_input_npin(node->input_pins[CLK_width + i]);
        /* delete to avoid mem leaks */
        delete_npin(node->input_pins[CLK_width + i]);

        /*****************************************************************************************/
        /*************************************** CLR_MUXES ***************************************/
        /*****************************************************************************************/
        nnode_t* CLR_mux = smux_with_sel_polarity(SET_muxes_output_pin,        // set mux output
                                                  get_zero_pin(netlist),       // GND (clr value)
                                                  copy_input_npin(select_clr), // clr selector
                                                  node                         // node
        );

        // specify mux_en[i] output pin
        npin_t* CLR_muxes_output_pin = CLR_mux->output_pins[0]->net->fanout_pins[0];

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        nnode_t* FF_node = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        FF_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remap D[i] from the dffsr node to the ff node */
        add_input_pin_to_node(FF_node, CLR_muxes_output_pin, 0);

        /**
         * remap the CLK pin from the dffsr node to the ff node
         * since we do not need it in dff node anymore 
         **/
        if (i == width - 1) {
            /* remap the CLK pin from the dffsr node to the new  */
            remap_pin_to_new_node(node->input_pins[0], FF_node, 1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(FF_node, copy_input_npin(node->input_pins[0]), 1);
        }

        // remap FF_nodes[i] output pin to dffsre output pin
        remap_pin_to_new_node(node->output_pins[i], FF_node, i);
    }

    // CLEAN UP
    free_nnode(node);
}
