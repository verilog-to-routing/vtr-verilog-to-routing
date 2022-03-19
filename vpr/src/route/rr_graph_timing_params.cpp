#include <cstdio>

#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_timing_params.h"

/****************** Subroutine definitions *********************************/

void add_rr_graph_C_from_switches(float C_ipin_cblock) {
    /* This routine finishes loading the C elements of the rr_graph. It assumes *
     * that when you call it the CHANX and CHANY nodes have had their C set to  *
     * their metal capacitance, and everything else has C set to 0.  The graph  *
     * connectivity (edges, switch types etc.) must all be loaded too.  This    *
     * routine will add in the capacitance on the CHANX and CHANY nodes due to: *
     *                                                                          *
     * 1) The output capacitance of the switches coming from OPINs;             *
     * 2) The input and output capacitance of the switches between the various  *
     *    wiring (CHANX and CHANY) segments; and                                *
     * 3) The input capacitance of the input connection block (or buffers       *
     *    separating tracks from the input connection block, if enabled by      *
     *    INCLUDE_TRACK_BUFFERS)                                    	    */

    int switch_index, maxlen;
    size_t to_node;
    int icblock, isblock, iseg_low, iseg_high;
    float Cin, Cout;
    t_rr_type from_rr_type, to_rr_type;
    bool* cblock_counted; /* [0..maxlen-1] -- 0th element unused. */
    float* buffer_Cin;    /* [0..maxlen-1] */
    bool buffered;
    float* Couts_to_add; /* UDSD */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();

    maxlen = std::max(device_ctx.grid.width(), device_ctx.grid.height());
    cblock_counted = (bool*)vtr::calloc(maxlen, sizeof(bool));
    buffer_Cin = (float*)vtr::calloc(maxlen, sizeof(float));

    std::vector<float> rr_node_C(rr_graph.num_nodes(), 0.); //Stores the final C

    for (const RRNodeId& rr_id : rr_graph.nodes()) {
        size_t inode = (size_t)rr_id;
        //The C may have already been partly initialized (e.g. with metal capacitance)
        rr_node_C[inode] += rr_graph.node_C(rr_id);

        from_rr_type = rr_graph.node_type(rr_id);

        if (from_rr_type == CHANX || from_rr_type == CHANY) {
            for (t_edge_size iedge = 0; iedge < rr_graph.num_edges(rr_id); iedge++) {
                to_node = size_t(rr_graph.edge_sink_node(rr_id, iedge));
                to_rr_type = rr_graph.node_type(RRNodeId(to_node));

                if (to_rr_type == CHANX || to_rr_type == CHANY) {
                    switch_index = rr_graph.edge_switch(rr_id, iedge);
                    Cin = rr_graph.rr_switch_inf(RRSwitchId(switch_index)).Cin;
                    Cout = rr_graph.rr_switch_inf(RRSwitchId(switch_index)).Cout;
                    buffered = rr_graph.rr_switch_inf(RRSwitchId(switch_index)).buffered();

                    /* If both the switch from inode to to_node and the switch from *
                     * to_node back to inode use bidirectional switches (i.e. pass  *
                     * transistors), there will only be one physical switch for     *
                     * both edges.  Hence, I only want to count the capacitance of  *
                     * that switch for one of the two edges.  (Note:  if there is   *
                     * a pass transistor edge from x to y, I always build the graph *
                     * so that there is a corresponding edge using the same switch  *
                     * type from y to x.) So, I arbitrarily choose to add in the    *
                     * capacitance in that case of a pass transistor only when      *
                     * processing the lower inode number.                           *
                     * If an edge uses a buffer I always have to add in the output  *
                     * capacitance.  I assume that buffers are shared at the same   *
                     * (i,j) location, so only one input capacitance needs to be    *
                     * added for all the buffered switches at that location.  If    *
                     * the buffers at that location have different sizes, I use the *
                     * input capacitance of the largest one.                        */

                    if (!buffered && inode < to_node) { /* Pass transistor. */
                        rr_node_C[inode] += Cin;
                        rr_node_C[to_node] += Cout;
                    }

                    else if (buffered) {
                        /* Prevent double counting of capacitance for UDSD */
                        if (rr_graph.node_direction(RRNodeId(to_node)) == Direction::BIDIR) {
                            /* For multiple-driver architectures the output capacitance can
                             * be added now since each edge is actually a driver */
                            rr_node_C[to_node] += Cout;
                        }
                        isblock = seg_index_of_sblock(inode, to_node);
                        buffer_Cin[isblock] = std::max(buffer_Cin[isblock], Cin);
                    }

                }
                /* End edge to CHANX or CHANY node. */
                else if (to_rr_type == IPIN) {
                    if (INCLUDE_TRACK_BUFFERS) {
                        /* Implements sharing of the track to connection box buffer.
                         * Such a buffer exists at every segment of the wire at which
                         * at least one logic block input connects. */
                        icblock = seg_index_of_cblock(from_rr_type, to_node);
                        if (cblock_counted[icblock] == false) {
                            rr_node_C[inode] += C_ipin_cblock;
                            cblock_counted[icblock] = true;
                        }
                    } else {
                        /* No track buffer. Simply add the capacitance onto the wire */
                        rr_node_C[inode] += C_ipin_cblock;
                    }
                }
            } /* End loop over all edges of a node. */

            /* Reset the cblock_counted and buffer_Cin arrays, and add buf Cin. */

            /* Method below would be faster for very unpopulated segments, but I  *
             * think it would be slower overall for most FPGAs, so commented out. */

            /*   for (iedge=0;iedge<device_ctx.rr_nodes[inode].num_edges();iedge++) {
             * to_node = device_ctx.rr_nodes[inode].edges[iedge];
             * if (rr_graph.node_type(RRNodeId(to_node)) == IPIN) {
             * icblock = seg_index_of_cblock (from_rr_type, to_node);
             * cblock_counted[icblock] = false;
             * }
             * }     */

            if (from_rr_type == CHANX) {
                iseg_low = rr_graph.node_xlow(rr_id);
                iseg_high = rr_graph.node_xhigh(rr_id);
            } else { /* CHANY */
                iseg_low = rr_graph.node_ylow(rr_id);
                iseg_high = rr_graph.node_yhigh(rr_id);
            }

            for (icblock = iseg_low; icblock <= iseg_high; icblock++) {
                cblock_counted[icblock] = false;
            }

            for (isblock = iseg_low - 1; isblock <= iseg_high; isblock++) {
                rr_node_C[inode] += buffer_Cin[isblock]; /* Biggest buf Cin at loc */
                buffer_Cin[isblock] = 0.;
            }

        }
        /* End node is CHANX or CHANY */
        else if (from_rr_type == OPIN) {
            for (t_edge_size iedge = 0; iedge < rr_graph.num_edges(rr_id); iedge++) {
                switch_index = rr_graph.edge_switch(rr_id, iedge);
                to_node = size_t(rr_graph.edge_sink_node(rr_id, iedge));
                to_rr_type = rr_graph.node_type(RRNodeId(to_node));

                if (to_rr_type != CHANX && to_rr_type != CHANY)
                    continue;

                if (rr_graph.node_direction(RRNodeId(to_node)) == Direction::BIDIR) {
                    Cout = rr_graph.rr_switch_inf(RRSwitchId(switch_index)).Cout;
                    to_node = size_t(rr_graph.edge_sink_node(rr_id, iedge)); /* Will be CHANX or CHANY */
                    rr_node_C[to_node] += Cout;
                }
            }
        }
        /* End node is OPIN. */
    } /* End for all nodes. */

    /* Now we need to add any Cout loads for nets that we previously didn't process
     * Current structures only keep switch information from a node to the next node and
     * not the reverse.  Therefore I need to go through all the possible edges to figure
     * out what the Cout's should be */
    Couts_to_add = (float*)vtr::calloc(rr_graph.num_nodes(), sizeof(float));
    for (const RRNodeId& inode : rr_graph.nodes()) {
        for (t_edge_size iedge = 0; iedge < rr_graph.num_edges(inode); iedge++) {
            switch_index = rr_graph.edge_switch(inode, iedge);
            to_node = size_t(rr_graph.edge_sink_node(inode, iedge));
            to_rr_type = rr_graph.node_type(RRNodeId(to_node));
            if (to_rr_type == CHANX || to_rr_type == CHANY) {
                if (rr_graph.node_direction(RRNodeId(to_node)) != Direction::BIDIR) {
                    /* Cout was not added in these cases */
                    Couts_to_add[to_node] = std::max(Couts_to_add[to_node], rr_graph.rr_switch_inf(RRSwitchId(switch_index)).Cout);
                }
            }
        }
    }
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        rr_node_C[(size_t)rr_id] += Couts_to_add[(size_t)rr_id];
    }

    //Create the final flywieghted t_rr_rc_data
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        mutable_device_ctx.rr_graph_builder.set_node_rc_index(rr_id, NodeRCIndex(find_create_rr_rc_data(rr_graph.node_R(rr_id), rr_node_C[(size_t)rr_id])));
    }

    free(Couts_to_add);
    free(cblock_counted);
    free(buffer_Cin);
}
