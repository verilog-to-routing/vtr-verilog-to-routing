#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node) {
    /* Returns the segment number (distance along the channel) of the connection *
     * box from from_rr_type (CHANX or CHANY) to to_node (IPIN).                 */

    auto& device_ctx = g_vpr_ctx.device();

    if (from_rr_type == CHANX)
        return (device_ctx.rr_nodes[to_node].xlow());
    else
        /* CHANY */
        return (device_ctx.rr_nodes[to_node].ylow());
}

int seg_index_of_sblock(int from_node, int to_node) {
    /* Returns the segment number (distance along the channel) of the switch box *
     * box from from_node (CHANX or CHANY) to to_node (CHANX or CHANY).  The     *
     * switch box on the left side of a CHANX segment at (i,j) has seg_index =   *
     * i-1, while the switch box on the right side of that segment has seg_index *
     * = i.  CHANY stuff works similarly.  Hence the range of values returned is *
     * 0 to device_ctx.grid.width()-1 (if from_node is a CHANX) or 0 to device_ctx.grid.height()-1 (if from_node is a CHANY).   */

    t_rr_type from_rr_type, to_rr_type;

    auto& device_ctx = g_vpr_ctx.device();

    from_rr_type = device_ctx.rr_nodes[from_node].type();
    to_rr_type = device_ctx.rr_nodes[to_node].type();

    if (from_rr_type == CHANX) {
        if (to_rr_type == CHANY) {
            return (device_ctx.rr_nodes[to_node].xlow());
        } else if (to_rr_type == CHANX) {
            if (device_ctx.rr_nodes[to_node].xlow() > device_ctx.rr_nodes[from_node].xlow()) { /* Going right */
                return (device_ctx.rr_nodes[from_node].xhigh());
            } else { /* Going left */
                return (device_ctx.rr_nodes[to_node].xhigh());
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANX */
    else if (from_rr_type == CHANY) {
        if (to_rr_type == CHANX) {
            return (device_ctx.rr_nodes[to_node].ylow());
        } else if (to_rr_type == CHANY) {
            if (device_ctx.rr_nodes[to_node].ylow() > device_ctx.rr_nodes[from_node].ylow()) { /* Going up */
                return (device_ctx.rr_nodes[from_node].yhigh());
            } else { /* Going down */
                return (device_ctx.rr_nodes[to_node].yhigh());
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in seg_index_of_sblock: to_node %d is of type %d.\n",
                            to_node, to_rr_type);
            return OPEN; //Should not reach here once thrown
        }
    }
    /* End from_rr_type is CHANY */
    else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in seg_index_of_sblock: from_node %d is of type %d.\n",
                        from_node, from_rr_type);
        return OPEN; //Should not reach here once thrown
    }
}
