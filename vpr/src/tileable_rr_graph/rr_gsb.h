#ifndef RR_GSB_H
#define RR_GSB_H

/********************************************************************
 * Include header files required by the data structure definition
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_geometry.h"

#include "rr_chan.h"
#include "rr_graph_view.h"

/********************************************************************
 * Object Generic Switch Block
 * This block contains
 * 1. A switch block
 * 2. A X-direction Connection block locates at the left side of the switch block
 * 2. A Y-direction Connection block locates at the top side of the switch block
 *
 *                                          TOP SIDE
 *  +-------------+              +---------------------------------+
 *  |             |              | OPIN_NODE CHAN_NODES OPIN_NODES |
 *  |             |              |                                 |
 *  |             |              | OPIN_NODES           OPIN_NODES |
 *  | X-direction |              |                                 |
 *  |      CB     |  LEFT SIDE   |        Switch Block             |  RIGHT SIDE
 *  |   [x][y]    |              |              [x][y]             |
 *  |             |              |                                 |
 *  |             |              | CHAN_NODES           CHAN_NODES |
 *  |             |              |                                 |
 *  |             |              | OPIN_NODES           OPIN_NODES |
 *  |             |              |                                 |
 *  |             |              | OPIN_NODE CHAN_NODES OPIN_NODES |
 *  +-------------+              +---------------------------------+
 *                                             BOTTOM SIDE
 *  +-------------+              +---------------------------------+
 *  |    Grid     |              |          Y-direction CB         |
 *  |  [x][y]     |              |              [x][y]             |
 *  +-------------+              +---------------------------------+
 *
 * num_sides: number of sides of this switch block
 * chan_rr_node: a collection of rr_nodes as routing tracks locating at each side of the Switch block <0..num_sides-1><0..chan_width-1>
 * chan_rr_node_direction: Indicate if this rr_node is an input or an output of the Switch block <0..num_sides-1><0..chan_width-1>
 * ipin_rr_node: a collection of rr_nodes as IPIN of a GRID locating at each side of the Switch block <0..num_sides-1><0..num_ipin_rr_nodes-1>
 * ipin_rr_node_grid_side: specify the side of the input pins on which side of a GRID  <0..num_sides-1><0..num_ipin_rr_nodes-1>
 * opin_rr_node: a collection of rr_nodes as OPIN of a GRID locating at each side of the Switch block <0..num_sides-1><0..num_opin_rr_nodes-1>
 * opin_rr_node_grid_side: specify the side of the output pins on which side of a GRID  <0..num_sides-1><0..num_opin_rr_nodes-1>
 * num_reserved_conf_bits: number of reserved configuration bits this switch block requires (mainly due to RRAM-based multiplexers)
 * num_conf_bits: number of configuration bits this switch block requires
 *******************************************************************/
class RRGSB {
  public:    /* Contructors */
    RRGSB(); /* Default constructor */
  public:    /* Accessors */
    /* Get the number of sides of this SB */
    size_t get_num_sides() const;

    /* Get the number of routing tracks on a side */
    size_t get_chan_width(const e_side& side) const;

    /* Get the type of routing tracks on a side */
    t_rr_type get_chan_type(const e_side& side) const;

    /* Get the maximum number of routing tracks on all sides */
    size_t get_max_chan_width() const;

    /* Get the number of routing tracks of a X/Y-direction CB */
    size_t get_cb_chan_width(const t_rr_type& cb_type) const;

    /* Return read-only object of the routing channels with a given side */
    const RRChan& chan(const e_side& chan_side) const;

    /* Get the sides of CB ipins in the array */
    std::vector<enum e_side> get_cb_ipin_sides(const t_rr_type& cb_type) const;
    /* Get the sides of CB opins in the array, OPINs can only be at the same sides of IPINs. Differently, they are inputs to a connection block */
    std::vector<enum e_side> get_cb_opin_sides(const t_rr_type& cb_type) const;

    /* Get the direction of a rr_node at a given side and track_id */
    enum PORTS get_chan_node_direction(const e_side& side, const size_t& track_id) const;

    /* Get a list of segments used in this routing channel */
    std::vector<RRSegmentId> get_chan_segment_ids(const e_side& side) const;

    /* Get a list of segments used in this routing channel */
    std::vector<size_t> get_chan_node_ids_by_segment_ids(const e_side& side,
                                                         const RRSegmentId& seg_id) const;

    /* get a rr_node at a given side and track_id */
    RRNodeId get_chan_node(const e_side& side, const size_t& track_id) const;

    /* get all the sorted incoming edges for a rr_node at a given side and track_id */
    std::vector<RREdgeId> get_chan_node_in_edges(const RRGraphView& rr_graph,
                                                 const e_side& side,
                                                 const size_t& track_id) const;

    /* get all the sorted incoming edges for a IPIN rr_node at a given side and ipin_id */
    std::vector<RREdgeId> get_ipin_node_in_edges(const RRGraphView& rr_graph,
                                                 const e_side& side,
                                                 const size_t& ipin_id) const;

    /* get the segment id of a channel rr_node */
    RRSegmentId get_chan_node_segment(const e_side& side, const size_t& track_id) const;

    /* Get the number of IPIN rr_nodes on a side */
    size_t get_num_ipin_nodes(const e_side& side) const;

    /* get a rr_node at a given side and track_id */
    RRNodeId get_ipin_node(const e_side& side, const size_t& node_id) const;

    /* Get the number of OPIN rr_nodes on a side */
    size_t get_num_opin_nodes(const e_side& side) const;
    /* Get the number of OPIN rr_nodes on a side of a connection block */
    size_t get_num_cb_opin_nodes(const t_rr_type& cb_type, const e_side& side) const;

    /* get a rr_node at a given side and track_id */
    RRNodeId get_opin_node(const e_side& side, const size_t& node_id) const;
    /* get a rr_node at a given side and track_id for a connection block */
    RRNodeId get_cb_opin_node(const t_rr_type& cb_type, const e_side& side, const size_t& node_id) const;

    int get_cb_chan_node_index(const t_rr_type& cb_type, const RRNodeId& node) const;

    int get_chan_node_index(const e_side& node_side, const RRNodeId& node) const;

    /* Get the node index in the array, return -1 if not found */
    int get_node_index(const RRGraphView& rr_graph, const RRNodeId& node, const e_side& node_side, const PORTS& node_direction) const;

    /* Given a rr_node, try to find its side and index in the Switch block */
    void get_node_side_and_index(const RRGraphView& rr_graph, const RRNodeId& node, const PORTS& node_direction, e_side& node_side, int& node_index) const;

    /* Check if the node exist in the opposite side of this Switch Block */
    bool is_sb_node_exist_opposite_side(const RRGraphView& rr_graph, const RRNodeId& node, const e_side& node_side) const;

  public: /* Accessors: to identify mirrors */
    /* check if the connect block exists in the GSB */
    bool is_cb_exist(const t_rr_type& cb_type) const;

    /* check if the switch block exists in the GSB, this function checks if a switch block physically exists (no routing wires, no OPIN nodes, and no interconnecting wires) */
    bool is_sb_exist(const RRGraphView& rr_graph) const;

    /* Check if the node imply a short connection inside the SB, which happens to long wires across a FPGA fabric */
    bool is_sb_node_passing_wire(const RRGraphView& rr_graph, const e_side& node_side, const size_t& track_id) const;

    /* check if the candidate SB satisfy the basic requirements
     * on being a mirror of the current one
     */
    bool is_sb_mirrorable(const RRGraphView& rr_graph, const RRGSB& cand) const;

  public:                                                                 /* Cooridinator conversion and output  */
    size_t get_x() const;                                                 /* get the x coordinate of this switch block */
    size_t get_y() const;                                                 /* get the y coordinate of this switch block */
    size_t get_sb_x() const;                                              /* get the x coordinate of this switch block */
    size_t get_sb_y() const;                                              /* get the y coordinate of this switch block */
    vtr::Point<size_t> get_sb_coordinate() const;                         /* Get the coordinate of the SB */
    size_t get_cb_x(const t_rr_type& cb_type) const;                      /* get the x coordinate of this X/Y-direction block */
    size_t get_cb_y(const t_rr_type& cb_type) const;                      /* get the y coordinate of this X/Y-direction block */
    vtr::Point<size_t> get_cb_coordinate(const t_rr_type& cb_type) const; /* Get the coordinate of the X/Y-direction CB */
    e_side get_cb_chan_side(const t_rr_type& cb_type) const;              /* get the side of a Connection block */
    e_side get_cb_chan_side(const e_side& ipin_side) const;               /* get the side of a Connection block */
    vtr::Point<size_t> get_side_block_coordinate(const e_side& side) const;
    vtr::Point<size_t> get_grid_coordinate() const;

  public: /* Mutators */
    /* get a copy from a source */
    void set(const RRGSB& src);
    void set_coordinate(const size_t& x, const size_t& y);

    /* Allocate the vectors with the given number of sides */
    void init_num_sides(const size_t& num_sides);

    /* Add a node to the chan_rr_node_ list and also
     * assign its direction in chan_rr_node_direction_
     */
    void add_chan_node(const e_side& node_side,
                       const RRChan& rr_chan,
                       const std::vector<enum PORTS>& rr_chan_dir);

    /* Add a node to the chan_rr_node_ list and also
     * assign its direction in chan_rr_node_direction_
     */
    void add_ipin_node(const RRNodeId& node,
                       const e_side& node_side);

    /* Add a node to the chan_rr_node_ list and also
     * assign its direction in chan_rr_node_direction_
     */
    void add_opin_node(const RRNodeId& node,
                       const e_side& node_side);

    /* Sort all the incoming edges for routing channel rr_node */
    void sort_chan_node_in_edges(const RRGraphView& rr_graph);
    /* Sort all the incoming edges for input pin rr_node */
    void sort_ipin_node_in_edges(const RRGraphView& rr_graph);
    /* Build the lists of opin node for connection blocks. This is required after adding all the nodes */
    void build_cb_opin_nodes(const RRGraphView& rr_graph);

  public: /* Mutators: cleaners */
    void clear();

    /* Clean the chan_width of a side */
    void clear_chan_nodes(const e_side& node_side);

    /* Clean the number of IPINs of a side */
    void clear_ipin_nodes(const e_side& node_side);

    /* Clean the number of OPINs of a side */
    void clear_opin_nodes(const e_side& node_side);

    /* Clean chan/opin/ipin nodes at one side */
    void clear_one_side(const e_side& node_side);

  private: /* Private Mutators: edge sorting */
    /* Sort all the incoming edges for one channel rr_node */
    void sort_chan_node_in_edges(const RRGraphView& rr_graph,
                                 const e_side& chan_side,
                                 const size_t& track_id);

    /* Sort all the incoming edges for one input pin rr_node */
    void sort_ipin_node_in_edges(const RRGraphView& rr_graph,
                                 const e_side& chan_side,
                                 const size_t& ipin_id);
  private: /* internal functions */
    size_t get_track_id_first_short_connection(const RRGraphView& rr_graph, const e_side& node_side) const;

  private: /* internal validators */
    bool validate_num_sides() const;
    bool validate_side(const e_side& side) const;
    bool validate_track_id(const e_side& side, const size_t& track_id) const;
    bool validate_cb_opin_node_id(const t_rr_type& cb_type, const e_side& side, const size_t& node_id) const;
    bool validate_opin_node_id(const e_side& side, const size_t& node_id) const;
    bool validate_ipin_node_id(const e_side& side, const size_t& node_id) const;
    bool validate_cb_type(const t_rr_type& cb_type) const;
    size_t get_cb_opin_type_id(const t_rr_type& cb_type) const;

  private: /* Internal Data */
    /* Coordinator */
    vtr::Point<size_t> coordinate_;

    /* Routing channel data
     * Each GSB may have four sides of routing track nodes
     */
    /* Node id in rr_graph denoting each routing track */
    std::vector<RRChan> chan_node_;

    /* Direction of a port when the channel node appear in the GSB module */
    std::vector<std::vector<PORTS>> chan_node_direction_;

    /* Sequence of edge ids for each routing channel node,
     * this is sorted by the location of edge source nodes in the context of GSB
     * The edge sorting is critical to uniquify the routing modules in OpenFPGA
     * This is due to that VPR allocate and sort edges randomly when building the rr_graph
     * As a result, previous nodes of a chan node may be the same in different GSBs
     * but their sequence is not. This will cause graph comparison to fail when uniquifying
     * the routing modules. Therefore, edge sorting can be done inside the GSB
     *
     * Storage organization:
     *   [chan_side][chan_node][edge_id_in_gsb_context]
     */
    std::vector<std::vector<std::vector<RREdgeId>>> chan_node_in_edges_;
    /* Sequence of edge ids for each input pin node. Same rules applied as the channel nodes */
    std::vector<std::vector<std::vector<RREdgeId>>> ipin_node_in_edges_;

    /* Logic Block Inputs data */
    std::vector<std::vector<RRNodeId>> ipin_node_;

    /* Logic Block Outputs data */
    std::vector<std::vector<RRNodeId>> opin_node_;
    /* Logic block outputs which directly drive IPINs in connection block,
     * CBX -> array[0], CBY -> array[1]
     * Each CB may have OPINs from all sides
     */
    std::array<std::array<std::vector<RRNodeId>, NUM_2D_SIDES>, 2> cb_opin_node_;
};

#endif
