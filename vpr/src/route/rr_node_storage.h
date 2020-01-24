#ifndef _RR_NODE_STORAGE_
#define _RR_NODE_STORAGE_

#include "rr_node_fwd.h"

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_* structures.                                            *
 *                                                                           *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for           *
 *       coordinate system) of the ends of this routing resource.            *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of           *
 *       length 1.  These values are used to decide whether or not this      *
 *       node should be added to the expansion heap, based on things         *
 *       like whether it's outside the net bounding box or is moving         *
 *       further away from the target, etc.                                  *
 * type:  What is this routing resource?                                     *
 * ptc_num:  Pin, track or class number, depending on rr_node type.          *
 *           Needed to properly draw.                                        *
 * cost_index: An integer index into the table of routing resource indexed   *
 *             data t_rr_index_data (this indirection allows quick dynamic   *
 *             changes of rr base costs, and some memory storage savings for *
 *             fields that have only a few distinct values).                 *
 * capacity:   Capacity of this node (number of routes that can use it).     *
 * num_edges:  Number of edges exiting this node.  That is, the number       *
 *             of nodes to which it connects.                                *
 * edges[0..num_edges-1]:  Array of indices of the neighbours of this        *
 *                         node.                                             *
 * switches[0..num_edges-1]:  Array of switch indexes for each of the        *
 *                            edges leaving this node.                       *
 *                                                                           *
 * direction: if the node represents a track, this field                     *
 *            indicates the direction of the track. Otherwise                *
 *            the value contained in the field should be                     *
 *            ignored.                                                       *
 * side: The side of a grid location where an IPIN or OPIN is located.       *
 *       This field is valid only for IPINs and OPINs and should be ignored  *
 *       otherwise.                                                          */
struct t_rr_node_data {
    //The edge information is stored in a structure to economize on the number of pointers held
    //by t_rr_node (to save memory), and is not exposed externally
    struct t_rr_edge {
        int sink_node = -1;   //The ID of the sink RR node associated with this edge
        short switch_id = -1; //The ID of the switch type this edge represents
    };

    //Note: we use a plain array and use small types for sizes to save space vs std::vector
    //      (using std::vector's nearly doubles the size of the class)
    std::unique_ptr<t_rr_edge[]> edges_ = nullptr;
    t_edge_size num_edges_ = 0;
    t_edge_size edges_capacity_ = 0;
    uint8_t num_non_configurable_edges_ = 0;

    int8_t cost_index_ = -1;
    int16_t rc_index_ = -1;

    int16_t xlow_ = -1;
    int16_t ylow_ = -1;
    int16_t xhigh_ = -1;
    int16_t yhigh_ = -1;

    t_rr_type type_ = NUM_RR_TYPES;
    union {
        e_direction direction; //Valid only for CHANX/CHANY
        e_side side;           //Valid only for IPINs/OPINs
    } dir_side_;

    union {
        int16_t pin_num;
        int16_t track_num;
        int16_t class_num;
    } ptc_;
    t_edge_size fan_in_ = 0;
    uint16_t capacity_ = 0;
};

// RR node and edge storage class.
class t_rr_node_storage {
  public:
    void reserve(size_t size) {
        storage_.reserve(size);
    }
    void resize(size_t size) {
        storage_.resize(size);
    }
    size_t size() const {
        return storage_.size();
    }
    bool empty() const {
        return storage_.empty();
    }

    void clear() {
        storage_.clear();
    }

    void shrink_to_fit() {
        storage_.shrink_to_fit();
    }

    void emplace_back() {
        storage_.emplace_back();
    }

    node_idx_iterator begin() const;

    node_idx_iterator end() const;

    const t_rr_node operator[](size_t idx) const;
    t_rr_node operator[](size_t idx);
    const t_rr_node at(size_t idx) const;
    t_rr_node at(size_t idx);

    const t_rr_node front() const;
    t_rr_node front();
    const t_rr_node back() const;
    t_rr_node back();

    friend class t_rr_node;

  private:
    t_rr_node_data& get(const RRNodeId& id) {
        return storage_[id];
    }
    const t_rr_node_data& get(const RRNodeId& id) const {
        return storage_[id];
    }

    vtr::vector<RRNodeId, t_rr_node_data> storage_;
};

#endif /* _RR_NODE_STORAGE_ */
