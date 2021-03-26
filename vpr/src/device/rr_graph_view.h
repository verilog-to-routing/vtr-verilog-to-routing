#ifndef _RR_GRAPH_OVERLAY_
#define _RR_GRAPH_OVERLAY_

#include <exception>

#include "rr_graph_fwd.h"
#include "rr_node_fwd.h"
#include "rr_graph2.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vpr_utils.h"
#include "vtr_strong_id_range.h"
#include "vtr_array_view.h"
#include "rr_graph_storage.h"

/* An read-only routing resource graph
 * which is an unified object including pointors to
 * - node storage
 * - edge_storage
 * - node_ptc_storage
 * - node_fan_in_storage
 * - rr_node_indices
 *
 * Note that the RRGraphView does not own the storage
 * It serves a virtual read-only protocol for
 * - placer
 * - router
 * - timing analyzer
 * - GUI
 *
 * Note that each client of rr_graph may get a frame view of the object
 * The RRGraphView is the complete frame view of the routing resource graph
 * - This helps to reduce the memory footprint for each client
 * - This avoids massive changes for each client on using the APIs
 *   as each frame view provides adhoc APIs for each client
 *
 * TODO: more compact frame views will be created, e.g., 
 * - a mini frame view: contains only node and edges, representing the connectivity of the graph
 * - a geometry frame view: an extended mini frame view with node-level attributes, 
 *                          in particular geometry information (type, x, y etc).
 *
 */
class RRGraphView {
    /****************
     * Constructors
     ****************/
  public:
    RRGraphView();

    /****************
     * Accessors
     ****************/
  public:
    /* Get the type of a routing resource node */
    t_rr_type node_type(const RRNodeId& node) const;

    /* Returns the index of the specified routing resource node.  (x,y) are
     * the location within the FPGA, rr_type specifies the type of resource,
     * and ptc gives the number of this resource.  ptc is the class number,
     * pin number or track number, depending on what type of resource this
     * is.  All ptcs start at 0 and go up to pins_per_clb-1 or the equivalent.
     * There are class_inf size SOURCEs + SINKs, type->num_pins IPINs + OPINs,
     * and max_chan_width CHANX and CHANY (each).
     *
     * Note that for segments (CHANX and CHANY) of length > 1, the segment is
     * given an rr_index based on the (x,y) location at which it starts (i.e.
     * lowest (x,y) location at which this segment exists).
     * This routine also performs error checking to make sure the node in
     * question exists.
     *
     * The 'side' argument only applies to IPIN/OPIN types, and specifies which
     * side of the grid tile the node should be located on. The value is ignored
     * for non-IPIN/OPIN types
     */
    RRNodeId find_node(const int& x,
                       const int& y,
                       const t_rr_type& type,
                       const int& ptc,
                       const e_side& side = NUM_SIDES) const;

    /****************
     * Mutators
     ****************/
  public:
    /* The ONLY mutators allowed */
    void set_internal_data(t_rr_graph_storage* node_storage,
                           t_rr_node_indices* rr_node_indices);

    /****************
     * internal data storage, can be accessed by parent class RRGraphBuilderView
     ****************/
  protected:
    /* node-level storage including edge storages */
    t_rr_graph_storage* node_storage_;
    /* Fast look-up */
    t_rr_node_indices* rr_node_indices_;
};

#endif
