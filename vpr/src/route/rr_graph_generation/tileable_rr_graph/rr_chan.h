#pragma once

#include <vector>

#include "vtr_geometry.h"

#include "rr_graph_view.h"

/********************************************************************
 * RRChan object aim to describe a routing channel in a routing resource graph 
 *  - What are the nodes in the RRGraph object, for each routing track
 *  - What are routing segments used by each node in the channel
 *  - What are the directions of each routing channel 
 *    being either X-direction or Y-direction
 *
 * Note : 
 *  - This is a collection of rr_nodes from the RRGraph Object 
 *    It does not rebuild or contruct any connects between rr_nodes
 *    It is just an annotation on an existing RRGraph
 *   -------------   ------
 *  |             | |      |
 *  |             | |  Y   |
 *  |     CLB     | | Chan |
 *  |             | |      |
 *  |             | |      |
 *   -------------   ------
 *   -------------
 *  |      X      |
 *  |    Channel  |
 *   -------------
 *******************************************************************/
class RRChan {
  public: // Constructors
    RRChan();

  public: // Accessors
    e_rr_type get_type() const;
    
    /**
     * @brief Get the number of tracks in this channel
     */
    size_t get_chan_width() const;

    /**
     * @brief Get the track_id of a node
     */
    int get_node_track_id(const RRNodeId node) const;

    /**
     * @brief Get the rr_node with the track_id
     */
    RRNodeId get_node(const size_t track_num) const;

    RRSegmentId get_node_segment(const RRNodeId node) const;

    RRSegmentId get_node_segment(const size_t track_num) const;

    /**
     * @brief Get a list of segments used in this routing channel
     */
    std::vector<RRSegmentId> get_segment_ids() const;

    /**
     * @brief Get a list of segments used in this routing channel
     */
    std::vector<size_t> get_node_ids_by_segment_ids(const RRSegmentId seg_id) const;
  public:
    void set(const RRChan&);

    /**
     * @brief Modify the type of routing channel
     */
    void set_type(const e_rr_type type);

    /**
     * @brief Reserve a number of nodes to the array
     */
    void reserve_node(const size_t node_size);

    /**
     * @brief Add a node to the routing channel
     */
    void add_node(const RRGraphView& rr_graph, const RRNodeId node, const RRSegmentId node_segment);

    /**
     * @brief Clear the content
     */
    void clear();

  private:
    /**
     * @brief Check if the type of a routing channel is valid
     */
    bool is_rr_type_valid(const e_rr_type type) const;

    /**
     * @brief Check if the node type is consistent with the type of routing channel
     */
    bool valid_node_type(const RRGraphView& rr_graph, const RRNodeId node) const;

    /**
     * @brief Validate if the track number in the range
     */
    bool valid_node_id(const size_t node_id) const;

  private:
    e_rr_type type_;                         // channel type: CHANX or CHANY
    std::vector<RRNodeId> nodes_;            // rr nodes of each track in the channel
    std::vector<RRSegmentId> node_segments_; // segment of each track
};
