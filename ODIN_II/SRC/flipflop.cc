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

#include "flipflop.hh"
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
    nnode_t* ff_node = make_1port_gate(FF_NODE, width + 1, width, node, traverse_mark_number);
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
        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;
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
    nnode_t** rst_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** D_ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** RST_ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** D_FF_NODE ***************************************/
        /*****************************************************************************************/

        D_ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        D_ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[ARST_width + CLK_width + i],
                              D_ff_nodes[i],
                              0);

        /* connecting the port 1: clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(D_ff_nodes[i],
                                  copy_input_npin(node->input_pins[ARST_width]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[ARST_width],
                                  D_ff_nodes[i],
                                  1);
        }

        /* Specifying the level_muxes outputs */
        // Connect output pin to related input pin
        npin_t* D_ff_node_out1 = allocate_npin();
        npin_t* D_ff_node_out2 = allocate_npin();
        nnet_t* D_ff_node_net = allocate_nnet();
        D_ff_node_net->name = make_full_ref_name(NULL, NULL, NULL, D_ff_nodes[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(D_ff_nodes[i], D_ff_node_out1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(D_ff_node_net, D_ff_node_out1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(D_ff_node_net, D_ff_node_out2);

        /*****************************************************************************************/
        /************************************** RST_FF_NODE **************************************/
        /*****************************************************************************************/

        RST_ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        RST_ff_nodes[i]->attributes->clk_edge_type = node->attributes->areset_polarity;

        /* connecting th reset value as the D of the RST_ff_node */
        add_input_pin_to_node(RST_ff_nodes[i],
                              reset_value->pins[i],
                              0);

        /* connecting the port 1: clk pin */
        add_input_pin_to_node(RST_ff_nodes[i],
                              copy_input_npin(select_reset->input_pins[0]),
                              1);

        /* Specifying the level_muxes outputs */
        // Connect output pin to related input pin
        npin_t* RST_ff_node_out1 = allocate_npin();
        npin_t* RST_ff_node_out2 = allocate_npin();
        nnet_t* RST_ff_node_net = allocate_nnet();
        RST_ff_node_net->name = make_full_ref_name(NULL, NULL, NULL, RST_ff_nodes[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(RST_ff_nodes[i], RST_ff_node_out1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(RST_ff_node_net, RST_ff_node_out1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(RST_ff_node_net, RST_ff_node_out2);

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* connecting the D_ff pin to 1: mux input */
        add_input_pin_to_node(rst_muxes[i],
                              D_ff_node_out2,
                              2);

        /* connecting the RST_ff pin to 0: mux input  */
        add_input_pin_to_node(rst_muxes[i],
                              RST_ff_node_out2,
                              3);

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              rst_muxes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(rst_muxes);
    vtr::free(D_ff_nodes);
    vtr::free(RST_ff_nodes);
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

    signal_list_t* reset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /*****************************************************************************************/
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1 = allocate_npin();
    npin_t* select_reset_output = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, select_reset->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_reset, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, select_reset_output);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** SRST_MUX ****************************************/
        /*****************************************************************************************/
        muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, muxes[i], 1);
        connect_nodes(not_select_reset, 0, muxes[i], 0);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              muxes[i],
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(muxes[i],
                              reset_value->pins[i],
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
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(muxes);
    vtr::free(ff_nodes);
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
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
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
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

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
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
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
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * (EN == EN_POLARITY) ? D : Q
         */
        add_input_pin_to_node(ff_nodes[i],
                              mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_nnode(node);

    vtr::free(muxes);
    vtr::free(ff_nodes);
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
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[ARST_width + CLK_width + D_width],
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
    nnode_t** D_ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** RST_ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

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
        remap_pin_to_new_node(node->input_pins[i + ARST_width + CLK_width],
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
        /*************************************** D_FF_NODE ***************************************/
        /*****************************************************************************************/

        D_ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        D_ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(D_ff_nodes[i],
                              en_mux_output_pin,
                              0);

        /* connecting the port 1: clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(D_ff_nodes[i],
                                  copy_input_npin(node->input_pins[ARST_width]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[ARST_width],
                                  D_ff_nodes[i],
                                  1);
        }

        /* Specifying the level_muxes outputs */
        // Connect output pin to related input pin
        npin_t* D_ff_node_out1 = allocate_npin();
        npin_t* D_ff_node_out2 = allocate_npin();
        nnet_t* D_ff_node_net = allocate_nnet();
        D_ff_node_net->name = make_full_ref_name(NULL, NULL, NULL, D_ff_nodes[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(D_ff_nodes[i], D_ff_node_out1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(D_ff_node_net, D_ff_node_out1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(D_ff_node_net, D_ff_node_out2);

        /*****************************************************************************************/
        /************************************** RST_FF_NODE **************************************/
        /*****************************************************************************************/

        RST_ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        RST_ff_nodes[i]->attributes->clk_edge_type = node->attributes->areset_polarity;

        /* connecting th reset value as the D of the RST_ff_node */
        add_input_pin_to_node(RST_ff_nodes[i],
                              reset_value->pins[i],
                              0);

        /* connecting the port 1: clk pin */
        add_input_pin_to_node(RST_ff_nodes[i],
                              copy_input_npin(select_reset->input_pins[0]),
                              1);

        /* Specifying the level_muxes outputs */
        // Connect output pin to related input pin
        npin_t* RST_ff_node_out1 = allocate_npin();
        npin_t* RST_ff_node_out2 = allocate_npin();
        nnet_t* RST_ff_node_net = allocate_nnet();
        RST_ff_node_net->name = make_full_ref_name(NULL, NULL, NULL, RST_ff_nodes[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(RST_ff_nodes[i], RST_ff_node_out1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(RST_ff_node_net, RST_ff_node_out1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(RST_ff_node_net, RST_ff_node_out2);

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* connecting the D_ff pin to 1: mux input */
        add_input_pin_to_node(rst_muxes[i],
                              D_ff_node_out2,
                              2);

        /* connecting the RST_ff pin to 0: mux input  */
        add_input_pin_to_node(rst_muxes[i],
                              RST_ff_node_out2,
                              3);

        /* connectiong the dffe output pin to the RST_ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              rst_muxes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(D_ff_nodes);
    vtr::free(RST_ff_nodes);
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
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
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
    signal_list_t* reset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width + EN_width],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
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
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

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
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
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
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              rst_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(ff_nodes);
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
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* reset_value = create_constant_signal(node->attributes->sreset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width + EN_width],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
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

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
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

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** en_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** rst_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              rst_muxes[i],
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
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        en_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, en_muxes[i], 1);
        connect_nodes(not_select_enable, 0, en_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(en_muxes[i],
                              rst_mux_output_pin,
                              2);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(en_muxes[i],
                              get_pad_pin(netlist),
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
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              en_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(ff_nodes);
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

    nnode_t** mux_set = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** mux_clr = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));

    int i;
    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** MUX_SET ****************************************/
        /*****************************************************************************************/
        mux_set[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_set = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // SET[i] === mux_set selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + i], select_set, 0);

        // make a not of selector
        nnode_t* not_select_set = make_not_gate(select_set, traverse_mark_number);
        connect_nodes(select_set, 0, not_select_set, 0);

        // connect mux_set selector based on the SET polarity
        if (node->attributes->set_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 1);
            connect_nodes(not_select_set, 0, mux_set[i], 0);
        } else if (node->attributes->set_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 0);
            connect_nodes(not_select_set, 0, mux_set[i], 1);
        }

        // remap D[i] to the mux_set[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + i],
                              mux_set[i],
                              2);
        // connect VCC to the mux_set[i]
        add_input_pin_to_node(mux_set[i],
                              get_one_pin(netlist),
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_set = allocate_npin();
        npin_t* new_pin2_set = allocate_npin();
        nnet_t* new_net_set = allocate_nnet();
        new_net_set->name = make_full_ref_name(NULL, NULL, NULL, mux_set[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_set[i], new_pin1_set, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_set, new_pin1_set);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_set, new_pin2_set);

        /*****************************************************************************************/
        /**************************************** MUX_CLR ****************************************/
        /*****************************************************************************************/
        mux_clr[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_clr = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // CLR[i] === mux_clr selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + i], select_clr, 0);

        // make a not of selector
        nnode_t* not_select_clr = make_not_gate(select_clr, traverse_mark_number);
        connect_nodes(select_clr, 0, not_select_clr, 0);

        // connect mux_clr selector based on the CLR polarity
        if (node->attributes->clr_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 1);
            connect_nodes(not_select_clr, 0, mux_clr[i], 0);
        } else if (node->attributes->clr_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 0);
            connect_nodes(not_select_clr, 0, mux_clr[i], 1);
        }

        // connect mux_set[i] output pin to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                              new_pin2_set,
                              2);
        // connect GND to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                              get_zero_pin(netlist),
                              3);
        // specify mux_clr[i] output pin
        npin_t* new_pin1_clr = allocate_npin();
        npin_t* new_pin2_clr = allocate_npin();
        nnet_t* new_net_clr = allocate_nnet();
        new_net_clr->name = make_full_ref_name(NULL, NULL, NULL, mux_clr[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_clr[i], new_pin1_clr, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_clr, new_pin1_clr);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_clr, new_pin2_clr);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        nnode_t* ff_node = allocate_nnode(node->loc);

        ff_node->type = FF_NODE;
        ff_node->traverse_visited = traverse_mark_number;
        ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        ff_node->name = node_name(ff_node, NULL);

        add_input_port_information(ff_node, 2);
        allocate_more_input_pins(ff_node, 2);

        add_output_port_information(ff_node, 1);
        allocate_more_output_pins(ff_node, 1);

        /**
         * remap the CLK pin from the dffsr node to the ff node
         * since we do not need it in dff node anymore 
         **/
        if (i == width - 1) {
            /**
             * remap the CLK pin from the dffsr node to the new  
             * ff node since we do not need it in dffsr node anymore 
             **/
            remap_pin_to_new_node(node->input_pins[0], ff_node, 1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(ff_node, copy_input_npin(node->input_pins[0]), 1);
        }

        /**
         * remap D[i] from the dffsr node to the ff node 
         * since we do not need it in dff node anymore 
         **/
        add_input_pin_to_node(ff_node, new_pin2_clr, 0);

        // remap node's output pin to mux_clr
        remap_pin_to_new_node(node->output_pins[i], ff_node, 0);

        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;
    }
    vtr::free(mux_set);
    vtr::free(mux_clr);
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
    nnode_t** mux_en = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** mux_set = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** mux_clr = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));

    int i;
    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** EN_CHECK ***************************************/
        /*****************************************************************************************/
        /* creating equal node to compare the value of enable */
        select_enable[i] = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
        /* connecting EN pin */
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + i],
                              select_enable[i],
                              0);

        /* to check the polarity and connect the correspondence signal */
        if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
            add_input_pin_to_node(select_enable[i],
                                  get_one_pin(netlist),
                                  1);
        } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
            add_input_pin_to_node(select_enable[i],
                                  get_zero_pin(netlist),
                                  1);
        }

        /* Specifying the level_muxes outputs */
        // Connect output pin to related input pin
        npin_t* new_pin1 = allocate_npin();
        npin_t* select_enable_output = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        new_net->name = make_full_ref_name(NULL, NULL, NULL, select_enable[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(select_enable[i], new_pin1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, select_enable_output);

        // make a not of selector
        nnode_t* not_select_enable = make_not_gate(select_enable[i], traverse_mark_number);
        connect_nodes(select_enable[i], 0, not_select_enable, 0);
        /*****************************************************************************************/
        /*************************************** VALUE_MUX ***************************************/
        /*****************************************************************************************/
        mux_en[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable[i], 0, mux_en[i], 1);
        connect_nodes(not_select_enable, 0, mux_en[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(mux_en[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + i],
                              mux_en[i],
                              3);

        // specify mux_en[i] output pin
        npin_t* new_pin1_en = allocate_npin();
        npin_t* new_pin2_en = allocate_npin();
        nnet_t* new_net_en = allocate_nnet();
        new_net_en->name = make_full_ref_name(NULL, NULL, NULL, mux_en[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_en[i], new_pin1_en, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_en, new_pin1_en);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_en, new_pin2_en);

        /*****************************************************************************************/
        /**************************************** MUX_SET ****************************************/
        /*****************************************************************************************/
        mux_set[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_set = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // SET[i] === mux_set selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + EN_width + i],
                              select_set,
                              0);

        // make a not of selector
        nnode_t* not_select_set = make_not_gate(select_set, traverse_mark_number);
        connect_nodes(select_set, 0, not_select_set, 0);

        // connect mux_set selector based on the SET polarity
        if (node->attributes->set_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 1);
            connect_nodes(not_select_set, 0, mux_set[i], 0);
        } else if (node->attributes->set_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 0);
            connect_nodes(not_select_set, 0, mux_set[i], 1);
        }

        // remap D[i] to the mux_set[i]
        add_input_pin_to_node(mux_set[i],
                              new_pin2_en,
                              2);
        // connect VCC to the mux_set[i]
        add_input_pin_to_node(mux_set[i],
                              get_one_pin(netlist),
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_set = allocate_npin();
        npin_t* new_pin2_set = allocate_npin();
        nnet_t* new_net_set = allocate_nnet();
        new_net_set->name = make_full_ref_name(NULL, NULL, NULL, mux_set[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_set[i], new_pin1_set, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_set, new_pin1_set);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_set, new_pin2_set);

        /*****************************************************************************************/
        /**************************************** MUX_CLR ****************************************/
        /*****************************************************************************************/
        mux_clr[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_clr = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // CLR[i] === mux_clr selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + i], select_clr, 0);

        // make a not of selector
        nnode_t* not_select_clr = make_not_gate(select_clr, traverse_mark_number);
        connect_nodes(select_clr, 0, not_select_clr, 0);

        // connect mux_clr selector based on the CLR polarity
        if (node->attributes->clr_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 1);
            connect_nodes(not_select_clr, 0, mux_clr[i], 0);
        } else if (node->attributes->clr_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 0);
            connect_nodes(not_select_clr, 0, mux_clr[i], 1);
        }

        // connect mux_set[i] output pin to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                              new_pin2_set,
                              2);
        // connect GND to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                              get_zero_pin(netlist),
                              3);
        // specify mux_clr[i] output pin
        npin_t* new_pin1_clr = allocate_npin();
        npin_t* new_pin2_clr = allocate_npin();
        nnet_t* new_net_clr = allocate_nnet();
        new_net_clr->name = make_full_ref_name(NULL, NULL, NULL, mux_clr[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_clr[i], new_pin1_clr, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_clr, new_pin1_clr);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_clr, new_pin2_clr);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        nnode_t* ff_node = allocate_nnode(node->loc);

        ff_node->type = FF_NODE;
        ff_node->traverse_visited = traverse_mark_number;
        ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        ff_node->name = node_name(ff_node, NULL);

        add_input_port_information(ff_node, 2);
        allocate_more_input_pins(ff_node, 2);

        add_output_port_information(ff_node, 1);
        allocate_more_output_pins(ff_node, 1);

        /**
         * remap the CLK pin from the dffsr node to the ff node
         * since we do not need it in dff node anymore 
         **/
        if (i == width - 1) {
            /**
             * remap the CLK pin from the dffsr node to the new  
             * ff node since we do not need it in dffsr node anymore 
             **/
            remap_pin_to_new_node(node->input_pins[0], ff_node, 1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(ff_node, copy_input_npin(node->input_pins[0]), 1);
        }

        /**
         * remap D[i] from the dffsr node to the ff node 
         * since we do not need it in dff node anymore 
         **/
        add_input_pin_to_node(ff_node, new_pin2_clr, 0);

        // remap node's output pin to mux_clr
        remap_pin_to_new_node(node->output_pins[i], ff_node, 0);

        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;
    }

    // CLEAN UP
    vtr::free(select_enable);
    vtr::free(mux_en);
    vtr::free(mux_set);
    vtr::free(mux_clr);
    free_nnode(node);
}
