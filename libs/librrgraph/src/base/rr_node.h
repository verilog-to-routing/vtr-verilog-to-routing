#ifndef RR_NODE_H
#define RR_NODE_H

#include <limits>
#include "physical_types.h"
#include "rr_node_fwd.h"
#include "rr_graph_fwd.h"
#include "rr_node_types.h"

#include "vtr_range.h"

#include <memory>
#include <cstdint>
#include <bitset>

// t_rr_node is a proxy object for accessing data in t_rr_graph_storage.
//
// In general, new code should not use this object, but instead directly
// use the t_rr_graph_storage object.  However this object remains for
// backwards compability, as described below.
//
// RR node and edges was original stored within the t_rr_node object, the
// full RR graph was stored in a std::vector<t_rr_node>, which was effectively
// an array of structures..
//
// The RR graph has since been refactored into the t_rr_graph_storage object.
// To prevent requiring all callsites where the std::vector<t_rr_node> to be
// changed at once, t_rr_graph_storage implements an interface that appears
// similiar to std::vector<t_rr_node>, even though the underlying storage is
// no longer a array of t_rr_node's.
//
// The t_rr_node class forwards all data accesses to t_rr_graph_storage, and
// itself only holds a pointer to the t_rr_graph_storage class and the RRNodeId
// being proxied.
//
// t_rr_node should never be directly constructed, instead
// t_rr_graph_storage::operator [] should be used.
//
// If additional **constant** RR node or RR edge data needs to be added while
// the t_rr_node proxy is still in used, then a get and set methods should be
// added first to t_rr_graph_storage, and then proxy methods should be added
// t_rr_node to invoke the relevant methods on t_rr_graph_storage.
//
// Example:
//
// For example let's add a field called "test" with type "t_type" to the RR
// node.
//
// First, the following methods should be added to t_rr_graph_storage:
//
// /* Get method to t_rr_graph_storage */
// t_type t_rr_graph_storage::node_test(RRNodeId) const;
//
// /* Set method to t_rr_graph_storage */
// void t_rr_graph_storage::set_node_test(RRNodeId, t_type);
//
// The particular storage method within t_rr_graph_storage depends on the data.
// See t_rr_graph_storage for the storage philosphy within philosophy.
//
// Second, add the proxy methods to t_rr_node.  Method prototypes should be
// added in rr_node.h and method implementations should be added in
// rr_node_impl.h.
//
// The implementation for the proxy methods would be something akin to:
//
// t_type t_rr_node::test() const {
//   return storage_->node_test(id_);
// }
//
// void t_rr_node::set_test(t_type value) {
//   storage_->set_node_test(id_, value);
// }
//
// By updating both t_rr_graph_storage and t_rr_node, either can be used until
// t_rr_node is fully removed in favor of directly using t_rr_graph_storage.
class t_rr_node {
  public: //Types
    t_rr_node(t_rr_graph_storage* storage, RRNodeId id)
        : storage_(storage)
        , id_(id) {}

  public: //Accessors
    edge_idx_range edges() const;
    edge_idx_range configurable_edges() const;
    edge_idx_range non_configurable_edges() const;

    t_edge_size num_edges() const;
    t_edge_size num_configurable_edges() const;
    t_edge_size num_non_configurable_edges() const;

    int edge_sink_node(t_edge_size iedge) const;
    short edge_switch(t_edge_size iedge) const;

    signed short length() const;

    RRIndexedDataId cost_index() const;
    short rc_index() const;

  public: //Mutators
    void set_side(e_side);

    void next_node() {
        id_ = RRNodeId((size_t)(id_) + 1);
    }

    RRNodeId id() const {
        return id_;
    }

    void prev_node() {
        id_ = RRNodeId((size_t)(id_)-1);
    }

  private: //Data
    t_rr_graph_storage* storage_;
    RRNodeId id_;
};

/* Data that is pointed to by the .cost_index member of t_rr_node.  It's     *
 * purpose is to store the base_cost so that it can be quickly changed       *
 * and to store fields that have only a few different values (like           *
 * seg_index) or whose values should be an average over all rr_nodes of a    *
 * certain type (like T_linear etc., which are used to predict remaining     *
 * delay in the timing_driven router).                                       *
 *                                                                           *
 * base_cost:  The basic cost of using an rr_node.                           *
 * ortho_cost_index:  The index of the type of rr_node that generally        *
 *                    connects to this type of rr_node, but runs in the      *
 *                    orthogonal direction (e.g. vertical if the direction   *
 *                    of this member is horizontal).                         *
 * seg_index:  Index into segment_inf of this segment type if this type of   *
 *             rr_node is an CHANX or CHANY; OPEN (-1) otherwise.            *
 * inv_length:  1/length of this type of segment.                            *
 * T_linear:  Delay through N segments of this type is N * T_linear + N^2 *  *
 *            T_quadratic.  For buffered segments all delay is T_linear.     *
 * T_quadratic:  Dominant delay for unbuffered segments, 0 for buffered      *
 *               segments.                                                   *
 * C_load:  Load capacitance seen by the driver for each segment added to    *
 *          the chain driven by the driver.  0 for buffered segments.        */

struct t_rr_indexed_data {
    float base_cost = std::numeric_limits<float>::quiet_NaN();
    float saved_base_cost = std::numeric_limits<float>::quiet_NaN();
    int ortho_cost_index = OPEN;
    int seg_index = OPEN;
    float inv_length = std::numeric_limits<float>::quiet_NaN();
    float T_linear = std::numeric_limits<float>::quiet_NaN();
    float T_quadratic = std::numeric_limits<float>::quiet_NaN();
    float C_load = std::numeric_limits<float>::quiet_NaN();
};

#include "rr_node_impl.h"

#endif
