/**
 * @file draw_types.h
 * 
 * This file contains declarations of structures and types shared by all drawing
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
 * Author: Long Yu (Mike) Wang, Sebastian Lievano
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
#    include "manual_moves.h"

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
    DRAW_CLUSTER_NETS,
    DRAW_PRIMITIVE_NETS
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

/*
 * Defines the type of drawings that can be generated for the NoC.
 * DRAW_NO_NOC -> user did not select the option to draw the NoC
 * DRAW_NOC_LINKS -> display the NoC links and how they are connected to each other
 * DRAW_NOC_LINK_USAGE -> Display the NoC links (same as DRAW_NOC_LINKS) and color the links based on their bandwidth usage
 */
enum e_draw_noc {
    DRAW_NO_NOC = 0,
    DRAW_NOC_LINKS,
    DRAW_NOC_LINK_USAGE
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

/**
 * @brief Structure used to store visibility and transparency state information for a specific layer (die) in the FPGA.
 *        This structure is also used to store the state information of the cross-layer connections option in the UI.
 */
struct t_draw_layer_display {
    ///@brief Whether the current layer should be visible.
    bool visible = false;

    ///@brief Transparency value ( 0 - transparent, 255 - Opaque)
    ///@note The UI has the opposite definition to make it more intuitive for the user,
    /// where increasing the value increases transparency. (255 - transparent, 0 - Opaque)
    int alpha = 255;
};

/**
 * @brief Structure used to store variables related to highlighting/drawing
 * 
 * Stores a lot of different variables to reflect current draw state. Most callback functions/UI elements
 * mutate some member in this struct, which then alters the draw state. Accessible through
 * global function get_draw_state_vars() in draw_global.cpp. It is recommended to name the variable draw_state for consistent form.
 * 
 * @note t_draw_state is used in the same way as a Context, but cannot be a Context because Contexts are
 * not copyable, while t_draw_state must be. (t_draw_state is copied to save a restore of the graphics state
 * when running graphics commands.)
 */
struct t_draw_state {
    ///@brief What to draw on the screen (ROUTING, PLACEMENT, NO_PICTURE)
    pic_type pic_on_screen = NO_PICTURE;

    ///@brief Whether to show nets at placement and routing.
    e_draw_nets show_nets = DRAW_NO_NETS;

    ///@brief Whether to show crit path
    e_draw_crit_path show_crit_path = DRAW_NO_CRIT_PATH;

    ///@brief Controls if congestion is shown, when ROUTING is on screen.
    e_draw_congestion show_congestion = DRAW_NO_CONGEST;

    ///@brief Controls if routing congestion costs are shown, when ROUTING is on screen.
    e_draw_routing_costs show_routing_costs;

    ///@brief Toggles whether block pin util is shown
    e_draw_block_pin_util show_blk_pin_util = DRAW_NO_BLOCK_PIN_UTIL;

    ///@brief Toggles whether router expansion cost is shown
    e_draw_router_expansion_cost show_router_expansion_cost = DRAW_NO_ROUTER_EXPANSION_COST;

    ///@brief Toggles whether placement macros are shown
    e_draw_placement_macros show_placement_macros = DRAW_NO_PLACEMENT_MACROS;

    int show_routing_bb = OPEN;

    ///@brief toggles whether routing util is shown
    e_draw_routing_util show_routing_util = DRAW_NO_ROUTING_UTIL;

    ///@brief Controls drawing of routing resources on screen, if pic_on_screen is ROUTING.
    e_draw_rr_toggle draw_rr_toggle = DRAW_NO_RR;

    ///@brief Whether routing util is shown
    bool clip_routing_util = false;

    ///@brief Boolean that toggles block outlines are shown
    bool draw_block_outlines = true;

    ///@brief Boolean that toggles block names
    bool draw_block_text = true;

    ///@brief Boolean that toggles showing partitions
    bool draw_partitions = false;

    ///@brief integer value for net max fanout
    int draw_net_max_fanout = std::numeric_limits<int>::max();

    ///@brief The maximum number of sub-block levels among all physical block types in the FPGA.
    int max_sub_blk_lvl = 0;

    ///@brief If 0, no internal drawing is shown. Otherwise, indicates how many levels of sub-pbs to be drawn
    int show_blk_internal = 0;

    ///@brief Whether graphics are enabled
    bool show_graphics = false;

    ///@brief How often is user input required. (0: each t, 1: each place, 2: never)
    int gr_automode = 0;

    ///@brief Should we automatically finish drawing (instead of waiting in the event loop for user interaction?
    bool auto_proceed = false;

    ///@brief GLOBAL or DETAILED
    e_route_type draw_route_type = GLOBAL;

    ///@brief default screen message on screen
    char default_message[vtr::bufsize];

    ///@brief color in which each net should be drawn. [0..cluster_ctx.clb_nlist.nets().size()-1]
    vtr::vector<ClusterNetId, ezgl::color> net_color;

    /**
     * @brief stores the state information of each routing resource.
     * 
     * Used to control drawing each routing resource when
     * ROUTING is on screen.
     * [0..device_ctx.rr_nodes.size()-1]
     */
    vtr::vector<RRNodeId, t_draw_rr_node> draw_rr_node;

    std::shared_ptr<const SetupTimingInfo> setup_timing_info;

    ///@brief pointer to architecture info. const
    const t_arch* arch_info = nullptr;

    std::shared_ptr<const vtr::ColorMap> color_map = nullptr;

    ///@brief Whether to generate an output graphics file
    bool save_graphics = false;

    std::string graphics_commands;

    ///@brief If we should pause for user interaction (requested by user)
    bool forced_pause = false;

    int sequence_number = 0;
    float net_alpha = 0.1;

    ///@brief Present congestion cost factor used when drawing. Is a copy of router's current pres_fac
    float pres_fac = 1.;

    ManualMovesState manual_moves_state;
    bool is_flat = false;

    ///@brief Whether we are showing the NOC button
    bool show_noc_button = false;

    ///@brief Draw state for NOC drawing
    e_draw_noc draw_noc = DRAW_NO_NOC;

    std::shared_ptr<const vtr::ColorMap> noc_usage_color_map = nullptr; // color map used to display noc link bandwidth usage

    ///Tracks autocomplete enabling.
    bool justEnabled = false;

    std::vector<Breakpoint> list_of_breakpoints;

    ///@brief Stores visibility and transparency drawing controls for each layer [0 ... grid.num_layers -1]
    std::vector<t_draw_layer_display> draw_layer_display;

    ///@brief Visibility and transparency for elements that cross die layers
    t_draw_layer_display cross_layer_display;

    ///@brief base of save graphics file name (i.e before extension)
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

    std::vector<std::pair<t_pl_loc, ezgl::color>> colored_locations;

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

/**
 * @brief Global Struct that stores drawn coords/sizes of grid blocks/logic blocks
 * 
 * Structure used to store coordinates and dimensions for
 * grid tiles and logic blocks in the FPGA. Accessible through the global
 * function get_draw_coords_vars().
 */
struct t_draw_coords {
    /**
     * @brief Form the axes of the chips coordinate system
     * 
     * tile_x and tile_y form two axes that make a
     * COORDINATE SYSTEM for grid_tiles, which goes from
     * (tile_x[0],tile_y[0]) at the lower left corner of the FPGA
     * to (tile_x[device_ctx.grid.width()-1]+tile_width, tile_y[device_ctx.grid.height()-1]+tile_width) in
     * the upper right corner.
     */
    float *tile_x, *tile_y;

    ///@brief Half-width or Half-height of a pin. Set when init_draw_coords is called
    float pin_size;

    /**
     * @brief stores drawing information for different block types
     * 
     * a list of drawing information for each type of
     * block, one for each type. Access it with
     * cluster_ctx.clb_nlist.block_type(block_id)->index
     */
    std::vector<t_draw_pb_type_info> blk_info;

    ///@brief constructor
    t_draw_coords();

    ///@brief returns tile width
    float get_tile_width();

    ///@brief returns tile width
    float get_tile_height();

    ///@brief returns bounding box for given pb in given clb
    ezgl::rectangle get_pb_bbox(ClusterBlockId clb_index, const t_pb_graph_node& pb_gnode);

    ///@brief returns bounding box of sub block at given location of given type w. given pb
    ezgl::rectangle get_pb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr type, const t_pb_graph_node& pb_gnode);

    ///@brief returns pb of sub block of given idx/given type at location
    ezgl::rectangle get_pb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr type);

    /**
     * @brief returns a bounding box for the given pb in the given
     * clb with absolute coordinates, that can be directly drawn.
     */
    ezgl::rectangle get_absolute_pb_bbox(const ClusterBlockId clb_index, const t_pb_graph_node* pb_gnode);

    ///@brief Returns bounding box for CLB of given idx/type
    ezgl::rectangle get_absolute_clb_bbox(const ClusterBlockId clb_index, const t_logical_block_type_ptr type);

    /**
     * @brief Returns a bounding box for the clb at device_ctx.grid[grid_x][grid_y].blocks[sub_block_index],
     * even if it is empty.
     */
    ezgl::rectangle get_absolute_clb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index);

    /**
     * @brief Returns a bounding box for the clb at device_ctx.grid[grid_x][grid_y].blocks[sub_block_index],
     * of given type even if it is empty.
     */
    ezgl::rectangle get_absolute_clb_bbox(int grid_layer, int grid_x, int grid_y, int sub_block_index, const t_logical_block_type_ptr block_type);

  private:
    float tile_width;
    friend void init_draw_coords(float);
};

#endif // NO_GRAPHICS

#endif
