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
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
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
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
				 "in seg_index_of_sblock: to_node %d is of type %d.\n",
					to_node, to_rr_type);
			return OPEN; //Should not reach here once thrown
		}
	}
	/* End from_rr_type is CHANY */
	else {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
			"in seg_index_of_sblock: from_node %d is of type %d.\n",
				from_node, from_rr_type);
		return OPEN; //Should not reach here once thrown
	}
}

void set_rr_node_cost_idx_based_on_seg_idx(int num_segments) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;

    for(size_t node_idx = 0; node_idx < rr_nodes.size(); node_idx++) {
        t_rr_type rr_type = rr_nodes[node_idx].type();
        size_t seg_idx = rr_nodes[node_idx].seg_index();
        // TODO: Add check to make sure that seg_idx was set. Can be done by chaning
        //       seg_index to be a short and then Asserting that its value is not -1
        //       since that is the value that rr_node initializes it with. However,
        //       seg_index() function currently returns a size_t type value
        if(rr_type == CHANX) {
            rr_nodes[node_idx].set_cost_index(CHANX_COST_INDEX_START + seg_idx);
        } else if (rr_type == CHANY) {
            rr_nodes[node_idx].set_cost_index(CHANX_COST_INDEX_START + num_segments + seg_idx);
        }
    }

}
