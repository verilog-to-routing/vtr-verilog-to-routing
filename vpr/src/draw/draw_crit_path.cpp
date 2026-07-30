#ifndef NO_GRAPHICS

#include <numbers>
#include <sstream>
#include <limits>
#include <array>

#include "draw_crit_path.h"
#include "draw.h"
#include "draw_basic.h"
#include "draw_global.h"
#include "draw_triangle.h"
#include "globals.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "ap_netlist_utils.h"

/**
 * @brief Scaling factor applied to DEFAULT_ARROW_SIZE.
 */
static constexpr int TIMING_EDGE_ARROW_SCALE = 30;

/**
 * @brief Indicates the relative position on a timing edge. 0 (start) and 1 (end) represent the two endpoints.
 */
static constexpr float EDGE_CENTER = 0.5;

/**
 * @brief Distance in pixels used to offset a delay label from the center, perpendicular to the edge.
 */
static constexpr int PERPENDICULAR_OFFSET = 13;

/**
 * @brief Fraction of the total edge length used to offset a delay label from the center, along the edge.
 */
static constexpr double EDGE_OFFSET_FRACTION = 0.1;

/**
 * @brief Maximum unit distance in pixels that a label can be offset from the center, along the edge.
 */
static constexpr int MAX_EDGE_OFFSET_UNIT = 40;

/**
 * @brief Size of the star drawn at the beginning and end of the critical path, in pixels.
 * 
 * Refers to the distance in pixels from the star center to the star tip.
 */
static constexpr int ENDPOINT_STAR_SIZE = 16;

/**
 * @brief Distance in pixels used to pad from the total delay meesages when drawing their background rectangle.
 * 
 * Used for both x and y-direction padding.
 */
static constexpr int PADDING = 5;

/**
 * @brief Highly contrasting colours that are useful for visualization.
 */
static const std::vector<ezgl::color> kelly_max_contrast_colors = {
    ezgl::color(34, 34, 34),    ///< black
    ezgl::color(243, 195, 0),   ///< yellow
    ezgl::color(135, 86, 146),  ///< purple
    ezgl::color(243, 132, 0),   ///< orange
    ezgl::color(190, 0, 50),    ///< red
    ezgl::color(194, 178, 128), ///< buf
    ezgl::color(132, 132, 130), ///< gray
    ezgl::color(0, 136, 86),    ///< green
    ezgl::color(230, 143, 172), ///< purplish pink
    ezgl::color(0, 103, 165),   ///< blue
    ezgl::color(249, 147, 121), ///< yellowish pink
    ezgl::color(96, 78, 151),   ///< violet
    ezgl::color(246, 166, 0),   ///< orange yellow
    ezgl::color(179, 68, 108),  ///< purplish red
    ezgl::color(220, 211, 0),   ///< greenish yellow
    ezgl::color(136, 45, 23),   ///< redish brown
    ezgl::color(141, 182, 0),   ///< yellow green
    ezgl::color(101, 69, 34),   ///< yellowish brown
    ezgl::color(226, 88, 34),   ///< reddish orange
    ezgl::color(43, 61, 38)     ///< olive green
};

/**
 * @brief Identifies which endpoint of a critical path needs to be drawn.
 */
enum class e_crit_path_endpoint_type {
    /// First node in the critical path.
    START,
    /// Last node in the critical path.
    END,
};

/**
 * @brief Candidate positions for placing a delay label relative to its timing-edge flyline.
 * 
 * For a flyline, we define start as the endpoint whose X coordinate is less, and end as the other.
 * Note: this convention only applies here. Other usage of start and end in this file may not follow this.
 * If the flyline is vertical, start and end are determined by the actual signal direction from src to sink.
 * Define a vector from start to end. ABOVE and BELOW are perpendicular offset from the vector. ABOVE goes towards
 * the vector's left hand side and BELOW goes towards the right. CENTER, LEFT and RIGHT are offset along the vector.
 * While CENTER means no offset, LEFT goes towards start and RIGHT goes towards end. The keyword FAR means doubling
 * the edgewise offset while keeping the same perpendicular offset. For example, if LEFT means 10 pixels
 * towards start, FAR_LEFT is 20 pixels towards start.
 */
enum class e_label_relative_pos {
    CENTER_ABOVE,
    CENTER_BELOW,
    LEFT_ABOVE,
    LEFT_BELOW,
    RIGHT_ABOVE,
    RIGHT_BELOW,
    FAR_LEFT_ABOVE,
    FAR_LEFT_BELOW,
    FAR_RIGHT_ABOVE,
    FAR_RIGHT_BELOW,
};

/**
 * @brief Contains all attributes of one timing edge delay label needed for drawing and finding a label position that minimizes overlaps.
 */
struct t_label_drawing_info {
    std::string delay_label_str;                 ///< Delay time across this timing edge in nanoseconds, represented in std::string and drawn on screen.
    bool hide_label = false;                     ///< True when the label is hidden due to overlaps.
    ezgl::color label_color = {0, 0, 0};         ///< Color used when drawing the label.
    int label_transparency = 0;                  ///< Alpha value used when drawing the label.
    double edge_length = 0.0;                    ///< Length of the associated timing-edge flyline in world units.
    double rotation_angle = 0.0;                 ///< Label rotation angle (the same as the associated flyline) in degrees.
    ezgl::rectangle virtual_centered_label_bbox; ///< A virtual label bounding box centered on the timing edge before offsets are applied.
    ezgl::rectangle label_bbox;                  ///< Final label bounding box after position offsets are applied.
};

/**
 * @brief A pair of timing node Ids used to uniquely identify a timing edge.
 */
struct t_timing_edge_id {
    tatum::NodeId src;  ///< source of the timing edge.
    tatum::NodeId sink; ///< sink of the timing edge.

    bool operator==(const t_timing_edge_id& other) const {
        return src == other.src && sink == other.sink;
    }
};

/**
 * @brief Draws dashed flylines between consecutive nodes in the provided timing path, and draws a star at the beginning and end of the paths.
 *
 * @param paths Timing paths whose consecutive node pairs define the flylines.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_timing_edge_flylines(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g);

/**
 * @brief Fills a star at one endpoint of a critical path and draws the critical path index at the center.
 *
 * The endpoint type (START or END) determines the star color.
 * The critical path index drawn at the star center is useful when multiple critical paths
 * are drawn simultaneously and we want to distinguish them.
 *
 * @param endpoint_type Whether the endpoint is the start or end of the critical path.
 * @param endpoint_coords Drawing coordinates of the endpoint in world units.
 * @param path_idx Index of the critical path (0 when only the worst path is chosen).
 * @param g Renderer used to perform drawing.
 */
static void draw_crit_path_endpoint(e_crit_path_endpoint_type endpoint_type, ezgl::point2d endpoint_coords, std::size_t path_idx, ezgl::renderer* g);

/**
 * @brief Draws routed connections for consecutive nodes in the provided timing paths.
 *
 * @param paths Timing paths whose consecutive node pairs define the connections.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_routed_timing_connections(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g);

/**
 * @brief Draws the routed connection between two timing graph nodes.
 * 
 * Note: this function is used in both non-server and server mode, unlike flyline drawing
 * which has a unique function dedicated to each mode.
 * The type of drawn connections depends on whether flat routing is on. When off, only the inter-cluster
 * connections are drawn. When on, there can also be intra-cluster connections besides the inter-cluster ones.
 *
 * @param src_tnode Source timing graph node for the connection.
 * @param sink_tnode Sink timing graph node for the connection.
 * @param color Color used to draw the routed connection.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_routed_connections_between_nodes(tatum::NodeId src_tnode, tatum::NodeId sink_tnode, ezgl::color color, ezgl::renderer* g);

/**
 * @brief Draws the routed connection between an atom sink pin and its driver pin in a flat (atom) netlist. Used when flat routing is enabled.
 *
 * Note: we do not pass in atom_src_pin because what we eventually need to get the routed connection
 * are the driver net pin id (src pin) and sink net pin id in the net they form, but the driver net pin id is always 0.
 * 
 * @param atom_sink_pin Atom sink pin whose routed connection should be drawn.
 * @param color Color used to highlight the routed connection.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_connections_from_atom_netlist(AtomPinId atom_sink_pin, ezgl::color color, ezgl::renderer* g);

/**
 * @brief Draws the routed connection between the two clbs associated with the atom src pin and sink pin through the clustered netlist.
 *
 * Note: if the connection is entirely internal to a cluster, nothing is drawn.
 * Here, we need to pass in atom_src_pin because we want to know the clb that atom_src_pin belongs to so that
 * we can correctly draw the inter-cluster routed connection.
 * 
 * @param atom_src_pin Atom source pin of the routed connection to be drawn.
 * @param atom_sink_pin Atom sink pin of the routed connection to be drawn.
 * @param color Color used to highlight the routed connection.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_connections_from_cluster_netlist(AtomPinId atom_src_pin, AtomPinId atom_sink_pin, ezgl::color color, ezgl::renderer* g);

#ifndef NO_SERVER

static void draw_server_mode_flylines_and_labels(ezgl::point2d start, ezgl::point2d end, float incr_delay, ezgl::renderer* g, bool skip_draw_delays /*=false*/);

#endif /* NO_SERVER */

/**
 * @brief Greedily calculate delay label positions for the least number of overlaps and then draws all visible labels.
 *
 * Note: when multiple paths are provided and some of them share the same timing edge segments, only one label will be added
 * to the calculation for each segment, and the others are discarded.
 * 
 * @param paths Timing paths whose consecutive node pairs define the timing edges to place delay labels.
 * @param g Pointer to the ezgl::renderer object.
 */
static void calculate_and_draw_delay_labels(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g);

static std::size_t get_num_edges_of_multi_paths(const std::vector<tatum::TimingPath>& paths);

/**
 * @brief Calculates delay, visibility, rotation, and initial bounding-box information for each timing edge.
 * 
 * Calculated results are stored in a vector and returned by the function.
 *
 * @param path Timing path whose consecutive node pairs define the timing edges to place delay labels.
 * @param pixels_per_world_unit Ratio between pixels and world units spanning the screen width.
 * Used to perform screen-to-world conversions for label bounding boxes that primarily use pixels.
 * @param g Pointer to the ezgl::renderer object. Used to get the dimension of the delay label string in pixels.
 * @return Per-edge delay label drawing information that does not yet tell where each label will be eventually drawn.
 */
static std::vector<t_label_drawing_info> calculate_basic_label_drawing_info(const std::vector<tatum::TimingPath>& paths,
                                                                            double pixels_per_world_unit,
                                                                            ezgl::renderer* g);

/**
 * @brief Chooses label positions in a way that greedily tries to minimize overlaps among visible labels.
 *
 * There can be cases where no position candidate of a label satisfies zero overlap. The algorithm will still choose the
 * least overlapped position (in a greedy perspective) instead of hiding the label, because its presence will positively affect
 * the placement of subsequent labels. Inevitable overlaps will be eventually handled by hide_still_cluttered_labels().
 * Chosen bounding box positions are updated to basic_label_drawing_info.
 * 
 * @param basic_label_drawing_info Basic per-edge label drawing information needed to perform the decluttering algorithm.
 * @param pixels_per_world_unit Ratio between pixels and world units spanning the screen width.
 * Passed to helper calculate_label_bbox_from_relative_pos() which uses values in pixels to offset label bounding boxes.
 * @return Per-edge delay label drawing information that has each label's updated position.
 */
static std::vector<t_label_drawing_info> calculate_least_cluttered_label_pos(std::vector<t_label_drawing_info> basic_label_drawing_info,
                                                                             double pixels_per_world_unit);

/**
 * @brief Hides labels that still overlap with others after calculate_least_cluttered_label_pos() has tried all candidates.
 * 
 * Hiding is performed in sequence from start to end (of the critcal path), from the first critcal path to the last.
 * Hiding one label may free up space for other labels and so they no longer need to be hidden.
 * Visibility of hidden labels are updated to post_decluttering_label_drawing_info.
 *
 * @param post_decluttering_label_drawing_info Per-edge label drawing information that includes each label's updated position.
 * @return Per-edge label drawing information that has updates on what labels are newly hidden due to still existing overlaps.
 */
static std::vector<t_label_drawing_info> hide_still_cluttered_labels(std::vector<t_label_drawing_info> post_decluttering_label_drawing_info);

/**
 * @brief Calculates a label bounding box using the provided position relative to its timing-edge flyline.
 *
 * @param label_to_update Single Label drawing information whose bounding box is to be updated.
 * @param label_relative_pos Candidate position to apply, relative to the timing-edge flyline.
 * @param pixels_per_world_unit Ratio between pixels and world units spanning the screen width.
 * Used to convert values specified in pixels that determine the offset of the label bounding box.
 * @return The rectangle associated with label_to_update. Used to replace the current bounding box .
 */
static ezgl::rectangle calculate_label_bbox_from_relative_pos(t_label_drawing_info& label_to_update, e_label_relative_pos label_relative_pos, double pixels_per_world_unit);

/**
 * @brief Returns true if two label bounding boxes overlap.
 *
 * @param bbox1 First bounding box to compare.
 * @param bbox2 Second bounding box to compare.
 */
static bool check_if_bboxes_overlap(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);

/**
 * @brief Draws all non-hidden delay labels in styles specified by the per-edge label drawing information.
 *
 * @param final_label_drawing_info Fully updated per-edge label drawing information queried for drawing.
 * @param g Pointer to the ezgl::renderer object.
 */
static void draw_labels(std::vector<t_label_drawing_info>& final_label_drawing_info, ezgl::renderer* g);

/**
 * @brief Draws messages that shows the total delay time and the cooresponding critical path index, with a background behind.
 * 
 * @param paths Timing paths whose total delay time is put into the messages drawn.
 * @param g Renderer used to perform drawing.
 */
static void draw_total_delay_messages(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g);

/**
 * @brief Calculates the color of a timing edge from its source timing node id deterministically.
 * 
 * @param src_tnode_id Source timing node id.
 * @return Color associated with the provided timing node id.
 */
static ezgl::color get_edge_color_from_src_tnode_id(tatum::NodeId src_tnode_id);

/**
 * @brief Returns the visibility and transparency for a timing edge flyline.
 *
 * In analytical placement, timing edge flylines are drawn between AP blocks, which reside at continuous layer levels
 * and are treated as always visible and opaque. Therefore, the same convention is applied to flylines.
 * In regular placement/routing views, visibility follows the discrete device-layer display settings.
 *
 * @param src_node Source timing node of the flyline.
 * @param sink_node Sink timing node of the flyline.
 * @return Visibility and transparency used to draw the flyline.
 */
static t_draw_layer_display get_timing_flyline_visibility(tatum::NodeId src_node, tatum::NodeId sink_node);

/**
 * @brief Returns the layer number of a timing path node.
 * 
 * @param node Timing node to locate.
 * 
 * @return layer number the node is situated on.
 */
static int get_tnode_layer_num(tatum::NodeId node);

/**
 * @brief Returns the drawing coordinates for a timing edge flyline.
 *
 * In analytical placement, timing nodes are mapped to the centers of their associated AP blocks.
 * If both endpoints collapse to the same AP block, there is no meaningful flyline to draw, so this returns std::nullopt.
 *
 * In non-AP drawing stages, timing nodes are mapped to their atom pin drawing coordinates.
 *
 * @param src_node Source timing node of the timing edge.
 * @param sink_node Sink timing node of the timing edge.
 *
 * @return The start/end drawing coordinates for the flyline, or std::nullopt if the flyline is skipped.
 */
static t_flyline_draw_coords get_timing_flyline_draw_coords(tatum::NodeId src_node,
                                                            tatum::NodeId sink_node);

/**
 * @brief Returns the drawing coordinates of a timing node.
 *
 * The timing node is mapped to its corresponding atom pin, then to the normal
 * atom pin drawing location.
 *
 * @param node Timing node to locate.
 * @return Drawing coordinates of the node.
 */
static ezgl::point2d get_tnode_draw_coord(tatum::NodeId node);

/**
 * @brief Returns the AP block containing a timing node.
 *
 * The timing node is mapped to its atom pin, then to the atom block containing
 * that pin, and finally to the AP block containing that atom block.
 *
 * @param node Timing node to look up.
 * @return AP block containing the timing node's atom block.
 */
static APBlockId get_tnode_ap_block(tatum::NodeId node);

void draw_crit_path(ezgl::renderer* g) {
    tatum::TimingPathCollector path_collector;
    t_draw_state* draw_state = get_draw_state_vars();
    const TimingContext& timing_ctx = g_vpr_ctx.timing();

    if (!draw_state->show_crit_path) {
        return;
    }

    if (!draw_state->setup_timing_info) {
        // No timing to draw
        return;
    }

    // Get the worst timing path
    auto paths = path_collector.collect_worst_setup_timing_paths(
        *timing_ctx.graph,
        *(draw_state->setup_timing_info->setup_analyzer()), draw_state->num_crit_paths);
    
    if (draw_state->show_crit_path_flylines) {
        draw_timing_edge_flylines(paths, g);
        if(draw_state->show_crit_path_delays) {
            calculate_and_draw_delay_labels(paths, g);
            draw_total_delay_messages(paths, g);
        }
    }

    // Ensure that we are already in the routing stage.
    if (draw_state->show_crit_path_routing && draw_state->pic_on_screen == e_pic_type::ROUTING) {
        draw_routed_timing_connections(paths, g);
    }
}

static void draw_timing_edge_flylines(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g) {
    g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
    g->set_line_width(3);

    for (std::size_t path_idx = 0; path_idx < paths.size(); path_idx++) {
        const tatum::TimingPath& path = paths[path_idx];
        auto elements = path.data_arrival_path().elements();
        // elements.size() returns the number of timing nodes. No flyline to draw when there is less than or equal to one node.
        if (elements.size() <= 1) {
            continue;
        }

        tatum::NodeId prev_node;
        for (const tatum::TimingPathElem& elem : elements) {
            tatum::NodeId node = elem.node();
            // Skip the first iteration because prev_node is not yet assigned to an actual node.
            if (prev_node) {
                ezgl::color color = get_edge_color_from_src_tnode_id(prev_node);

                // Check visibility of layers where source (prev_node) and sink (node) reside.
                t_draw_layer_display flyline_visibility = get_timing_flyline_visibility(prev_node, node);

                if (flyline_visibility.visible) {
                    g->set_color(color, flyline_visibility.alpha);

                    // Calculate the drawing coordinates of the flyline.
                    t_flyline_draw_coords timing_flyline_draw_coords = get_timing_flyline_draw_coords(prev_node, node);
                    ezgl::point2d start = timing_flyline_draw_coords.start;
                    ezgl::point2d end = timing_flyline_draw_coords.end;

                    // No flyline to draw when the two timing nodes collapse to the same drawing point.
                    if (!timing_flyline_draw_coords.collapse_to_point) {
                        g->draw_line(start, end);
                        // Draw an arrow at the flyline center.
                        draw_triangle_along_line_fixed_px(g, start, end, EDGE_CENTER, TIMING_EDGE_ARROW_SCALE * DEFAULT_ARROW_SIZE);
                    }

                    // Draw a star at the beginning and end of the timing path.
                    // Note: even if the first and/or last flyline segment collapses to a single point,
                    // the star is still drawn regardless.
                    if (prev_node == elements.begin()->node()) {
                        draw_crit_path_endpoint(e_crit_path_endpoint_type::START, start, path_idx, g);
                    }
                    
                    // We have ensured that elements.size() <= 1 at the beginning, but if in the future that logic
                    // is tampered, --elements.end() will become dangerous.
                    VTR_ASSERT_SAFE(elements.size() > 1);
                    if (node == (--elements.end())->node()) {
                        draw_crit_path_endpoint(e_crit_path_endpoint_type::END, end, path_idx, g);
                    }
                }
            }
            prev_node = node;
        }
    }

    g->set_line_dash(ezgl::line_dash::none);
    g->set_line_width(0);
}

static void draw_crit_path_endpoint(e_crit_path_endpoint_type endpoint_type, ezgl::point2d endpoint_coords, std::size_t path_idx, ezgl::renderer* g) {
    if (endpoint_type == e_crit_path_endpoint_type::START) {
        // Medium Green. We do not use the default ezgl::GREEN due to poor contrast.
        g->set_color({0x00, 0xCC, 0x00});
    } else if (endpoint_type == e_crit_path_endpoint_type::END) {
        g->set_color(ezgl::RED);
    } else {
        VTR_ASSERT_SAFE_MSG(false, "Illegal e_crit_path_endpoint value provided! Legal values are START and END.");
    }

    // Draw a star shape at the endpoint.
    draw_star_fixed_px(endpoint_coords, ENDPOINT_STAR_SIZE, g);

    // Draw the critical path index at the star center.
    g->set_font_size(16);
    // Use white for good contrast to the two star colors.
    g->set_color(ezgl::WHITE);
    g->draw_text(endpoint_coords, std::to_string(path_idx));
}

static void draw_routed_timing_connections(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g) {
    VTR_ASSERT_SAFE(get_draw_state_vars()->pic_on_screen == e_pic_type::ROUTING);

    for (const tatum::TimingPath& path : paths) {
        auto elements = path.data_arrival_path().elements();
        // elements.size() returns the number of timing nodes. No connection to draw when there is less than or equal to one node.
        if (elements.size() <= 1) {
            continue;
        }

        tatum::NodeId prev_node;
        for (const tatum::TimingPathElem& elem : elements) {
            tatum::NodeId node = elem.node();
            // Skip the first iteration because prev_node is not yet assigned to an actual node.
            if (prev_node) {
                ezgl::color color = get_edge_color_from_src_tnode_id(prev_node);

                draw_routed_connections_between_nodes(prev_node, node, color, g);

            }
            prev_node = node;
        }
    }
}

static void draw_routed_connections_between_nodes(tatum::NodeId src_tnode, tatum::NodeId sink_tnode, ezgl::color color, ezgl::renderer* g) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const TimingContext& timing_ctx = g_vpr_ctx.timing();
    const RoutingContext& routing_ctx = g_vpr_ctx.routing();

    AtomPinId atom_src_pin = atom_ctx.lookup().tnode_atom_pin(src_tnode);
    AtomPinId atom_sink_pin = atom_ctx.lookup().tnode_atom_pin(sink_tnode);

    tatum::EdgeId tedge = timing_ctx.graph->find_edge(src_tnode, sink_tnode);
    tatum::EdgeType edge_type = timing_ctx.graph->edge_type(tedge);

    // We currently only trace interconnect edges in detail, and treat all others as flylines.
    if (edge_type == tatum::EdgeType::INTERCONNECT) {
        // When flat routing is enabled, the route tree is derived from the atom netist which we need to query to get the routed connection.
        // Otherwise, the route tree is derived from the cluster netlist.
        if (routing_ctx.is_flat) {
            // See the function declaration for why atom_src_pin does not need to be passed in.
            draw_connections_from_atom_netlist(atom_sink_pin, color, g);
        } else {
            // See the function declaration for why atom_src_pin is needed here.
            draw_connections_from_cluster_netlist(atom_src_pin, atom_sink_pin, color, g);
        }
    }
}

static void draw_connections_from_atom_netlist(AtomPinId atom_sink_pin, ezgl::color color, ezgl::renderer* g) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    // Fetch the net connected the sink pin (there is only one unique net because the sink pin is an output here).
    AtomNetId atom_net_id = atom_ctx.netlist().pin_net(atom_sink_pin);
    VTR_ASSERT(atom_net_id != AtomNetId::INVALID());
    int sink_net_pin_index = atom_ctx.netlist().pin_net_index(atom_sink_pin);
    // A valid sink net pin index should be greater or equal to 1; 0 corresponds to the driver pin (src pin).
    VTR_ASSERT(sink_net_pin_index >= 1);

    t_draw_state* draw_state = get_draw_state_vars();

    // A src net pin is always 0.
    std::vector<RRNodeId> routed_rr_nodes = trace_routed_connection_rr_nodes(atom_net_id, 0, sink_net_pin_index);

    // Mark all the nodes highlighted.
    for (RRNodeId inode : routed_rr_nodes) {
        draw_state->draw_rr_node[inode].color = color;
        draw_state->draw_rr_node[inode].node_highlighted = true;
    }

    // draw_partial_route() takes care of layer visibility and cross-layer settings.
    draw_partial_route(routed_rr_nodes, g);
}

static void draw_connections_from_cluster_netlist(AtomPinId atom_src_pin, AtomPinId atom_sink_pin, ezgl::color color, ezgl::renderer* g) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    // All atom pins are implemented inside CLBs, so next hop is to the top-level CLB pins
    // TODO: most of this code is highly similar to code in PostClusterDelayCalculator, refactor
    //      into a common method for walking the clustered netlist, this would also (potentially)
    //      allow us to grab the component delays
    AtomBlockId atom_src_block = atom_ctx.netlist().pin_block(atom_src_pin);
    AtomBlockId atom_sink_block = atom_ctx.netlist().pin_block(atom_sink_pin);

    ClusterBlockId clb_src_block = atom_ctx.lookup().atom_clb(atom_src_block);
    VTR_ASSERT(clb_src_block != ClusterBlockId::INVALID());
    ClusterBlockId clb_sink_block = atom_ctx.lookup().atom_clb(
        atom_sink_block);
    VTR_ASSERT(clb_sink_block != ClusterBlockId::INVALID());

    const t_pb_graph_pin* sink_gpin = atom_ctx.lookup().atom_pin_pb_graph_pin(
        atom_sink_pin);
    VTR_ASSERT(sink_gpin);

    int sink_pb_route_id = sink_gpin->pin_count_in_cluster;

    ClusterNetId cluster_net_id = ClusterNetId::INVALID();
    int sink_block_pin_index = -1;
    int sink_net_pin_index = -1;

    std::tie(cluster_net_id, sink_block_pin_index, sink_net_pin_index) = find_pb_route_clb_input_net_pin(clb_sink_block,
                                                                                                         sink_pb_route_id);
    if (cluster_net_id != ClusterNetId::INVALID() && sink_block_pin_index != -1
        && sink_net_pin_index != -1) {
        // Connection leaves the CLB
        // Now that we have the CLB source and sink pins, we need to grab all the points on the routing connecting the pins
        VTR_ASSERT(
            cluster_ctx.clb_nlist.net_driver_block(cluster_net_id)
            == clb_src_block);

        t_draw_state* draw_state = get_draw_state_vars();

        std::vector<RRNodeId> routed_rr_nodes = trace_routed_connection_rr_nodes(cluster_net_id, 0, sink_net_pin_index);

        // Mark all the nodes highlighted

        for (RRNodeId inode : routed_rr_nodes) {
            draw_state->draw_rr_node[inode].color = color;
            draw_state->draw_rr_node[inode].node_highlighted = true;
        }

        // draw_partial_route() takes care of layer visibility and cross-layer settings
        draw_partial_route(routed_rr_nodes, (ezgl::renderer*)g);
    } else {
        // Connection entirely within the CLB, we don't draw the internal routing so treat it as a fly-line
        VTR_ASSERT(clb_src_block == clb_sink_block);
    }
}

static void calculate_and_draw_delay_labels(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g) {
    // The ratio between pixels and world units spanning the screen width.
    // Used to perform screen-to-world conversions for label bounding boxes that primarily use pixels.
    double pixels_per_world_unit = get_pixels_per_world_unit(g);

    // Calculate basic information needed for resolving overlap and drawing.
    std::vector<t_label_drawing_info> basic_label_drawing_info =
        calculate_basic_label_drawing_info(paths, pixels_per_world_unit, g);

    // Update the drawing info vector by trying to resolve all overlaps first.
    std::vector<t_label_drawing_info> post_decluttering_label_drawing_info =
        calculate_least_cluttered_label_pos(std::move(basic_label_drawing_info), pixels_per_world_unit);

    // Get the final drawing info vector by hiding labels with inevitable overlaps;
    std::vector<t_label_drawing_info> final_label_drawing_info =
        hide_still_cluttered_labels(std::move(post_decluttering_label_drawing_info));

    // Draw non-hidden delay labels on the timing edge flylines.
    draw_labels(final_label_drawing_info, g);
}

static std::size_t get_num_edges_of_multi_paths(const std::vector<tatum::TimingPath>& paths) {
    std::size_t total_num_edges = 0;
    for (const tatum::TimingPath& path : paths) {
        auto elements = path.data_arrival_path().elements();
        // elements.size() returns the number of timing nodes, but we want the number of timing edges.
        std::size_t num_edges = elements.size() == 0 ? 0 : elements.size() - 1;
        total_num_edges += num_edges;
    }
    return total_num_edges;
}

static std::vector<t_label_drawing_info> calculate_basic_label_drawing_info(const std::vector<tatum::TimingPath>& paths,
                                                                            double pixels_per_world_unit,
                                                                            ezgl::renderer* g) {
    // Set font size to correctly calculate text (label) dimension later.
    g->set_font_size(16);

    // Per-edge label drawing info.
    std::vector<t_label_drawing_info> basic_label_drawing_info;
    std::size_t total_num_edges = get_num_edges_of_multi_paths(paths);
    // If every critical path has completely different timing edge segments, there will be a number of total_num_edges labels.
    // If not, some labels are repeated and will be skipped, in which case the final vector size will be less.
    basic_label_drawing_info.reserve(total_num_edges);

    // A record of visited timing edges. Used to check for repeated labels.
    // Since the number of timing edges is usually small (even when there are multiple critcal paths),
    // std::vector has good enough performace.
    std::vector<t_timing_edge_id> visited_edges;
    visited_edges.reserve(total_num_edges);

    for (const tatum::TimingPath& path : paths) {
        auto elements = path.data_arrival_path().elements();
        // elements.size() returns the number of timing nodes. No label to draw when there is less than or equal to one node.
        if (elements.size() <= 1) {
            continue;
        }

        tatum::NodeId prev_node;
        float prev_arr_time = std::numeric_limits<float>::quiet_NaN();
        for (const tatum::TimingPathElem& elem : elements) {
            tatum::NodeId node = elem.node();
            float arr_time = elem.tag().time();
            // Skip the first iteration because prev_node is not yet assigned to an actual node.
            if (prev_node) {
                t_timing_edge_id edge_id = {prev_node, node};
                // A label is skipped if its corresponding timing edge has been visited before.
                if (std::find(visited_edges.begin(), visited_edges.end(), edge_id) != visited_edges.end()) {
                    prev_node = node;
                    prev_arr_time = arr_time;
                    continue;
                } else {
                    visited_edges.push_back(edge_id);
                }

                // Check visibility of layers where source and sink reside.
                t_draw_layer_display flyline_visibility = get_timing_flyline_visibility(prev_node, node);
                // Hide the label if the corresponding timing edge flyline is not visible.
                if (!flyline_visibility.visible) {
                    prev_node = node;
                    prev_arr_time = arr_time;
                    continue;
                }

                // Calculate where the corresponding timing edge flyline is placed.
                t_flyline_draw_coords timing_flyline_draw_coords = get_timing_flyline_draw_coords(prev_node, node);
                // No flyline to draw when the two timing nodes collapse to the same drawing point,
                // and hence no label to draw.
                if (timing_flyline_draw_coords.collapse_to_point) {
                    prev_node = node;
                    prev_arr_time = arr_time;
                    continue;
                }

                // After the several checks above, we can safely construct a drawing info object for the label.
                t_label_drawing_info& drawing_info = basic_label_drawing_info.emplace_back();

                // Delay time in seconds.
                float delay_time = arr_time - prev_arr_time;
                // Convert to nanoseconds.
                delay_time = 1e9 * delay_time
                std::stringstream ss;
                // Set precision to three decimals and use std::fixed to explicitly show three decimals for visual consistency among labels
                // (Note: 1.5, for example, is consistent with std::setprecision(3) but still needs to be extended to 1.500 by std::fixed).
                ss << std::setprecision(3) << std::fixed << delay_time;
                // This local std::string will help construct the label bounding box later.
                std::string delay_label_str = ss.str();
                drawing_info.delay_label_str = delay_label_str;

                drawing_info.label_color = get_edge_color_from_src_tnode_id(prev_node);
                drawing_info.label_transparency = flyline_visibility.alpha;
                
                ezgl::point2d start = timing_flyline_draw_coords.start;
                ezgl::point2d end = timing_flyline_draw_coords.end;
                // After this step, start and end are just a relative concept.
                // This step is to ensure that start is always to the physical left of end,
                // which later helps facilitate the math.
                if (start.x > end.x) {
                    std::swap(start, end);
                }
                double min_y = std::min(start.y, end.y);
                double max_y = std::max(start.y, end.y);
                // It is already ensured in the previous step that start.x < end.x.
                ezgl::rectangle edge_bbox({start.x, min_y}, {end.x, max_y});

                // Calculate the length of the corresponding timing edge.
                drawing_info.edge_length = std::sqrt(std::pow(edge_bbox.width(), 2) + std::pow(edge_bbox.height(), 2));

                // Since start.x < end.x, the result from atan2() is always between - pi / 2 and pi/ 2.
                double rotation_angle = (180 / std::numbers::pi) * atan2(end.y - start.y, end.x - start.x);
                drawing_info.rotation_angle = rotation_angle;

                // Calculate the bounding box that inscribes the label. This can be imagined as a horizontal rectangle
                // fitting in a tilted (not necessarily always) rectangle that represents the label:
                //      .................
                //      .        ////// .
                //      .      //////   .
                //      .    //////     .
                //      .  //////       .
                //      .//////         .
                //      .................
                // Note: This illustration is for reference only; the tilted rectangle should have square corners.

                // This specifies the dimension of the "tilted rectangle" in pixels.
                ezgl::t_text_dimension delay_label_dimension = g->get_text_dimension(delay_label_str);
                // The bbox is defined in world coordinates, and we need to perform a conversion to pixels at the end.
                double label_bbox_width = (delay_label_dimension.width * cos(rotation_angle * (std::numbers::pi / 180))
                                        + delay_label_dimension.height * std::abs(sin(rotation_angle * (std::numbers::pi / 180))))
                                        / pixels_per_world_unit;
                double label_bbox_height = (delay_label_dimension.width * std::abs(sin(rotation_angle * (std::numbers::pi / 180)))
                                            + delay_label_dimension.height * cos(rotation_angle * (std::numbers::pi / 180)))
                                        / pixels_per_world_unit;

                ezgl::point2d bbox_bottom_left = edge_bbox.center() - ezgl::point2d(label_bbox_width / 2, label_bbox_height / 2);
                // Calculates a virtual bounding box centered on the timing edge before offsets are applied.
                drawing_info.virtual_centered_label_bbox = ezgl::rectangle(bbox_bottom_left, label_bbox_width, label_bbox_height);
                // Apply CENTER_ABOVE to get the default label bounding box.
                drawing_info.label_bbox = calculate_label_bbox_from_relative_pos(drawing_info, e_label_relative_pos::CENTER_ABOVE, pixels_per_world_unit);
            }
            prev_node = node;
            prev_arr_time = arr_time;
        }
    }
    return basic_label_drawing_info;
}

static std::vector<t_label_drawing_info> calculate_least_cluttered_label_pos(std::vector<t_label_drawing_info> basic_label_drawing_info,
                                                                             double pixels_per_world_unit) {

    // The label position candidates are ordered in an implied priority which the decluttering algorithm will follow.
    constexpr std::array<e_label_relative_pos, 10> label_pos_candidates = {e_label_relative_pos::CENTER_ABOVE,
                                                                           e_label_relative_pos::LEFT_ABOVE,
                                                                           e_label_relative_pos::RIGHT_ABOVE,
                                                                           e_label_relative_pos::CENTER_BELOW,
                                                                           e_label_relative_pos::LEFT_BELOW,
                                                                           e_label_relative_pos::RIGHT_BELOW,
                                                                           e_label_relative_pos::FAR_LEFT_ABOVE,
                                                                           e_label_relative_pos::FAR_RIGHT_ABOVE,
                                                                           e_label_relative_pos::FAR_LEFT_BELOW,
                                                                           e_label_relative_pos::FAR_RIGHT_BELOW};

    for (std::size_t label_idx = 0; label_idx < basic_label_drawing_info.size(); label_idx++) {
        t_label_drawing_info& drawing_info = basic_label_drawing_info[label_idx];

        // The position candidate to start with is CENTER_ABOVE, which aligns with the order in label_pos_candidates.
        e_label_relative_pos candidate_with_least_overlaps = e_label_relative_pos::CENTER_ABOVE;

        int least_num_overlaps = std::numeric_limits<int>::max();

        // Try all possible position candidates unless finding one with zero overlap.
        for (const e_label_relative_pos& pos_candidate : label_pos_candidates) {
            // Update the label bounding box stored in drawing_info, which is tied to the current label being processed.
            drawing_info.label_bbox = calculate_label_bbox_from_relative_pos(drawing_info, pos_candidate, pixels_per_world_unit);

            int curr_num_overlaps = 0;

            // Check for potential overlaps with all other labels.
            for (std::size_t label_idx_to_compare = 0; label_idx_to_compare < basic_label_drawing_info.size(); label_idx_to_compare++) {
                const t_label_drawing_info& drawing_info_to_compare = basic_label_drawing_info[label_idx_to_compare];

                // Skip self-comparison and / or if the label to compare is invisible.
                if (label_idx == label_idx_to_compare || drawing_info_to_compare.hide_label) {
                    continue;
                }
                // Update the number of overlaps the current position candidate results in.
                if (check_if_bboxes_overlap(drawing_info.label_bbox, drawing_info_to_compare.label_bbox)) {
                    curr_num_overlaps++;
                }
            }

            // Early exit condition
            if (curr_num_overlaps == 0) {
                candidate_with_least_overlaps = pos_candidate;
                break;
            }
            // A candidate with fewer overlaps found. Still cannot exit though because there might be an untried candidate that has zero overlap.
            if (curr_num_overlaps < least_num_overlaps) {
                least_num_overlaps = curr_num_overlaps;
                candidate_with_least_overlaps = pos_candidate;
            }
        }
        // Update the label bounding box using the chosen position candidate.
        drawing_info.label_bbox = calculate_label_bbox_from_relative_pos(drawing_info, candidate_with_least_overlaps, pixels_per_world_unit);
    }
    return basic_label_drawing_info;
}

static std::vector<t_label_drawing_info> hide_still_cluttered_labels(std::vector<t_label_drawing_info> post_decluttering_label_drawing_info) {

    for (std::size_t label_idx = 0; label_idx < post_decluttering_label_drawing_info.size(); label_idx++) {
        t_label_drawing_info& drawing_info = post_decluttering_label_drawing_info[label_idx];

        // As long as there is one overlap associated with this label, we hide it.
        // Note: A hidden label may positively affect the fate of subsequent labels, and hence we must perform a clean calculation for each label
        // to ensure the most recent status is reflected.
        bool has_overlap = false;
        for (std::size_t label_idx_to_compare = 0; label_idx_to_compare < post_decluttering_label_drawing_info.size(); label_idx_to_compare++) {
            const t_label_drawing_info& drawing_info_to_compare = post_decluttering_label_drawing_info[label_idx_to_compare];

            // Skip self-comparison and / or if the label to compare is invisible.
            if (label_idx == label_idx_to_compare || drawing_info_to_compare.hide_label) {
                continue;
            }

            if (check_if_bboxes_overlap(drawing_info.label_bbox, drawing_info_to_compare.label_bbox)) {
                has_overlap = true;
                break;
            }
        }
        if (has_overlap) {
            drawing_info.hide_label = true;
        }
    }
    return post_decluttering_label_drawing_info;
}

static ezgl::rectangle calculate_label_bbox_from_relative_pos(t_label_drawing_info& label_to_update,
                                                              e_label_relative_pos label_relative_pos,
                                                              double pixels_per_world_unit) {
    double edge_length = label_to_update.edge_length;
    // The unit length in world coordinates that can be doubled or directly used as the edgewise offset.
    double edge_offset_unit = edge_length * EDGE_OFFSET_FRACTION;

    // For ultra long timing edges, using a fraction of the total edge length may result in labels jumping drastically
    // at different zoom levels. Therefore, we want to cap the edge offset unit at a certain threshold.
    // Convert MAX_EDGE_OFFSET_UNIT (defined in pixels) to world coordinates.
    if (edge_offset_unit > MAX_EDGE_OFFSET_UNIT / pixels_per_world_unit) {
        edge_offset_unit = MAX_EDGE_OFFSET_UNIT / pixels_per_world_unit;
    }

    double perpendicular_offset = 0;
    double edge_offset = 0.0;

    // Apply perpendicular offset associated with the specified relative position.
    switch (label_relative_pos) {
        case e_label_relative_pos::CENTER_ABOVE:
        case e_label_relative_pos::LEFT_ABOVE:
        case e_label_relative_pos::RIGHT_ABOVE:
        case e_label_relative_pos::FAR_LEFT_ABOVE:
        case e_label_relative_pos::FAR_RIGHT_ABOVE:
            // Convert PERPENDICULAR_OFFSET (defined in pixels) to world coordinates.
            perpendicular_offset = PERPENDICULAR_OFFSET / pixels_per_world_unit;
            break;
        case e_label_relative_pos::CENTER_BELOW:
        case e_label_relative_pos::LEFT_BELOW:
        case e_label_relative_pos::RIGHT_BELOW:
        case e_label_relative_pos::FAR_LEFT_BELOW:
        case e_label_relative_pos::FAR_RIGHT_BELOW:
            perpendicular_offset = -PERPENDICULAR_OFFSET / pixels_per_world_unit;
            break;
        default:
            // Unidentified e_label_relative_pos provided (could be due to modifying the original enum
            // while forgetting to update this function). Output a failure.
            VTR_ASSERT(false);
            perpendicular_offset = PERPENDICULAR_OFFSET / pixels_per_world_unit;
    }

    // Apply edge offset associated with the specified relative position.
    switch (label_relative_pos) {
        case e_label_relative_pos::CENTER_ABOVE:
        case e_label_relative_pos::CENTER_BELOW:
            edge_offset = 0.0;
            break;
        case e_label_relative_pos::LEFT_ABOVE:
        case e_label_relative_pos::LEFT_BELOW:
            edge_offset = -edge_offset_unit;
            break;
        case e_label_relative_pos::RIGHT_ABOVE:
        case e_label_relative_pos::RIGHT_BELOW:
            edge_offset = edge_offset_unit;
            break;
        case e_label_relative_pos::FAR_LEFT_ABOVE:
        case e_label_relative_pos::FAR_LEFT_BELOW:
            edge_offset = -edge_offset_unit * 2;
            break;
        case e_label_relative_pos::FAR_RIGHT_ABOVE:
        case e_label_relative_pos::FAR_RIGHT_BELOW:
            edge_offset = edge_offset_unit * 2;
            break;
        default:
            // Unidentified e_label_relative_pos provided. Output a failure.
            VTR_ASSERT(false);
            edge_offset = 0.0;
    }

    double rotation_angle_in_deg = label_to_update.rotation_angle * (std::numbers::pi / 180);
    // Apply the perpendicular offset
    double x_offset = -perpendicular_offset * sin(rotation_angle_in_deg);
    double y_offset = perpendicular_offset * cos(rotation_angle_in_deg);
    // Apply the edge offset.
    x_offset += edge_offset * cos(rotation_angle_in_deg);
    y_offset += edge_offset * sin(rotation_angle_in_deg);

    // Calculate the label bounding box by applying the 2d offset to the virtual bounding box.
    ezgl::rectangle label_bbox = label_to_update.virtual_centered_label_bbox + ezgl::point2d(x_offset, y_offset);
    return label_bbox;
}

static bool check_if_bboxes_overlap(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2) {
    if (bbox1.right() < bbox2.left() || bbox1.left() > bbox2.right() || bbox1.bottom() > bbox2.top() || bbox1.top() < bbox2.bottom()) {
        return false;
    } else {
        return true;
    }
}

static void draw_labels(std::vector<t_label_drawing_info>& final_label_drawing_info, ezgl::renderer* g) {
    g->set_font_size(16);

    for (t_label_drawing_info& drawing_info : final_label_drawing_info) {
        if (!drawing_info.hide_label) {
            g->set_color(drawing_info.label_color, drawing_info.label_transparency);
            g->set_text_rotation(drawing_info.rotation_angle);
            g->draw_text(drawing_info.label_bbox.center(), drawing_info.delay_label_str);
        }
    }
    g->set_text_rotation(0);
}

static void draw_total_delay_messages(const std::vector<tatum::TimingPath>& paths, ezgl::renderer* g) {
    // The drawing logic of the total delay message background requires at least one path to exist.
    // Therefore, we need an explicit early exit.
    if (paths.size() == 0) {
        return;
    }

    g->set_font_size(20);
    g->set_text_rotation(0);
    // Use the screen (pixel) coordinates to draw the total delay messages and their background at a fixed screen location.
    g->set_coordinate_system(ezgl::SCREEN);
    
    // The number of total delay messages is tied to the number of critical paths.
    std::vector<std::string> total_delay_messages;
    total_delay_messages.reserve(paths.size());

    // The longest total delay message width, in pixels.
    double max_msg_width = 0.0;
    // The total delay message height. Since a uniform font size is used, all the messages share the same height,
    // contrasting with their width.
    double msg_height = 0.0;

    for (std::size_t path_idx = 0; path_idx < paths.size(); path_idx++) {
        float total_delay_time = paths[path_idx].path_info().delay();
        // Convert to nanoseconds.
        total_delay_time = 1e9 * total_delay_time;
        // Construct the message.
        std::stringstream ss;
        ss << "Crit Path [" << path_idx << "]: ";
        // Set precision to three decimals and use std::fixed to explicitly show three decimals for visual consistency among labels
        // (Note: 1.5, for example, is consistent with std::setprecision(3) but still needs to be extended to 1.500 by std::fixed).
        ss << std::setprecision(3) << std::fixed << total_delay_time;
        ss << " ns";
        std::string total_delay_msg = ss.str();
        // Save the message for drawing later.
        total_delay_messages.push_back(total_delay_msg);

        // Message dimension in pixels.
        ezgl::t_text_dimension msg_dimension = g->get_text_dimension(total_delay_msg);

        // Only need to update this once, because the message height is always the same under the same font size.
        if (path_idx == 0) {
            msg_height = msg_dimension.height;
        }

        if (msg_dimension.width > max_msg_width) {
            max_msg_width = msg_dimension.width;
        }
    }

    // Note: when we draw the background and the messages, keep in mind that the y axis is inverted and y = 0 is the screen top.
    //
    // We will draw the background rectangle first, then the messages. Otherwise the messages will be overdrawn.
    //
    // The rectangle's center x coordinate is determined by subtracting max_msg_width from the right of the screen.
    double background_rect_center_x = g->get_visible_screen().right() - max_msg_width;

    // The width of the rectangle before padding is max_msg_width. Subtract half of that from the center to reach its left side.
    // Finally, apply padding to create the actual left side of the rectangle.
    double rect_left = background_rect_center_x - max_msg_width / 2 - PADDING;

    // The first total delay message will be drawn at y = msg_height. To reach the text ceiling (the rough rectangle top),
    // we need to subtract half the height from where it is drawn, and this gives us y = msg_height - msg_height / 2 = msg_height / 2.
    // Finally, apply padding to create the actual top side of the rectangle.
    double rect_top = msg_height / 2 - PADDING;

    // Take into account padding on the left and right.
    double rect_width = max_msg_width + 2 * PADDING;

    // The message height multiplied by the number of messages gives the rough rectangle height.
    // Also take into account padding on the top and bottom.
    double rect_height = msg_height * total_delay_messages.size() + 2 * PADDING;

    ezgl::rectangle rect(ezgl::point2d{rect_left, rect_top}, rect_width, rect_height);
    // Draw the background rectangle with a little translucency for good visual effect. 0 is transparent and 255 is opaque.
    g->set_color(ezgl::WHITE, 225);
    g->fill_rectangle(rect);
    
    // Draw the messages.
    g->set_color(ezgl::BLACK, 255);
    for (std::size_t msg_idx = 0; msg_idx < total_delay_messages.size(); msg_idx++) {
        std::string total_delay_msg = total_delay_messages[msg_idx];
        // Align the message with the background rectangle's center x coordinate.
        // We want the first message to be drawn at y = msg_height, and hence we need the plus 1 added to msg_idx.
        g->draw_text(ezgl::point2d{background_rect_center_x, msg_height * (msg_idx + 1)}, total_delay_msg);
    }
    g->set_coordinate_system(ezgl::WORLD);
}

static ezgl::color get_edge_color_from_src_tnode_id(tatum::NodeId src_tnode_id) {
    VTR_ASSERT_SAFE(src_tnode_id.is_valid());
    return kelly_max_contrast_colors[static_cast<std::size_t>(src_tnode_id) % kelly_max_contrast_colors.size()];
}

static t_draw_layer_display get_timing_flyline_visibility(tatum::NodeId src_node, tatum::NodeId sink_node) {
    t_draw_state* draw_state = get_draw_state_vars();

    // In analytical placement, the AP blocks that timing nodes are mapped to live at continuous layer levels
    // and are treated as always visible and opaque. Therefore, the same convention is applied to flylines.
    if (draw_state->pic_on_screen == e_pic_type::ANALYTICAL_PLACEMENT) {
        // Note: in the VPR context, 255 is opaque (and 0 is transparent).
        return t_draw_layer_display{true, 255};
    } else {
        // In other drawing stages, we conform to the discrete device-layer display settings.
        int src_block_layer = get_tnode_layer_num(src_node);
        int sink_block_layer = get_tnode_layer_num(sink_node);
        return get_element_visibility_and_transparency(src_block_layer, sink_block_layer);
    }
}

static int get_tnode_layer_num(tatum::NodeId node) {
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    AtomPinId atom_pin = atom_ctx.lookup().tnode_atom_pin(node);
    AtomBlockId atom_block = atom_ctx.netlist().pin_block(atom_pin);
    ClusterBlockId clb_block = atom_ctx.lookup().atom_clb(atom_block);
    return block_locs[clb_block].loc.layer;
}

static t_flyline_draw_coords get_timing_flyline_draw_coords(tatum::NodeId src_node,
                                                            tatum::NodeId sink_node) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_flyline_draw_coords flyline_draw_coords;
    flyline_draw_coords.collapse_to_point = false;

    ezgl::point2d start, end;
    if (draw_state->pic_on_screen == e_pic_type::ANALYTICAL_PLACEMENT) {
        // In analytical placement, the timing nodes are mapped to the centers of their associated
        // AP blocks. If the two timing nodes collapse to the same AP block, there is no flyline to draw.
        APBlockId src_ap_block = get_tnode_ap_block(src_node);
        APBlockId sink_ap_block = get_tnode_ap_block(sink_node);

        if (src_ap_block == sink_ap_block) {
            flyline_draw_coords.collapse_to_point = true;
        }

        flyline_draw_coords.start = get_ap_block_draw_coords(src_ap_block);
        flyline_draw_coords.end = get_ap_block_draw_coords(sink_ap_block);
    } else {
        // In other stages, timing nodes are mapped to their corresponding atom pin coordinates.
        flyline_draw_coords.start = get_tnode_draw_coord(src_node);
        flyline_draw_coords.end = get_tnode_draw_coord(sink_node);
    }

    return flyline_draw_coords;
}

static ezgl::point2d get_tnode_draw_coord(tatum::NodeId node) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    AtomPinId pin = atom_ctx.lookup().tnode_atom_pin(node);
    return atom_pin_draw_coord(pin);
}

static APBlockId get_tnode_ap_block(tatum::NodeId node) {
    t_draw_state* draw_state = get_draw_state_vars();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    // Get the lookup from atom block id to AP block id.
    const AtomBlockAPBlockLookup* atom_block_ap_block_lookup = draw_state->get_atom_block_ap_block_lookup_ptr();
    if (!atom_block_ap_block_lookup) {
        VTR_LOG_ERROR("Use of AtomBlockAPBlockLookup outside its lifetime (analytical placement) is not allowed.");
        return APBlockId::INVALID();
    }

    AtomPinId atom_pin = atom_ctx.lookup().tnode_atom_pin(node);
    AtomBlockId atom_block = atom_ctx.netlist().pin_block(atom_pin);
    return atom_block_ap_block_lookup->get_ap_block(atom_block);
}

#ifndef NO_SERVER

void draw_crit_path_elements(const std::vector<tatum::TimingPath>& paths, const std::map<std::size_t, std::set<std::size_t>>& indexes, bool draw_crit_path_contour, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    // The server mode does not support drawing during the AP stage.
    VTR_ASSERT(draw_state->pic_on_screen != e_pic_type::ANALYTICAL_PLACEMENT);

    const ezgl::color contour_color{0, 0, 0, 40};
    const ezgl::line_dash contour_line_style{ezgl::line_dash::none};
    const int contour_line_width{1};

    auto draw_server_mode_flylines_and_labels_helper_fn = [](ezgl::renderer* renderer, const ezgl::color& color, ezgl::line_dash line_style, int line_width, float delay,
                                                             const tatum::NodeId& prev_node, const tatum::NodeId& node, bool skip_draw_delays = false) {
        renderer->set_color(color);
        renderer->set_line_dash(line_style);
        renderer->set_line_width(line_width);
        draw_server_mode_flylines_and_labels(get_tnode_draw_coord(prev_node),
                                             get_tnode_draw_coord(node), delay, renderer, skip_draw_delays);

        renderer->set_line_dash(ezgl::line_dash::none);
        renderer->set_line_width(0);
    };

    for (const auto& [path_index, element_indexes] : indexes) {
        if (path_index < paths.size()) {
            const tatum::TimingPath& path = paths[path_index];

            // Walk through the timing path drawing each edge
            tatum::NodeId prev_node;
            float prev_arr_time = std::numeric_limits<float>::quiet_NaN();
            std::size_t element_counter = 0;
            for (const tatum::TimingPathElem& elem : path.data_arrival_path().elements()) {
                bool draw_current_element = element_indexes.empty() || element_indexes.find(element_counter) != element_indexes.end();

                // draw element
                tatum::NodeId node = elem.node();
                float arr_time = elem.tag().time();

                // We draw each 'edge' in a different color, this allows users to identify the stages and
                // any routing which corresponds to the edge
                //
                // We pick colors from the kelly max-contrast list, for long paths there may be repeats
                ezgl::color color = kelly_max_contrast_colors[element_counter % kelly_max_contrast_colors.size()];

                if (prev_node) {
                    float delay = arr_time - prev_arr_time;
                    if (draw_state->show_crit_path_flylines) {
                        if (draw_current_element) {
                            draw_server_mode_flylines_and_labels_helper_fn(g, color, ezgl::line_dash::asymmetric_5_3, /*line_width*/ 3, delay, prev_node, node);
                        } else if (draw_crit_path_contour) {
                            draw_server_mode_flylines_and_labels_helper_fn(g, contour_color, contour_line_style, contour_line_width, delay, prev_node, node, /*skip_draw_delays*/ true);
                        }
                    }
                    if (draw_state->show_crit_path_routing) {
                        if (draw_current_element) {
                            VTR_ASSERT(draw_state->pic_on_screen == e_pic_type::ROUTING);
                            //Draw the routed version of the timing edge
                            draw_routed_connections_between_nodes(prev_node, node, color, g);
                        }
                    }
                }

                prev_node = node;
                prev_arr_time = arr_time;
                // end draw element

                element_counter++;
            }
        }
    }
}

static void draw_server_mode_flylines_and_labels(ezgl::point2d start, ezgl::point2d end, float incr_delay, ezgl::renderer* g, bool skip_draw_delays /*=false*/) {
    g->draw_line(start, end);
    draw_triangle_along_line(g, start, end, 0.95, 40 * DEFAULT_ARROW_SIZE);
    draw_triangle_along_line(g, start, end, 0.05, 40 * DEFAULT_ARROW_SIZE);

    bool draw_delays = get_draw_state_vars()->show_crit_path_delays && !skip_draw_delays;

    if (draw_delays) {
        // Determine the strict bounding box based on the lines start/end
        float min_x = std::min(start.x, end.x);
        float max_x = std::max(start.x, end.x);
        float min_y = std::min(start.y, end.y);
        float max_y = std::max(start.y, end.y);

        // If we have a nearly horizontal/vertical line the bbox is too
        // small to draw the text, so widen it by a tile (i.e. CLB) width
        float tile_width = get_draw_coords_vars()->get_tile_width();
        if (max_x - min_x < tile_width) {
            max_x += tile_width / 2;
            min_x -= tile_width / 2;
        }
        if (max_y - min_y < tile_width) {
            max_y += tile_width / 2;
            min_y -= tile_width / 2;
        }

        // TODO: draw the delays nicer
        //   * rotate to match edge
        //   * offset from line
        //   * track visible in window
        ezgl::rectangle text_bbox({min_x, min_y}, {max_x, max_y});

        std::stringstream ss;
        ss.precision(3);
        ss << 1e9 * incr_delay; //In nanoseconds
        std::string incr_delay_str = ss.str();

        // Get the angle of line, to rotate the text
        float text_angle = (180 / std::numbers::pi)
                           * atan((end.y - start.y) / (end.x - start.x));

        // Get the screen coordinates for text drawing
        ezgl::rectangle screen_coords = g->world_to_screen(text_bbox);
        g->set_text_rotation(text_angle);

        // Set the text colour to black to differentiate it from the line
        g->set_font_size(16);
        g->set_color(ezgl::color(0, 0, 0));

        g->set_coordinate_system(ezgl::SCREEN);

        // Find an offset so it is sitting on top/below of the line
        float x_offset = screen_coords.center().x
                         - 8 * sin(text_angle * (std::numbers::pi / 180));
        float y_offset = screen_coords.center().y
                         - 8 * cos(text_angle * (std::numbers::pi / 180));

        ezgl::point2d offset_text_bbox(x_offset, y_offset);
        g->draw_text(offset_text_bbox, incr_delay_str,
                     text_bbox.width(), text_bbox.height());

        g->set_font_size(14);

        g->set_text_rotation(0);
        g->set_coordinate_system(ezgl::WORLD);
    }
}

#endif /* NO_SERVER */
#endif /* NO_GRAPHICS */
