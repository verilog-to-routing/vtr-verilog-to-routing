#ifndef RR_SPATIAL_LOOKUP_H
#define RR_SPATIAL_LOOKUP_H

#include "vpr_types.h"

/******************************************************************** 
 * A data structure storing the fast look-up for the nodes
 * in a routing resource graph
 *
 * The data structure allows users to 
 * - Update the look-up with new nodes
 * - Find the id of a node with given information, e.g., x, y, type etc.
 ********************************************************************/
class RRSpatialLookup {
    /****************
     * Constructors
     ****************/
  public:
    RRSpatialLookup(t_rr_node_indices& rr_node_indices);

    /****************
     * Accessors
     ****************/
  public:
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
    RRNodeId find_node(int x,
                       int y,
                       t_rr_type type,
                       int ptc,
                       e_side side = NUM_SIDES) const;

    /****************
     * Mutators
     ****************/
  public:
    /* Register a node in the fast look-up 
     * - You must have a node id
     *   1. a valid one is to register a node in the lookup
     *   2. an invalid id means the removal of a node from the lookup
     * - (x, y) are the coordinate of the node to be indexable in the fast look-up
     * - type is the type of a node
     * - ptc is a feature number of a node, which can be
     *   1. pin index in a tile when type is OPIN/IPIN
     *   2. track index in a routing channel when type is CHANX/CHANY
     * - side is the side of node on the tile, applicable to OPIN/IPIN 
     *
     * Note that the add node here will not create a node in the node list
     * You MUST add the node in the t_rr_node_storage so that the node is valid  
     */
    void add_node(RRNodeId node,
                  int x,
                  int y,
                  t_rr_type type,
                  int ptc,
                  e_side side);

    /****************
     * internal data storage
     ****************/
  private:
    /* TODO: When the refactoring effort finishes, 
     * the data structure will be the owner of the data storages. 
     * That is why the reference is used here.
     * It can avoid a lot of code changes once the refactoring is finished 
     * (there is no function get data directly through the rr_node_indices in DeviceContext).
     * If pointers are used, it may cause many codes in client functions 
     * or inside the data structures to be changed later.
     * That explains why the reference is used here temporarily
     */
    /* Fast look-up */
    t_rr_node_indices& rr_node_indices_;
};

#endif
