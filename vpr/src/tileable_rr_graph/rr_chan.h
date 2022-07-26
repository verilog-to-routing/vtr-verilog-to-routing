#ifndef RR_CHAN_H
#define RR_CHAN_H

/********************************************************************
 * Include header files required by the data structure definition
 *******************************************************************/
#include <vector>

/* Headers from vtrutil library */
#include "vtr_geometry.h"

/* Headers from openfpgautil library */
#include "openfpga_port.h"

/* Headers from vpr library */
#include "rr_graph_obj.h"

/* Begin namespace openfpga */
namespace openfpga {

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
  public: /* Constructors */
    RRChan();
  public: /* Accessors */
    t_rr_type get_type() const;
    size_t get_chan_width() const; /* get the number of tracks in this channel */
    int get_node_track_id(const RRNodeId& node) const; /* get the track_id of a node */
    RRNodeId get_node(const size_t& track_num) const; /* get the rr_node with the track_id */
    RRSegmentId get_node_segment(const RRNodeId& node) const;
    RRSegmentId get_node_segment(const size_t& track_num) const;
    bool is_mirror(const RRGraph& rr_graph, const RRChan& cand) const; /* evaluate if two RR_chan is mirror to each other */
    std::vector<RRSegmentId> get_segment_ids() const; /* Get a list of segments used in this routing channel */
    std::vector<size_t> get_node_ids_by_segment_ids(const RRSegmentId& seg_id) const; /* Get a list of segments used in this routing channel */
  public: /* Mutators */
    /* copy */
    void set(const RRChan&); 

    /* modify the type of routing channel */
    void set_type(const t_rr_type& type); 

    /* reseve a number of nodes to the array */
    void reserve_node(const size_t& node_size); 

    /* add a node to the routing channel */
    void add_node(const RRGraph& rr_graph, const RRNodeId& node, const RRSegmentId& node_segment); 

    /* clear the content */
    void clear(); 

  private: /* internal functions */

    /* For the type of a routing channel, only valid type is CHANX and CHANY */
    bool valid_type(const t_rr_type& type) const;  

    /* Check each node, see if the node type is consistent with the type of routing channel */
    bool valid_node_type(const RRGraph& rr_graph, const RRNodeId& node) const;  

    /* Validate if the track number in the range */
    bool valid_node_id(const size_t& node_id) const;  

  private: /* Internal Data */
    t_rr_type type_; /* channel type: CHANX or CHANY */
    std::vector<RRNodeId> nodes_; /* rr nodes of each track in the channel */
    std::vector<RRSegmentId> node_segments_; /* segment of each track */
};



} /* End namespace openfpga*/

#endif
