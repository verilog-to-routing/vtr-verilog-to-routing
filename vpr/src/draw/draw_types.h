/* This file contains declarations of structures and types shared by all drawing
 * routines.
 *
 * Key structures:
 *  t_draw_coords - holds coordinates and dimensions for each grid tile and each
 *					logic block
 *  t_draw_state - holds variables that control drawing modes based on user input
 *				   (eg. clicking on the menu buttons)
 *               - holds state variables that control drawing and highlighting of
 *				   architectural elements on the FPGA chip
 *
 * Author: Long Yu (Mike) Wang
 * Date: August 20, 2013
 */

#ifndef DRAW_TYPES_H
#define DRAW_TYPES_H

#ifndef NO_GRAPHICS

#    include <vector>
#    include <memory>
#    include "clustered_netlist.h"
#    include "timing_info_fwd.h"
#    include "vtr_util.h"
#    include "vpr_types.h"
#    include "vtr_color_map.h"
#    include "vtr_vector.h"
#    include "breakpoint.h"

#    include "ezgl/point.hpp"
#    include "ezgl/rectangle.hpp"
#    include "ezgl/color.hpp"

enum e_draw_crit_path {
    DRAW_NO_CRIT_PATH,
    DRAW_CRIT_PATH_FLYLINES,
    DRAW_CRIT_PATH_FLYLINES_DELAYS,
    DRAW_CRIT_PATH_ROUTING,
    DRAW_CRIT_PATH_ROUTING_DELAYS
};

enum e_draw_nets {
    DRAW_NO_NETS = 0,
    DRAW_NETS,
    DRAW_LOGICAL_CONNECTIONS
};

/* Draw rr_graph from less detailed to more detailed
 * in order to speed up drawing when toggle_rr is clicked
 * on for the first time.
 */
enum e_draw_rr_toggle {
    DRAW_NO_RR = 0,
    DRAW_NODES_RR,
    DRAW_NODES_SBOX_RR,
    DRAW_NODES_SBOX_CBOX_RR,
    DRAW_NODES_SBOX_CBOX_INTERNAL_RR,
    DRAW_ALL_RR,
};

enum e_draw_congestion {
    DRAW_NO_CONGEST = 0,
    DRAW_CONGESTED,
    DRAW_CONGESTED_WITH_NETS,
};

enum e_draw_routing_costs {
    DRAW_NO_ROUTING_COSTS = 0,
    DRAW_TOTAL_ROUTING_COSTS,
    DRAW_LOG_TOTAL_ROUTING_COSTS,
    DRAW_ACC_ROUTING_COSTS,
    DRAW_LOG_ACC_ROUTING_COSTS,
    DRAW_PRES_ROUTING_COSTS,
    DRAW_LOG_PRES_ROUTING_COSTS,
    DRAW_BASE_ROUTING_COSTS,
};

enum e_draw_block_pin_util {
    DRAW_NO_BLOCK_PIN_UTIL = 0,
    DRAW_BLOCK_PIN_UTIL_TOTAL,
    DRAW_BLOCK_PIN_UTIL_INPUTS,
    DRAW_BLOCK_PIN_UTIL_OUTPUTS,
};

enum e_draw_routing_util {
    DRAW_NO_ROUTING_UTIL,
    DRAW_ROUTING_UTIL,
    DRAW_ROUTING_UTIL_WITH_VALUE,
    DRAW_ROUTING_UTIL_WITH_FORMULA,
    DRAW_ROUTING_UTIL_OVER_BLOCKS, //Draw over blocks at full opacity (useful for figure generation)
};

enum e_draw_router_expansion_cost {
    DRAW_NO_ROUTER_EXPANSION_COST,
    DRAW_ROUTER_EXPANSION_COST_TOTAL,
    DRAW_ROUTER_EXPANSION_COST_KNOWN,
    DRAW_ROUTER_EXPANSION_COST_EXPECTED,
    DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES,
    DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES,
    DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES,
};

enum e_draw_placement_macros {
    DRAW_NO_PLACEMENT_MACROS = 0,
    DRAW_PLACEMENT_MACROS,
};

enum e_draw_net_type {
    ALL_NETS,
    HIGHLIGHTED
};

/* Chanx to chany or vice versa? */
enum e_edge_dir {
    FROM_X_TO_Y,
    FROM_Y_TO_X
};

/* Structure which stores state information of a rr_node. Used
 * for controling the drawing each rr_node when ROUTING is on screen.
 * color: Color of the rr_node
 * node_highlighted: Whether the node is highlighted. Useful for
 *					 highlighting routing resources on rr_graph
 */
typedef struct {
    ezgl::color color;
    bool node_highlighted;
} t_draw_rr_node;

/* Structure used to store state variables that control drawing and
 * highlighting.
 * pic_on_screen: What to draw on the screen (PLACEMENT, ROUTING, or
 *				  NO_PICTURE).
 * show_nets: Whether to show nets at placement and routing.
 * show_congestion: Controls if congestion is shown, when ROUTING is
 *					on screen.
 * show_routing_costs: Controls if routing congestion costs are shown,
 *                     when ROUTING is on screen.
 * draw_rr_toggle: Controls drawing of routing resources on screen,
 *				   if pic_on_screen is ROUTING.
 * show_blk_internal: If 0, no internal drawing is shown. Otherwise,
 *					  indicates how many levels of sub-pbs to be drawn
 * max_sub_blk_lvl: The maximum number of sub-block levels among all
 *                  physical block types in the FPGA.
 * show_graphics: Whether graphics is enabled.
 * gr_automode: How often is user input required. (0: each t, 1: each place, 2: never)
 * auto_proceed: Should we automatically finish drawing (instead of waiting in the event
 *               loop for user interaction?
 * draw_route_type: GLOBAL or DETAILED
 * default_message: default screen message on screen
 * net_color: color in which each net should be drawn.
 *			  [0..cluster_ctx.clb_nlist.nets().size()-1]
 * block_color: color in which each blocks should be drawn.
 *			    [0..cluster_ctx.clb_nlist.blocks().size()-1]
 * draw_rr_node: stores the state information of each routing resource.
 *				 Used to control drawing each routing resource when
 *				 ROUTING is on screen.
 *				 [0..device_ctx.rr_nodes.size()-1]
 * save_graphics: Whether to generate an output graphcis file
 * force_pause: Should we pause for user interaction (since the user requested it)
 * save_graphics_file_base: Base of save graphis file name (i.e. before extension)
 * pres_fac: present congestion cost factor
 */
struct t_draw_state {
    pic_type pic_on_screen = NO_PICTURE;
    e_draw_nets show_nets = DRAW_NO_NETS;
    e_draw_crit_path show_crit_path = DRAW_NO_CRIT_PATH;
    e_draw_congestion show_congestion = DRAW_NO_CONGEST;
    e_draw_routing_costs show_routing_costs;
    e_draw_block_pin_util show_blk_pin_util = DRAW_NO_BLOCK_PIN_UTIL;
    e_draw_router_expansion_cost show_router_expansion_cost = DRAW_NO_ROUTER_EXPANSION_COST;
    e_draw_placement_macros show_placement_macros = DRAW_NO_PLACEMENT_MACROS;
    int show_routing_bb = OPEN;
    e_draw_routing_util show_routing_util = DRAW_NO_ROUTING_UTIL;
    e_draw_rr_toggle draw_rr_toggle = DRAW_NO_RR;
    bool clip_routing_util = false;
    bool draw_block_outlines = true;
    bool draw_block_text = true;
    int draw_net_max_fanout = std::numeric_limits<int>::max();
    int max_sub_blk_lvl = 0;
    int show_blk_internal = 0;
    bool show_graphics = false;
    int gr_automode = 0;
    bool auto_proceed = false;
    e_route_type draw_route_type = GLOBAL;
    char default_message[vtr::bufsize];
    vtr::vector<ClusterNetId, ezgl::color> net_color;
    t_draw_rr_node* draw_rr_node = nullptr;
    std::shared_ptr<const SetupTimingInfo> setup_timing_info;
    const t_arch* arch_info = nullptr;
    std::shared_ptr<const vtr::ColorMap> color_map = nullptr;
    bool save_graphics = false;
    std::string graphics_commands;
    bool forced_pause = false;
    int sequence_number = 0;
    float net_alpha = 0.1;
    float pres_fac = 1.;

    std::vector<Breakpoint> list_of_breakpoints;

    std::string save_graphics_file_base = "vpr";

    t_draw_state() = default;
    t_draw_state(const t_draw_state&) = default;
    t_draw_state& operator=(const t_draw_state&) = default;

    void reset_nets_congestion_and_rr();

    bool showing_sub_blocks();

    ezgl::color block_color(ClusterBlockId blk) const;
    void set_block_color(ClusterBlockId blk, ezgl::color color);
    void reset_block_color(ClusterBlockId blk);
    void reset_block_colors();

#    ifdef VTR_ENABLE_DEBUG_LOGGING
    std::vector<std::pair<t_pl_loc, ezgl::color>> colored_blocks;
#    endif
  private:
    friend void alloc_draw_structs(const t_arch* arch);
    vtr::vector<ClusterBlockId, ezgl::color> block_color_;
    vtr::vector<ClusterBlockId, bool> use_default_block_color_;
};

/* For each cluster type, this structure stores drawing
 * information for all sub-blocks inside. This includes
 * the bounding box for drawing each sub-block. */
struct t_draw_pb_type_info {
    std::vector<ezgl::rectangle> subblk_array;
    ezgl::rectangle get_pb_bbox(const t_pb_graph_node& pb_gnode);
    ezgl::rectangle& get_pb_bbox_ref(const t_pb_graph_node& pb_gnode);
};

/* Structure used to store coordinates and dimensions for
 * grid tiles and logic blocks in the FPGA.
 * tile_x and tile_y: together form two axes that make a
 * COORDINATE SYSTEM for grid_tiles, which goes from
 * (tile_x[0],tile_y[0]) at the lower left corner of the FPGA
 * to (tile_x[device_ctx.grid.width()-1]+tile_width, tile_y[device_ctx.grid.height()-1]+tile_width) in
 * the upper right corner.
 * tile_width: Width (and height) of a grid_tile.s
 *			 Set when init_draw_coords is called.
 * gap_size: distance of the gap between two adjacent
 *           clbs; the literal channel "width" .
 * pin_size: The half-width or half-height of a pin.
 *			 Set when init_draw_coords is called.
 * blk_info: a list of drawing information for each type of
 *           block, one for each type. Access it with
 *           cluster_ctx.clb_nlist.block_type(block_id)->index
 */
struct t_draw_coords {
    float *tile_x, *tile_y;
    float pin_size;

    std::vector<t_draw_pb_type_info> blk_info;
    t_draw_coords();

    float get_tile_width();
    float get_tile_height();

    /**
     * Retrieve the bounding box for the given pb in the given
     * clb, from this data structure
     */
    ezgl::rectangle get_pb_bbox(ClusterBlockId clb_index, const t_pb_graph_node& pb_gnode);
    ezgl::rectangle get_pb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr type, const t_pb_graph_node& pb_gnode);
    ezgl::rectangle get_pb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr type);

    /**
     * Return a bounding box for the given pb in the given
     * clb with absolute coordinates, that can be directtly drawn.
     */
    ezgl::rectangle get_absolute_pb_bbox(const ClusterBlockId clb_index, const t_pb_graph_node* pb_gnode);

    /**
     * Return a bounding box for the clb at device_ctx.grid[grid_x][grid_y].blocks[sub_block_index],
     * even if it is empty.
     */
    ezgl::rectangle get_absolute_clb_bbox(const ClusterBlockId clb_index, const t_logical_block_type_ptr type);
    ezgl::rectangle get_absolute_clb_bbox(int grid_x, int grid_y, int sub_block_index);

    ezgl::rectangle get_absolute_clb_bbox(int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr block_type);

  private:
    float tile_width;
    friend void init_draw_coords(float);
};

#endif // NO_GRAPHICS

#endif
