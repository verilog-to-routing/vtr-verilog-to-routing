#ifndef RR_SPATIAL_LOOKUP_H
#define RR_SPATIAL_LOOKUP_H

#include "vpr_types.h"

/******************************************************************** 
 * A data structure built to find the id of an routing resource node 
 * (rr_node) given information about its physical position and type.
 * The data structure is mostly needed during rr_graph building
 *
 * The data structure allows users to 
 * - Update the look-up with new nodes
 * - Find the id of a node with given information, e.g., x, y, type etc.
 ********************************************************************/
class RRSpatialLookup {
    /* -- Constructors -- */
  public:
    /* Explicitly define the only way to create an object */
    explicit RRSpatialLookup(t_rr_node_indices& rr_node_indices);

    /* Disable copy constructors and copy assignment operator
     * This is to avoid accidental copy because it could be an expensive operation considering that the 
     * memory footprint of the data structure could ~ Gb
     * Using the following syntax, we prohibit accidental 'pass-by-value' which can be immediately caught 
     * by compiler
     */
    RRSpatialLookup(const RRSpatialLookup&) = delete;
    void operator=(const RRSpatialLookup&) = delete;

    /* -- Accessors -- */
  public:
    /* Returns the index of the specified routing resource node.  
     * - (x, y) are the grid location within the FPGA
     * - rr_type specifies the type of resource,
     * - ptc gives a unique number of resources of that type (e.g. CHANX) at that (x,y).
     *   All ptcs start at 0 and are positive.
     *   Depending on what type of resource this is, ptc can be 
     *     - the class number of a common SINK/SOURCE node of grid, 
     *       starting at 0 and go up to class_inf size - 1 of SOURCEs + SINKs in a grid
     *     - pin number of an input/output pin of a grid. They would normally start at 0
     *       and go to the number of pins on a block at that (x, y) location
     *     - track number of a routing wire in a channel. They would normally go from 0
     *       to channel_width - 1 at that (x,y)
     *
     * An invalid id will be returned if the node does not exist
     *
     * Note that for segments (CHANX and CHANY) of length > 1, the segment is
     * given an rr_index based on the (x,y) location at which it starts (i.e.
     * lowest (x,y) location at which this segment exists).
     *
     * The 'side' argument only applies to IPIN/OPIN types, and specifies which
     * side of the grid tile the node should be located on. The value is ignored
     * for non-IPIN/OPIN types
     *
     * This routine also performs error checking to make sure the node in
     * question exists.
     */
    RRNodeId find_node(int x,
                       int y,
                       t_rr_type type,
                       int ptc,
                       e_side side = NUM_SIDES) const;

    /* -- Mutators -- */
  public:
    /* Register a node in the fast look-up 
     * - You must have a valid node id to register the node in the lookup
     * - (x, y) are the coordinate of the node to be indexable in the fast look-up
     * - type is the type of a node
     * - ptc is a feature number of a node, which can be
     *   - the class number of a common SINK/SOURCE node of grid, 
     *   - pin index in a tile when type is OPIN/IPIN
     *   - track index in a routing channel when type is CHANX/CHANY
     * - side is the side of node on the tile, applicable to OPIN/IPIN 
     *
     * Note that a node added with this call will not create a node in the rr_graph_storage node list
     * You MUST add the node in the rr_graph_storage so that the node is valid  
     *
     * TODO: Consider to try to return a reference to *this so that we can do chain calls
     *   - .add_node(...)
     *   - .add_node(...)
     *   - .add_node(...)
     *   As such, multiple node addition could be efficiently implemented
     */
    void add_node(RRNodeId node,
                  int x,
                  int y,
                  t_rr_type type,
                  int ptc,
                  e_side side);

    /* TODO: Add an API remove_node() to unregister a node from the look-up */

    /* -- Internal data storage -- */
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
