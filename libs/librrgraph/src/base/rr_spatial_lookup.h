#ifndef RR_SPATIAL_LOOKUP_H
#define RR_SPATIAL_LOOKUP_H

/** 
 * @file
 * @brief This RRSpatialLookup class encapsulates 
 *        the node-lookup for a routing resource graph
 *
 * A data structure built to find the id of an routing resource node 
 * (rr_node) given information about its physical position and type.
 * The data structure is mostly needed during building a routing resource graph
 *
 * The data structure allows users to 
 *
 *   - Update the look-up with new nodes
 *   - Find the id of a node with given information, e.g., x, y, type etc.
 */
#include "vtr_geometry.h"
#include "vtr_vector.h"
#include "physical_types.h"
#include "rr_node_types.h"
#include "rr_graph_fwd.h"

class RRSpatialLookup {
    /* -- Constructors -- */
  public:
    /* Explicitly define the only way to create an object */
    explicit RRSpatialLookup();

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
    /**
     * @brief Returns the index of the specified routing resource node.  
     *
     *   @param (x, y) are the grid location within the FPGA
     *   @param rr_type specifies the type of resource,
     *   @param ptc gives a unique number of resources of that type (e.g. CHANX) at that (x,y).
     *
     * @note All ptcs start at 0 and are positive.
     *       Depending on what type of resource this is, ptc can be 
     *         - the class number of a common SINK/SOURCE node of grid, 
     *           starting at 0 and go up to class_inf size - 1 of SOURCEs + SINKs in a grid
     *         - pin number of an input/output pin of a grid. They would normally start at 0
     *           and go to the number of pins on a block at that (x, y) location
     *         - track number of a routing wire in a channel. They would normally go from 0
     *           to channel_width - 1 at that (x,y)
     *
     * @note An invalid id will be returned if the node does not exist
     *
     * @note For segments (CHANX and CHANY) of length > 1, the segment is
     * given an rr_index based on the (x,y) location at which it starts (i.e.
     * lowest (x,y) location at which this segment exists).
     *
     * @note The 'side' argument only applies to IPIN/OPIN types, and specifies which
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

    /**
     * @brief Returns the indices of the specified routing resource nodes, representing routing tracks in a channel.  
     *
     *   @param (x, y) are the coordinate of the routing channel within the FPGA
     *   @param rr_type specifies the type of routing channel, either x-direction or y-direction
     *
     * @note 
     * - Return an empty list if there are no routing channel at the given (x, y) location
     * - The node list returned only contain valid ids
     *   For example, if the 2nd routing track does not exist in a routing channel at (x, y) location,
     *   while the 3rd routing track does exist in a routing channel at (x, y) location,
     *   the node list will not contain the node for the 2nd routing track, but the 2nd element in the list
     *   will be the node for the 3rd routing track
     */
    std::vector<RRNodeId> find_channel_nodes(int x,
                                             int y,
                                             t_rr_type type) const;

    /**
     * @brief Like find_node() but returns all matching nodes on all the sides.
     *
     * This is particularly useful for getting all instances
     * of a specific IPIN/OPIN at a specific grid tile (x,y) location.
     */
    std::vector<RRNodeId> find_nodes_at_all_sides(int x,
                                                  int y,
                                                  t_rr_type rr_type,
                                                  int ptc) const;

    /**
     * @brief Returns all matching nodes on all the sides at a specific grid tile (x,y) location.
     *
     * As this is applicable to grid pins, the type of nodes are limited to SOURCE/SINK/IPIN/OPIN
     */
    std::vector<RRNodeId> find_grid_nodes_at_all_sides(int x,
                                                       int y,
                                                       t_rr_type rr_type) const;

    /* -- Mutators -- */
  public:
    /** @brief Reserve the memory for a list of nodes at (x, y) location with given type and side */
    void reserve_nodes(int x,
                       int y,
                       t_rr_type type,
                       int num_nodes,
                       e_side side = SIDES[0]);

    /**
     * @brief Register a node in the fast look-up 
     *
     * @note You must have a valid node id to register the node in the lookup
     *
     *   @param (x, y) are the coordinate of the node to be indexable in the fast look-up
     *   @param type is the type of a node
     *   @param ptc is a feature number of a node, which can be
     *     - the class number of a common SINK/SOURCE node of grid, 
     *     - pin index in a tile when type is OPIN/IPIN
     *     - track index in a routing channel when type is CHANX/CHANY
     *   @param side is the side of node on the tile, applicable to OPIN/IPIN 
     *
     * @note a node added with this call will not create a node in the rr_graph_storage node list
     * You MUST add the node in the rr_graph_storage so that the node is valid  
     */
    /*
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
                  e_side side = SIDES[0]);

    /**
     * @brief Mirror the last dimension of a look-up, i.e., a list of nodes, from a source coordinate to 
     * a destination coordinate.
     *
     * This function is mostly need by SOURCE and SINK nodes which are indexable in multiple locations.
     * Considering a bounding box (x, y)->(x + width, y + height) of a multi-height and multi-width grid, 
     * SOURCE and SINK nodes are indexable in any location inside the boundry.
     *
     * An example of usage: 
     *
     *
     * ``` 
     *   // Create a empty lookup
     *   RRSpatialLookup rr_lookup;
     *   // Adding other nodes ...
     *   // Copy the nodes whose types are SOURCE at (1, 1) to (1, 2)
     *   rr_lookup.mirror_nodes(vtr::Point<int>(1, 1),
     *                          vtr::Point<int>(1, 2),
     *                          SOURCE,
     *                          TOP);
     * ```
     *
     * @note currently this function only accepts SOURCE/SINK nodes. May unlock for the other types 
     * depending on needs
     */
    /*
     * TODO: Consider to make a high-level API to duplicate the nodes for large blocks. 
     * Then this API can become a private one
     * For example, 
     *
     *
     * ```
     *   expand_nodes(source_coordinate, bounding_box_coordinate, type, side);
     * ```
     *
     * Alternatively, we can rework the ``find_node()`` API so that we always search the lowest (x,y) 
     * corner when dealing with large blocks. But this may require the data structure to be dependent 
     * on DeviceGrid information (it needs to identify if a grid has height > 1 as well as width > 1)
     */
    void mirror_nodes(const vtr::Point<int>& src_coord,
                      const vtr::Point<int>& des_coord,
                      t_rr_type type,
                      e_side side);

    /**
     * @brief Resize the given 3 dimensions (x, y, side) of the RRSpatialLookup data structure for the given type
     *
     * This function will keep any existing data
     *
     * @note Strongly recommend to use when the sizes of dimensions are deterministic
     */
    /*
     * TODO: should have a reserve function but vtd::ndmatrix does not have such API
     *       as a result, resize can be an internal one while reserve function is a public mutator
     */
    void resize_nodes(int x,
                      int y,
                      t_rr_type type,
                      e_side side);

    /** @brief Reorder the internal look up to be more memory efficient */
    void reorder(const vtr::vector<RRNodeId, RRNodeId> dest_order);

    /** @brief Clear all the data inside */
    void clear();

    /* -- Internal data queries -- */
  private:
    /* An internal API to find all the nodes in a specific location with a given type
     * For OPIN/IPIN nodes that may exist on multiple sides, a specific side must be provided  
     * This API is NOT public because its too powerful for developers with very limited sanity checks 
     * But it is used to build the public APIs find_channel_nodes() etc., where sufficient sanity checks are applied
     */
    std::vector<RRNodeId> find_nodes(int x,
                                     int y,
                                     t_rr_type type,
                                     e_side side = SIDES[0]) const;

    /* -- Internal data storage -- */
  private:
    /* Fast look-up: TODO: Should rework the data type. Currently it is based on a 3-dimensional arrqay mater where some dimensions must always be accessed with a specific index. Such limitation should be overcome */
    t_rr_node_indices rr_node_indices_;
};

#endif
