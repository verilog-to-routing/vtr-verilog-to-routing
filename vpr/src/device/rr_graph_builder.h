#ifndef RR_GRAPH_BUILDER_H
#define RR_GRAPH_BUILDER_H

/**
 * @file 
 * @brief This file defines the RRGraphBuilder data structure which allows data modification on a routing resource graph 
 *
 * The builder does not own the storage but it serves a virtual protocol for
 *   - node_storage: store the node list 
 *   - node_lookup: store a fast look-up for the nodes
 *
 * @note
 * - This is the only data structure allowed to modify a routing resource graph
 *
 */
#include "rr_graph_storage.h"
#include "rr_spatial_lookup.h"

class RRGraphBuilder {
    /* -- Constructors -- */
  public:
    /* See detailed comments about the data structures in the internal data storage section of this file */
    RRGraphBuilder(t_rr_graph_storage* node_storage);

    /* Disable copy constructors and copy assignment operator
     * This is to avoid accidental copy because it could be an expensive operation considering that the 
     * memory footprint of the data structure could ~ Gb
     * Using the following syntax, we prohibit accidental 'pass-by-value' which can be immediately caught 
     * by compiler
     */
    RRGraphBuilder(const RRGraphBuilder&) = delete;
    void operator=(const RRGraphBuilder&) = delete;

    /* -- Mutators -- */
  public:
    /** @brief Return a writable object for rr_nodes */
    t_rr_graph_storage& node_storage();
    /** @brief Return a writable object for update the fast look-up of rr_node */
    RRSpatialLookup& node_lookup();
    /** @brief Set the type of a node with a given valid id */
    inline void set_node_type(RRNodeId id, t_rr_type type) {
        node_storage_.set_node_type(id, type);
    }
    /**
     * @brief Add an existing rr_node in the node storage to the node look-up
     *
     * The node will be added to the lookup for every side it is on (for OPINs and IPINs) 
     * and for every (x,y) location at which it exists (for wires that span more than one (x,y)).
     *
     * This function requires a valid node which has already been allocated in the node storage, with
     *   - a valid node id
     *   - valid geometry information: xlow/ylow/xhigh/yhigh
     *   - a valid node type
     *   - a valid node ptc number
     *   - a valid side (applicable to OPIN and IPIN nodes only
     */
    void add_node_to_all_locs(RRNodeId node);

    /** @brief Clear all the underlying data storage */
    void clear();

    /** @brief Set capacity of this node (number of routes that can use it). */
    inline void set_node_capacity(RRNodeId id, short new_capacity) {
        node_storage_.set_node_capacity(id, new_capacity);
    }

    /** @brief Set the node coordinate */
    inline void set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2) {
        node_storage_.set_node_coordinates(id, x1, y1, x2, y2);
    }

    /** @brief Set the node_ptc_num; The ptc (pin, track, or class) number is an integer
     * that allows you to differentiate between wires, pins or sources/sinks with overlapping x,y coordinates or extent.
     * This is useful for drawing rr-graphs nicely.
     *
     * The ptc_num carries different meanings for different node types
     * (true in VPR RRG that is currently supported, may not be true in customized RRG)
     * CHANX or CHANY: the track id in routing channels
     * OPIN or IPIN: the index of pins in the logic block data structure
     * SOURCE and SINK: the class id of a pin (indicating logic equivalence of pins) in the logic block data structure */
    inline void set_node_ptc_num(RRNodeId id, short new_ptc_num) {
        node_storage_.set_node_ptc_num(id, new_ptc_num);
    }

    /** @brief set_node_pin_num() is designed for logic blocks, which are IPIN and OPIN nodes */
    inline void set_node_pin_num(RRNodeId id, short new_pin_num) {
        node_storage_.set_node_pin_num(id, new_pin_num);
    }

    /** @brief set_node_track_num() is designed for routing tracks, which are CHANX and CHANY nodes */
    inline void set_node_track_num(RRNodeId id, short new_track_num) {
        node_storage_.set_node_track_num(id, new_track_num);
    }

    /** @brief set_ node_class_num() is designed for routing source and sinks, which are SOURCE and SINK nodes */
    inline void set_node_class_num(RRNodeId id, short new_class_num) {
        node_storage_.set_node_class_num(id, new_class_num);
    }

    /** @brief Set the node direction; The node direction is only available of routing channel nodes, such as x-direction routing tracks (CHANX) and y-direction routing tracks (CHANY). For other nodes types, this value is not meaningful and should be set to NONE. */
    inline void set_node_direction(RRNodeId id, Direction new_direction) {
        node_storage_.set_node_direction(id, new_direction);
    }

    /* -- Internal data storage -- */
  private:
    /* TODO: When the refactoring effort finishes, 
     * the builder data structure will be the owner of the data storages. 
     * That is why the reference to storage/lookup is used here.
     * It can avoid a lot of code changes once the refactoring is finished 
     * (there is no function get data directly through the node_storage in DeviceContext).
     * If pointers are used, it may cause many codes in client functions 
     * or inside the data structures to be changed later.
     * That explains why the reference is used here temporarily
     */
    /* node-level storage including edge storages */
    t_rr_graph_storage& node_storage_;
    /* Fast look-up for rr nodes */
    RRSpatialLookup node_lookup_;
};

#endif
