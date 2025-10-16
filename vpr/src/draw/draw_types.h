#pragma once
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

#ifndef NO_GRAPHICS

#include <vector>
#include <memory>
#include "timing_info_fwd.h"
#include "vtr_util.h"
#include "vpr_types.h"
#include "vtr_color_map.h"
#include "vtr_vector.h"
#include "breakpoint.h"
#include "manual_moves.h"

#include "ezgl/rectangle.hpp"
#include "ezgl/color.hpp"
#include "ezgl/application.hpp"
#include "draw_color.h"

/// @brief Whether to draw routed nets or flylines (direct lines between sources and sinks).
enum e_draw_nets {
    DRAW_ROUTED_NETS,
    DRAW_FLYLINES
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

/// Chanx to chany or vice versa?
enum class e_chan_edge_dir {
    FROM_X_TO_Y,
    FROM_Y_TO_X
};

/// From inter-cluster pin to intra-cluster pin or vice versa?
enum e_pin_edge_dir {
    FROM_INTER_CLUSTER_TO_INTRA_CLUSTER,
    FROM_INTRA_CLUSTER_TO_INTER_CLUSTER
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

/// Different types of edges in the routing resource graph. Used to determine color of edges when drawn.
enum class e_edge_type {
    PIN_TO_OPIN,  // any pin to output pin
    PIN_TO_IPIN,  // any pin to input pin
    OPIN_TO_CHAN, // output pin to channel
    CHAN_TO_IPIN, // channel to input pin
    CHAN_TO_CHAN, // channel to channel
    NUM_EDGE_TYPES
};

/// Maps edge types to the color with which they are visualized
inline const vtr::array<e_edge_type, ezgl::color, (size_t)e_edge_type::NUM_EDGE_TYPES> EDGE_COLOR_MAP{
    ezgl::LIGHT_PINK,
    ezgl::MEDIUM_PURPLE,
    ezgl::PINK,
    ezgl::PURPLE,
    blk_DARKGREEN};

/**
 * @brief Structure used to hold data passed into the toggle checkbox callback function.
 */
typedef struct {
    ezgl::application* app; // Pointer to the ezgl application instance
    bool* toggle_state;     // Pointer to the boolean variable that will be toggled
} t_checkbox_data;

/**
 * @brief Structure used to store the state information of an rr_node. 
 * Used to control drawing each rr_node when ROUTING is on screen.
 * color: Color of the rr_node
 * node_highlighted: Whether the node is highlighted. Useful for highlighting routing resources on rr_graph.
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

struct PartialPlacement;

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
    e_pic_type pic_on_screen = e_pic_type::NO_PICTURE;

    ///@brief Whether to draw nets or not
    bool show_nets = false;

    ///@brief Whether to draw flylines or routed nets
    e_draw_nets draw_nets = DRAW_FLYLINES;

    ///@brief Whether to show inter-cluster nets
    bool draw_inter_cluster_nets = false;

    /// @brief Whether to show intra-cluster nets
    bool draw_intra_cluster_nets = false;

    ///@brief Whether to show block fan-in and fan-out
    bool highlight_fan_in_fan_out = false;

    ///@brief Whether to show crit path
    bool show_crit_path = false;

    bool show_crit_path_flylines = false;
    bool show_crit_path_delays = false;
    bool show_crit_path_routing = false;

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

    int show_routing_bb = UNDEFINED;

    ///@brief toggles whether routing util is shown
    e_draw_routing_util show_routing_util = DRAW_NO_ROUTING_UTIL;

    ///@brief Controls drawing of routing resources on screen.
    bool show_rr = false;

    bool draw_channel_nodes = false;
    bool draw_inter_cluster_pins = false;
    bool draw_switch_box_edges = false;
    bool draw_connection_box_edges = false;
    bool draw_intra_cluster_nodes = false;
    bool draw_intra_cluster_edges = false;
    bool highlight_rr_edges = false;

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
    e_route_type draw_route_type = e_route_type::GLOBAL;

    ///@brief default screen message on screen
    char default_message[vtr::bufsize];

    ///@brief color in which each net should be drawn. [0..cluster_ctx.clb_nlist.nets().size()-1]
    vtr::vector<ParentNetId, ezgl::color> net_color;

    ///@brief RR Nodes which the user has clicked on.
    std::set<RRNodeId> hit_nodes;

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

    ///@brief net transparency factor (0 - Transparent, 255 - Opaque)
    int net_alpha = 255;

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

    ezgl::color block_color(ClusterBlockId blk) const;
    void set_block_color(ClusterBlockId blk, ezgl::color color);
    void reset_block_color(ClusterBlockId blk);
    void reset_block_colors();

    ///@brief Refresh graphics resources' size after update to cluster blocks size
    void refresh_graphic_resources_after_cluster_change();

    std::vector<std::pair<t_pl_loc, ezgl::color>> colored_locations;

    /// @brief Stores UI checkbox struct for passing into the callback functions
    std::list<t_checkbox_data> checkbox_data;

    /**
     * @brief Set the reference to placement location variable.
     *
     * @details During the placement stage, this reference should point to a
     * local object in the placement stage because the placement stage does not change
     * the global stage in place_ctx until the end of placement. After the placement
     * is done, the reference should point to the global state stored in place_ctx.
     *
     * @param blk_loc_registry The PlaceLocVars that the reference will point to.
     */
    void set_graphics_blk_loc_registry_ref(const BlkLocRegistry& blk_loc_registry) {
        blk_loc_registry_ref_ = std::ref(blk_loc_registry);
    }

    /**
     * @brief Returns the reference to placement block location variables.
     * @return A const reference to placement block location variables.
     */
    const BlkLocRegistry& get_graphics_blk_loc_registry_ref() const {
        return blk_loc_registry_ref_->get();
    }

    /**
     * @brief Set the internal reference to NoC link bandwidth utilization array.
     * @param noc_link_bandwidth_usages The array that the internal reference will
     * be pointing to.
     */
    void set_noc_link_bandwidth_usages_ref(const vtr::vector<NocLinkId, double>& noc_link_bandwidth_usages) {
        noc_link_bandwidth_usages_ref_ = noc_link_bandwidth_usages;
    }

    /**
     * @brief Returns the reference to NoC link bandwidth utilization array.
     * @return A const reference to NoC link bandwidth utilization array.
     */
    const vtr::vector<NocLinkId, double>& get_noc_link_bandwidth_usages_ref() const {
        return noc_link_bandwidth_usages_ref_->get();
    }

  private:
    friend void alloc_draw_structs(const t_arch* arch);
    vtr::vector<ClusterBlockId, ezgl::color> block_color_;
    vtr::vector<ClusterBlockId, bool> use_default_block_color_;

    /**
     * @brief Stores a reference to a BlkLocRegistry to be used in the graphics code.
     * @details This reference let us pass in a currently-being-optimized placement state,
     * rather than using the global placement state in placement context that is valid only once placement is done
     */
    std::optional<std::reference_wrapper<const BlkLocRegistry>> blk_loc_registry_ref_;

    /**
     * @brief Stores a reference to NoC link bandwidth utilization to be used in the graphics codes.
     */
    std::optional<std::reference_wrapper<const vtr::vector<NocLinkId, double>>> noc_link_bandwidth_usages_ref_;

    /**
     * @brief Stores a temporary reference to the Analytical Placement partial placement (best placement).
     * @details This is set by the AP global placer just before drawing and cleared immediately after.
     *          Only a reference is stored to avoid copying and lifetime issues.
     */
    std::optional<std::reference_wrapper<const PartialPlacement>> ap_partial_placement_ref_;

public:
    // Set/clear/get the AP partial placement reference used during AP drawing
    void set_ap_partial_placement_ref(const PartialPlacement& p) { ap_partial_placement_ref_ = std::cref(p); }
    void clear_ap_partial_placement_ref() { ap_partial_placement_ref_.reset(); }
    const PartialPlacement* get_ap_partial_placement_ref() const { return ap_partial_placement_ref_ ? &ap_partial_placement_ref_->get() : nullptr; }
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
    std::vector<float> tile_x, tile_y;

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

    ///@brief Sets the tile width
    inline void set_tile_width(float new_tile_width) {
        tile_width = new_tile_width;
    }

    ///@brief returns tile width
    float get_tile_width() const;

    ///@brief returns tile width
    float get_tile_height() const;

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

    /**
     * @brief returns a 2D point for the absolute location of the given pb_graph_pin in the entire graphics context.
     * @param clb_index The index of the cluster block containing the pin.
     * @param pb_graph_pin The pin for which the absolute location is requested.
     * @return A point2d representing the absolute coordinates of the pin.
     */
    ezgl::point2d get_absolute_pin_location(const ClusterBlockId clb_index, const t_pb_graph_pin* pb_graph_pin);

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
    friend void init_draw_coords(float width_val, const BlkLocRegistry& blk_loc_registry);
};

#endif // NO_GRAPHICS
