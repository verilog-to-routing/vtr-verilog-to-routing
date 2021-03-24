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

/* An overlay of routing resource graph
 * which is an unified object including pointors to
 * - node storage
 * - edge_storage
 * - node_ptc_storage
 * - node_fan_in_storage
 * - rr_node_indices
 *
 * Note that the overlay does not own the storage
 * It serves a virtual protocol for 
 * - rr_graph builder
 * - placer
 * - router
 * - timing analyzer
 * - GUI
 *
 * Note that each client of rr_graph may get a frame view of the object
 * - This helps to reduce the memory footprint for each client
 * - This avoids massive changes for each client on using the APIs
 *   as each frame view provides adhoc APIs for each client
 *  
 */
class RRGraphOverlay {
  /****************
   * Constructors
   ****************/
  public:
    RRGraphOverlay();

  /****************
   * Accessors
   ****************/
  public:
    t_rr_type node_type(const RRNodeId& id) const;

  /****************
   * Mutators
   ****************/
  public:
    void set_internal_data(t_rr_graph_storage* node_storage,
                           t_rr_node_indices* rr_node_indices);

  /****************
   * internal data
   ****************/
  private:
    /* node-level storage including edge storages */
    t_rr_graph_storage* node_storage_;
    /* Fast look-up */
    t_rr_node_indices* rr_node_indices_; 
};

#endif
