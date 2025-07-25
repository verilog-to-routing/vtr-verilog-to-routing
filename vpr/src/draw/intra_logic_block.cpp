/* This file holds subroutines responsible for drawing inside clustered logic blocks.
 * The four main subroutines defined here are draw_internal_alloc_blk(),
 * draw_internal_init_blk(), draw_internal_draw_subblk(), and toggle_blk_internal().
 * When VPR graphics initially sets up, draw_internal_alloc_blk() will be called from
 * draw.c to allocate space for the structures needed for internal blks drawing->
 * Before any drawing, draw_internal_init_blk() will pre-compute a bounding box
 * for each sub-block in the pb_graph of every physical block type. When the menu button
 * "Blk Internal" is pressed, toggle_blk_internal() will enable internal blocks drawing->
 * Then, with each subsequent click on the button, toggle_blk_internal() will propel one
 * more level of pbs to be drawn. Draw_internal_draw_subblk() will be called whenever
 * new blocks need to be drawn, and this function is responsible for drawing sub-blocks
 * from the pre-computed bounding box values.
 *
 * Author: Long Yu (Mike) Wang
 * Date: August 2013, May 2014
 *
 * Author: Matthew J.P. Walker
 * Date: May 2014
 */

#ifndef NO_GRAPHICS

#include <cstdio>
#include <algorithm>
#include <string.h>

#include "vtr_assert.h"

#include "intra_logic_block.h"
#include "globals.h"
#include "atom_netlist.h"
#include "vpr_utils.h"
#include "draw_global.h"
#include "draw.h"
#include "draw_triangle.h"

// Vertical padding (as a fraction of tile height) added to leave space for text inside internal logic blocks
constexpr float FRACTION_TEXT_PADDING = 0.01;

/************************* Subroutines local to this file. *******************************/

/**
 * @brief A recursive function which traverses through each sub-block of a pb_graph_node, calculates bounding boxes for each sub-block, and stores the bounding boxes in the global draw_coords variable.
 * @param type_descrip_index The index of the logical block type.
 * @param pb_graph_node The pb_graph_node.
 * @param parent_width The width of the parent block.
 * @param parent_height The height of the parent block.
 */
static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, float parent_width, float parent_height);

/**
 * @brief Finds the maximum depth of sub-blocks for a given pb_type.
 * @param pb_type The top-level pb_type.
 * @return The maximum level for the given pb_type.
 */
static int draw_internal_find_max_lvl(const t_pb_type& pb_type);
/**
 * @brief A helper function to calculate the number of child blocks for a given t_mode.
 * @param mode The mode of the parent pb_type.
 * @return The number of child blocks for the given t_mode.
 */
static int get_num_child_blocks(const t_mode& mode);
/**
 * @brief A helper function for draw_internal_load_coords. 
 * Calculates the coordinates of a internal block and stores its bounding box inside global variables. 
 * The calculated width and height of the block are also assigned to the pointers blk_width and blk_height.
 * @param type_descrip_index The index of the logical block type.
 * @param pb_graph_node The pb_graph_node of the logical block type.
 * @param blk_num Each logical block type has num_modes * num_pb_type_children sub-blocks. blk_num is the index of the sub-block within the logical block type.
 * @param num_columns Number of columns of blocks to draw in the parent block type.
 * @param num_rows Number of rows of blocks to draw in the parent block type.
 * @param parent_width The width of the parent block.
 * @param parent_height The height of the parent block.
 * @param blk_width Pointer to store the calculated width of the block.
 * @param blk_height Pointer to store the calculated height of the block.
 */
static void draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, int blk_num, int num_columns, int num_rows, float parent_width, float parent_height, float* blk_width, float* blk_height);
std::vector<AtomBlockId> collect_pb_atoms(const t_pb* pb);
void collect_pb_atoms_recurr(const t_pb* pb, std::vector<AtomBlockId>& atoms);
t_pb* highlight_sub_block_helper(const ClusterBlockId clb_index, t_pb* pb, const ezgl::point2d& local_pt, int max_depth);

#ifndef NO_GRAPHICS
static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* pb, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::renderer* g);
void draw_atoms_fanin_fanout_flylines(const std::vector<AtomBlockId>& atoms, ezgl::renderer* g);
void draw_selected_pb_flylines(ezgl::renderer* g);
void draw_one_logical_connection(const AtomPinId src_pin, const AtomPinId sink_pin, ezgl::renderer* g);
#endif /* NO_GRAPHICS */

/************************* Subroutine definitions begin *********************************/

void draw_internal_alloc_blk() {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const auto& device_ctx = g_vpr_ctx.device();
    t_pb_graph_node* pb_graph_head;

    /* Create a vector holding coordinate information for each type of physical logic
     * block.
     */
    draw_coords->blk_info.resize(device_ctx.logical_block_types.size());

    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        pb_graph_head = type.pb_graph_head;

        /* Create an vector with size equal to the total number of pins for each type
         * of physical logic block, in order to uniquely identify each sub-block in
         * the pb_graph of that type.
         */
        draw_coords->blk_info.at(type.index).subblk_array.resize(pb_graph_head->total_pb_pins);
    }
}

void draw_internal_init_blk() {
    /* Call accessor function to retrieve global variables. */
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    t_pb_graph_node* pb_graph_head_node;

    auto& device_ctx = g_vpr_ctx.device();
    for (const auto& type : device_ctx.logical_block_types) {
        /* Empty block has no sub_blocks */
        if (is_empty_type(&type)) {
            continue;
        }

        pb_graph_head_node = type.pb_graph_head;
        int type_descriptor_index = type.index;

        //We use the maximum over all tiles which can implement this logical block type
        int num_sub_tiles = 1;
        int width = 1;
        int height = 1;
        for (const auto& tile : type.equivalent_tiles) {
            num_sub_tiles = std::max(num_sub_tiles, tile->capacity);
            width = std::max(width, tile->width);
            height = std::max(height, tile->height);
        }

        // set the clb dimensions
        ezgl::rectangle& clb_bbox = draw_coords->blk_info.at(type_descriptor_index).subblk_array.at(0);
        ezgl::point2d bot_left = clb_bbox.bottom_left();
        ezgl::point2d top_right = clb_bbox.top_right();

        // note, that all clbs of the same type are the same size,
        // and that consequently we have *one* model for each type.
        bot_left = {0, 0};
        if (size_t(width) > device_ctx.grid.width() || size_t(height) > device_ctx.grid.height()) {
            // in this case, the clb certainly wont't fit, but this prevents
            // an out-of-bounds access, and provides some sort of (probably right)
            // value
            top_right = ezgl::point2d(
                (draw_coords->tile_x[1] - draw_coords->tile_x[0]) * (width - 1),
                (draw_coords->tile_y[1] - draw_coords->tile_y[0]) * (height - 1));
        } else {
            top_right = ezgl::point2d(
                draw_coords->tile_x[width - 1],
                draw_coords->tile_y[height - 1]);
        }
        top_right += ezgl::point2d(
            draw_coords->get_tile_width() / num_sub_tiles,
            draw_coords->get_tile_width());

        clb_bbox = ezgl::rectangle(bot_left, top_right);
        draw_internal_load_coords(type_descriptor_index, pb_graph_head_node,
                                  clb_bbox.width(), clb_bbox.height());

        /* Determine the max number of sub_block levels in the FPGA */
        draw_state->max_sub_blk_lvl = std::max(draw_internal_find_max_lvl(*type.pb_type),
                                               draw_state->max_sub_blk_lvl);
    }
    //draw_state->max_sub_blk_lvl -= 1;
}

#ifndef NO_GRAPHICS
void draw_internal_draw_subblk(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (!draw_state->show_blk_internal) {
        return;
    }
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& grid_blocks = draw_state->get_graphics_blk_loc_registry_ref().grid_blocks();

    int total_layer_num = device_ctx.grid.get_num_layers();

    for (int layer_num = 0; layer_num < total_layer_num; layer_num++) {
        if (draw_state->draw_layer_display[layer_num].visible) {
            for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
                for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                    /* Only the first block of a group should control drawing */
                    const auto& type = device_ctx.grid.get_physical_type({i, j, layer_num});
                    int width_offset = device_ctx.grid.get_width_offset({i, j, layer_num});
                    int height_offset = device_ctx.grid.get_height_offset({i, j, layer_num});

                    if (width_offset > 0 || height_offset > 0)
                        continue;

                    /* Don't draw if tile is empty. This includes corners. */
                    if (is_empty_type(type))
                        continue;

                    int num_sub_tiles = type->capacity;
                    for (int k = 0; k < num_sub_tiles; ++k) {
                        /* Don't draw if block is empty. */
                        if (!grid_blocks.block_at_location({i, j, k, layer_num})) {
                            continue;
                        }

                        /* Get block ID */
                        ClusterBlockId bnum = grid_blocks.block_at_location({i, j, k, layer_num});
                        /* Safety check, that physical blocks exists in the CLB */
                        if (cluster_ctx.clb_nlist.block_pb(bnum) == nullptr) {
                            continue;
                        }
                        draw_internal_pb(bnum, cluster_ctx.clb_nlist.block_pb(bnum), ezgl::rectangle({0, 0}, 0, 0), cluster_ctx.clb_nlist.block_type(bnum), g);
                    }
                }
            }
        }
    }
    //Draw the atom-level net flylines for the selected pb
    //(inputs: blue, outputs: red, internal: orange)
    draw_selected_pb_flylines(g);
}
#endif /* NO_GRAPHICS */

/* This function traverses through the pb_graph of a certain physical block type and
 * finds the maximum sub-block levels for that type.
 */
static int draw_internal_find_max_lvl(const t_pb_type& pb_type) {
    int i, j;
    t_mode mode;
    int max_levels = 0;

    /* If pb_type is a primitive, we have reached the end of pb_graph */
    if (pb_type.is_primitive())
        return (pb_type.depth);

    for (i = 0; i < pb_type.num_modes; ++i) {
        mode = pb_type.modes[i];

        for (j = 0; j < mode.num_pb_type_children; ++j) {
            max_levels = std::max(draw_internal_find_max_lvl(mode.pb_type_children[j]), max_levels);
        }
    }
    return max_levels;
}

static int get_num_child_blocks(const t_mode& mode) {
    // not all child_pb_types have the same number of physical blocks, so we have to manually loop through and count the physical blocks
    int num_blocks = 0;
    for (int j = 0; j < mode.num_pb_type_children; ++j) {
        num_blocks += mode.pb_type_children[j].num_pb;
    }
    return num_blocks;
}

/* Helper function for initializing bounding box values for each sub-block. This function
 * traverses through the pb_graph for a descriptor_type (given by type_descrip_index), and
 * calls helper function to compute bounding box values.
 */
static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, float parent_width, float parent_height) {
    float blk_width = 0.;
    float blk_height = 0.;

    t_pb_type* pb_type = pb_graph_node->pb_type;
    int num_modes = pb_type->num_modes;
    /* If pb_type is primitive, we have reached the end of pb_graph */
    if (pb_type->is_primitive()) {
        return;
    }

    for (int i = 0; i < num_modes; ++i) {
        t_mode mode = pb_type->modes[i];
        int num_children = mode.num_pb_type_children;
        int num_blocks = get_num_child_blocks(mode);
        int blk_num = 0;

        for (int j = 0; j < num_children; ++j) {
            // Find the number of instances for each child pb_type.
            int num_pb = mode.pb_type_children[j].num_pb;

            // Determine how we want to arrange the sub-blocks in the parent block.
            // We want the blocks to be squarish, and not too wide or too tall.
            // In other words, we want the number of rows to be as close to the number of columns as possible such that
            // num_rows * num_columns =  num_blocks.
            // first, determine the "middle" factor for the number of columns
            int num_columns = 1;
            for (int k = 1; k * k <= num_blocks; ++k) {
                if (num_blocks % k == 0) {
                    num_columns = k;
                }
            }
            int num_rows = num_blocks / num_columns;
            // Currently num_rows >= num_columns

            // If blocks are too wide, swap num_rows and num_columns, so that num_rows <= num_columns.
            const int MAX_WIDTH_HEIGHT_RATIO = 2;
            if (parent_width > parent_height * MAX_WIDTH_HEIGHT_RATIO) {
                std::swap(num_columns, num_rows);
            }

            for (int k = 0; k < num_pb; ++k) {

                // Compute bound box for block. Don't call if pb_type is root-level pb.
                draw_internal_calc_coords(type_descrip_index,
                                          &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                          blk_num, num_columns, num_rows,
                                          parent_width, parent_height,
                                          &blk_width, &blk_height);

                // Traverse to next level in the pb_graph
                draw_internal_load_coords(type_descrip_index,
                                          &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                          blk_width, blk_height);

                blk_num++;
            }
        }
    }
}

/* Helper function which computes bounding box values for a sub-block. The coordinates
 * are relative to the left and bottom corner of the parent block.
 */
static void
draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, int blk_num, int num_columns, int num_rows, float parent_width, float parent_height, float* blk_width, float* blk_height) {

    // get the bbox for this pb type
    ezgl::rectangle& pb_bbox = get_draw_coords_vars()->blk_info.at(type_descrip_index).get_pb_bbox_ref(*pb_graph_node);

    float tile_width = get_draw_coords_vars()->get_tile_width();

    constexpr float FRACTION_PARENT_PADDING = 0.005;
    constexpr float FRACTION_CHILD_MARGIN = 0.003;

    float abs_parent_padding = tile_width * FRACTION_PARENT_PADDING;
    float abs_text_padding = tile_width * FRACTION_TEXT_PADDING;
    float abs_child_margin = tile_width * FRACTION_CHILD_MARGIN;

    // add safety check to ensure that the dimensions will never be below zero
    if (parent_width <= 2 * abs_parent_padding || parent_height <= 2 * abs_parent_padding - abs_text_padding) {
        abs_parent_padding = 0;
        abs_text_padding = 0;
    }

    /* Draw all child-level blocks using most of the space inside their parent block. */
    float parent_drawing_width = parent_width - 2 * abs_parent_padding;
    float parent_drawing_height = parent_height - 2 * abs_parent_padding - abs_text_padding;

    int x_index = blk_num % num_columns;
    int y_index = blk_num / num_columns;

    float child_width = parent_drawing_width / num_columns;
    float child_height = parent_drawing_height / num_rows;

    // add safety check to ensure that the dimensions will never be below zero
    if (child_width <= abs_child_margin * 2 || child_height <= abs_child_margin * 2) {
        abs_child_margin = 0;
    }

    // The starting point to draw the physical block.
    double left = child_width * x_index + abs_parent_padding + abs_child_margin;
    double bot = child_height * y_index + abs_parent_padding + abs_child_margin;

    child_width -= abs_child_margin * 2;
    child_height -= abs_child_margin * 2;

    /* Endpoint for drawing the pb_type */
    double right = left + child_width;
    double top = bot + child_height;

    pb_bbox = ezgl::rectangle({right, top}, {left, bot});

    *blk_width = child_width;
    *blk_height = child_height;
}

#ifndef NO_GRAPHICS
/* Helper subroutine to draw all sub-blocks. This function traverses through the pb_graph
 * which a netlist block can map to, and draws each sub-block inside its parent block. With
 * each click on the "Blk Internal" button, a new level is shown.
 */
static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* pb, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    t_selected_sub_block_info& sel_sub_info = get_selected_sub_block_info();

    t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    ezgl::rectangle temp = draw_coords->get_pb_bbox(clb_index, *pb->pb_graph_node);
    ezgl::rectangle abs_bbox = temp + parent_bbox.bottom_left();

    int layer_num = block_locs[clb_index].loc.layer;
    int transparency_factor = draw_state->draw_layer_display[layer_num].alpha;

    // if we've gone too far, don't draw anything
    if (pb_type->depth > draw_state->show_blk_internal) {
        return;
    }

    // first draw box

    if (pb->name != nullptr) {
        // If block is used, draw it in colour with solid border.
        g->set_line_dash(ezgl::line_dash::none);

        // determine default background color
        if (sel_sub_info.is_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(SELECTED_COLOR, transparency_factor);
        } else if (sel_sub_info.is_sink_of_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(DRIVES_IT_COLOR, transparency_factor);
        } else if (sel_sub_info.is_source_of_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(DRIVEN_BY_IT_COLOR, transparency_factor);
        } else {
            g->set_color(draw_state->block_color(clb_index), transparency_factor);
        }
    } else {
        // If block is not used, draw as empty block (ie. white
        // background with dashed border).

        g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
        g->set_color(ezgl::WHITE);
    }
    g->fill_rectangle(abs_bbox);
    g->set_color(ezgl::BLACK, transparency_factor);

    if (draw_state->draw_block_outlines) {
        g->draw_rectangle(abs_bbox);
    }

    // Display text for each physical block.
    std::string pb_display_text(pb_type->name);
    std::string pb_type_name(pb_type->name);

    if (!pb->is_primitive()) {
        // Format for non-primitives: <block_type_name>[<placement_index>]:<mode_name>
        std::string mode_name = pb->pb_graph_node->pb_type->modes[pb->mode].name;
        pb_display_text += "[" + std::to_string(pb->pb_graph_node->placement_index) + "]";

        // Don't display mode name if it is the same as the pb_type name
        if (mode_name != pb_type_name) {
            pb_display_text += ":" + mode_name;
        }
    } else {
        // Format for primitives: <block_type_name>(<block_name>)
        if (pb->name != nullptr) {
            std::string pb_name(pb->name);
            pb_display_text += "(" + pb_name + ")";
        }
    }

    g->set_font_size(16);
    if (pb_type->depth == draw_state->show_blk_internal || pb->child_pbs == nullptr) {
        // If this pb is at the lowest displayed level, or has no more children, then
        // label it in the center with its type and name

        if (draw_state->draw_block_text) {
            g->draw_text(
                abs_bbox.center(),
                pb_display_text.c_str(),
                abs_bbox.width(),
                abs_bbox.height());
        }

    } else {
        // else (ie. has chilren, and isn't at the lowest displayed level)
        // just label its type, and put it up at the top so we can see it
        if (draw_state->draw_block_text) {
            g->draw_text(
                ezgl::point2d(abs_bbox.center_x(),
                              abs_bbox.top() - draw_coords->get_tile_height() * FRACTION_TEXT_PADDING),
                pb_display_text.c_str(),
                abs_bbox.width(),
                abs_bbox.height());
        }
    }

    // now recurse on the child pbs.

    // return if no children, or this is an unusused pb,
    // or if going down will be too far down (this one is redundant, but for optimazition)
    if (pb->child_pbs == nullptr || pb->name == nullptr
        || pb_type->depth == draw_state->show_blk_internal) {
        return;
    }

    int num_child_types = pb->get_num_child_types();
    for (int i = 0; i < num_child_types; ++i) {
        if (pb->child_pbs[i] == nullptr) {
            continue;
        }

        int num_pb = pb->get_num_children_of_type(i);
        for (int j = 0; j < num_pb; ++j) {
            t_pb* child_pb = &pb->child_pbs[i][j];

            VTR_ASSERT(child_pb != nullptr);

            t_pb_type* pb_child_type = child_pb->pb_graph_node->pb_type;

            if (pb_child_type == nullptr) {
                continue;
            }

            // now recurse
            draw_internal_pb(clb_index, child_pb, abs_bbox, type, g);
        }
    }
}

void draw_selected_pb_flylines(ezgl::renderer* g) {
    t_selected_sub_block_info& sel_sub_info = get_selected_sub_block_info();

    const t_pb* pb = sel_sub_info.get_selected_pb();

    if (pb) {
        auto atoms = collect_pb_atoms(pb);
        draw_atoms_fanin_fanout_flylines(atoms, g);
    }
}

void draw_atoms_fanin_fanout_flylines(const std::vector<AtomBlockId>& atoms, ezgl::renderer* g) {
    std::set<AtomBlockId> atoms_set(atoms.begin(), atoms.end());

    auto& atom_nl = g_vpr_ctx.atom().netlist();

    g->set_line_dash(ezgl::line_dash::none);
    g->set_line_width(2);

    for (AtomBlockId blk : atoms) {
        for (AtomPinId ipin : atom_nl.block_input_pins(blk)) {
            AtomNetId net = atom_nl.pin_net(ipin);

            AtomPinId net_driver = atom_nl.net_driver(net);
            AtomBlockId net_driver_blk = atom_nl.pin_block(net_driver);

            if (atoms_set.count(net_driver_blk)) {
                g->set_color(ezgl::ORANGE); //Internal
            } else {
                g->set_color(ezgl::BLUE); //External input
            }

            ezgl::point2d start = atom_pin_draw_coord(net_driver);
            ezgl::point2d end = atom_pin_draw_coord(ipin);
            g->draw_line(start, end);
            draw_triangle_along_line(g, start, end, 0.95, 40 * DEFAULT_ARROW_SIZE);
            draw_triangle_along_line(g, start, end, 0.05, 40 * DEFAULT_ARROW_SIZE);
        }

        for (AtomPinId opin : atom_nl.block_output_pins(blk)) {
            AtomNetId net = atom_nl.pin_net(opin);

            for (AtomPinId net_sink : atom_nl.net_sinks(net)) {
                AtomBlockId net_sink_blk = atom_nl.pin_block(net_sink);

                if (atoms_set.count(net_sink_blk)) {
                    g->set_color(ezgl::ORANGE); //Internal
                } else {
                    g->set_color(ezgl::RED); //External output
                }

                ezgl::point2d start = atom_pin_draw_coord(opin);
                ezgl::point2d end = atom_pin_draw_coord(net_sink);
                g->draw_line(start, end);
                draw_triangle_along_line(g, start, end, 0.95, 40 * DEFAULT_ARROW_SIZE);
                draw_triangle_along_line(g, start, end, 0.05, 40 * DEFAULT_ARROW_SIZE);
            }
        }
    }
}
#endif /* NO_GRAPHICS */

std::vector<AtomBlockId> collect_pb_atoms(const t_pb* pb) {
    std::vector<AtomBlockId> atoms;
    collect_pb_atoms_recurr(pb, atoms);
    return atoms;
}

void collect_pb_atoms_recurr(const t_pb* pb, std::vector<AtomBlockId>& atoms) {
    auto& atom_ctx = g_vpr_ctx.atom();

    if (pb->is_primitive()) {
        //Base case
        AtomBlockId blk = atom_ctx.lookup().atom_pb_bimap().pb_atom(pb);
        if (blk) {
            atoms.push_back(blk);
        }
    } else {
        //Recurse
        VTR_ASSERT_DEBUG(atom_ctx.lookup().atom_pb_bimap().pb_atom(pb) == AtomBlockId::INVALID());

        for (int itype = 0; itype < pb->get_num_child_types(); ++itype) {
            for (int ichild = 0; ichild < pb->get_num_children_of_type(itype); ++ichild) {
                collect_pb_atoms_recurr(&pb->child_pbs[itype][ichild], atoms);
            }
        }
    }
}

#ifndef NO_GRAPHICS
void draw_logical_connections(ezgl::renderer* g) {
    const t_selected_sub_block_info& sel_subblk_info = get_selected_sub_block_info();
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& atom_ctx = g_vpr_ctx.atom();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    g->set_line_dash(ezgl::line_dash::none);

    //constexpr float NET_ALPHA = 0.0275;
    float NET_ALPHA = draw_state->net_alpha;
    int transparency_factor;

    // iterate over all the atom nets
    for (auto net_id : atom_ctx.netlist().nets()) {
        if ((int)atom_ctx.netlist().net_pins(net_id).size() - 1 > draw_state->draw_net_max_fanout) {
            continue;
        }

        AtomPinId driver_pin_id = atom_ctx.netlist().net_driver(net_id);
        AtomBlockId src_blk_id = atom_ctx.netlist().pin_block(driver_pin_id);
        ClusterBlockId src_clb = atom_ctx.lookup().atom_clb(src_blk_id);

        int src_layer_num = block_locs[src_clb].loc.layer;
        //To only show primitive nets that are connected to currently active layers on the screen
        if (!draw_state->draw_layer_display[src_layer_num].visible) {
            continue; /* Don't Draw */
        }

        const t_pb_graph_node* src_pb_gnode = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(src_blk_id);
        bool src_is_selected = sel_subblk_info.is_in_selected_subtree(src_pb_gnode, src_clb);
        bool src_is_src_of_selected = sel_subblk_info.is_source_of_selected(src_pb_gnode, src_clb);

        // iterate over the sinks
        for (auto sink_pin_id : atom_ctx.netlist().net_sinks(net_id)) {
            AtomBlockId sink_blk_id = atom_ctx.netlist().pin_block(sink_pin_id);
            const t_pb_graph_node* sink_pb_gnode = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(sink_blk_id);
            ClusterBlockId sink_clb = atom_ctx.lookup().atom_clb(sink_blk_id);
            int sink_layer_num = block_locs[sink_clb].loc.layer;

            t_draw_layer_display element_visibility = get_element_visibility_and_transparency(src_layer_num, sink_layer_num);

            if (!element_visibility.visible) {
                continue; /* Don't Draw */
            }

            transparency_factor = element_visibility.alpha;

            //color selection
            //transparency factor is the most transparent of the 2 options that the user selects from the UI
            if (src_is_selected && sel_subblk_info.is_sink_of_selected(sink_pb_gnode, sink_clb)) {
                g->set_color(DRIVES_IT_COLOR, fmin(transparency_factor, DRIVES_IT_COLOR.alpha * NET_ALPHA));
            } else if (src_is_src_of_selected && sel_subblk_info.is_in_selected_subtree(sink_pb_gnode, sink_clb)) {
                g->set_color(DRIVEN_BY_IT_COLOR, fmin(transparency_factor, DRIVEN_BY_IT_COLOR.alpha * NET_ALPHA));
            } else if (draw_state->draw_nets == DRAW_FLYLINES && draw_state->show_nets && (draw_state->showing_sub_blocks() || src_clb != sink_clb)) {
                g->set_color(ezgl::BLACK, fmin(transparency_factor, ezgl::BLACK.alpha * NET_ALPHA)); // if showing all, draw the other ones in black
            } else {
                continue; // not showing all, and not the specified block, so skip
            }

            draw_one_logical_connection(driver_pin_id, sink_pin_id, g);
        }
    }
}
#endif /* NO_GRAPHICS */

/**
 * Helper function for draw_one_logical_connection(...).
 * We return the index of the pin in the context of the *model*
 * This is used to determine where to draw connection start/end points
 *
 * The pin index in the model context is returned via pin_index.
 *
 * The total number of model pins (either inputs or outputs) is returned via total_pins
 *
 * The search inputs parameter tells this function to search
 * inputs (if true) or outputs (if false).
 */
void find_pin_index_at_model_scope(const AtomPinId pin_id, const AtomBlockId blk_id, int* pin_index, int* total_pins) {
    auto& atom_ctx = g_vpr_ctx.atom();
    const LogicalModels& models = g_vpr_ctx.device().arch->models;

    AtomPortId port_id = atom_ctx.netlist().pin_port(pin_id);
    const t_model_ports* model_port = atom_ctx.netlist().port_model(port_id);

    //Total up the port widths
    //  Note that we do this on the model since the atom netlist doesn't include unused ports
    int pin_cnt = 0;
    *pin_index = -1; //initialize
    const t_model& model = models.get_model(atom_ctx.netlist().block_model(blk_id));
    for (const t_model_ports* port : {model.inputs, model.outputs}) {
        while (port) {
            if (port == model_port) {
                //This is the port the pin is associated with, record it's index

                //Get the pin index in the port
                int atom_port_index = atom_ctx.netlist().pin_port_bit(pin_id);

                //The index of this pin in the model is the pins counted so-far
                //(i.e. across previous ports) plus the index in the port
                *pin_index = pin_cnt + atom_port_index;
            }

            //Running total of model pins seen so-far
            pin_cnt += port->size;

            port = port->next;
        }
    }

    VTR_ASSERT(*pin_index != -1);

    *total_pins = pin_cnt;
}

#ifndef NO_GRAPHICS
/**
 * Draws ONE logical connection from src_pin in src_lblk to sink_pin in sink_lblk.
 * The *_abs_bbox parameters are for mild optimization, as the absolute bbox can be calculated
 * more efficiently elsewhere.
 */
void draw_one_logical_connection(const AtomPinId src_pin, const AtomPinId sink_pin, ezgl::renderer* g) {
    ezgl::point2d src_point = atom_pin_draw_coord(src_pin);
    ezgl::point2d sink_point = atom_pin_draw_coord(sink_pin);

    // draw a link connecting the pins.
    g->draw_line(src_point, sink_point);

    const auto& atom_ctx = g_vpr_ctx.atom();
    if (atom_ctx.lookup().atom_clb(atom_ctx.netlist().pin_block(src_pin)) == atom_ctx.lookup().atom_clb(atom_ctx.netlist().pin_block(sink_pin))) {
        // if they are in the same clb, put one arrow in the center
        float center_x = (src_point.x + sink_point.x) / 2;
        float center_y = (src_point.y + sink_point.y) / 2;

        draw_triangle_along_line(g,
                                 center_x, center_y,
                                 src_point.x, sink_point.x,
                                 src_point.y, sink_point.y);
    } else {
        // if they are not, put 2 near each end
        draw_triangle_along_line(g, src_point, sink_point, 0.05);
        draw_triangle_along_line(g, src_point, sink_point, 0.95);
    }
}
#endif /* NO_GRAPHICS */

int highlight_sub_block(const ezgl::point2d& point_in_clb, ClusterBlockId clb_index, t_pb* pb) {
    t_draw_state* draw_state = get_draw_state_vars();

    int max_depth = draw_state->show_blk_internal;

    t_pb* new_selected_sub_block = highlight_sub_block_helper(clb_index, pb, point_in_clb, max_depth);
    if (new_selected_sub_block == nullptr) {
        get_selected_sub_block_info().clear();
        return 1;
    } else {
        get_selected_sub_block_info().set(new_selected_sub_block, clb_index);
        return 0;
    }
}

/**
 * The recursive part of highlight_sub_block. finds which pb is under
 * the given location. local_pt is relative to the given pb, and pb should
 * be in clb.
 */
t_pb* highlight_sub_block_helper(const ClusterBlockId clb_index, t_pb* pb, const ezgl::point2d& local_pt, int max_depth) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_pb_type* pb_type = pb->pb_graph_node->pb_type;

    // check to see if we are past the displayed level,
    // if pb has children,
    // and if pb is dud
    if (pb_type->depth + 1 > max_depth
        || pb->child_pbs == nullptr
        || pb_type->is_primitive()) {
        return nullptr;
    }

    int num_child_types = pb->get_num_child_types();

    // Iterate through each type of child pb, and each pb of that type.
    for (int i = 0; i < num_child_types; ++i) {
        int num_children_of_type = pb->get_num_children_of_type(i);

        for (int j = 0; j < num_children_of_type; ++j) {
            if (pb->child_pbs[i] == nullptr) {
                continue;
            }

            t_pb* child_pb = &pb->child_pbs[i][j];
            t_pb_graph_node* pb_child_node = child_pb->pb_graph_node;

            // get the bbox for this child
            const ezgl::rectangle& bbox = draw_coords->get_pb_bbox(clb_index, *pb_child_node);

            // If child block is being used, check if it intersects
            if (child_pb->name != nullptr && bbox.contains(local_pt)) {
                // check farther down the graph, see if we can find
                // something more specific.
                t_pb* subtree_result = highlight_sub_block_helper(
                    clb_index, child_pb, local_pt - bbox.bottom_left(), max_depth);
                if (subtree_result != nullptr) {
                    // we found something more specific.
                    return subtree_result;
                } else {
                    // couldn't find something more specific, return parent
                    return child_pb;
                }
            }
        }
    }
    return nullptr;
}

t_selected_sub_block_info& get_selected_sub_block_info() {
    // used to keep track of the selected sub-block.
    static t_selected_sub_block_info selected_sub_block_info;
    return selected_sub_block_info;
}

/*
 * Begin definition of t_selected_sub_block_info functions.
 */

t_selected_sub_block_info::t_selected_sub_block_info() {
    clear();
}

template<typename HashType>

void add_all_children(const t_pb* pb, const ClusterBlockId clb_index, std::unordered_set<t_selected_sub_block_info::gnode_clb_pair, HashType>& set) {
    if (pb == nullptr) {
        return;
    }

    set.insert(t_selected_sub_block_info::gnode_clb_pair(pb->pb_graph_node, clb_index));

    int num_child_types = pb->get_num_child_types();
    for (int i = 0; i < num_child_types; ++i) {
        int num_children_of_type = pb->get_num_children_of_type(i);
        for (int j = 0; j < num_children_of_type; ++j) {
            add_all_children(&pb->child_pbs[i][j], clb_index, set);
        }
    }
}

void t_selected_sub_block_info::set(t_pb* new_selected_sub_block, const ClusterBlockId new_containing_block_index) {
    selected_pb = new_selected_sub_block;
    selected_pb_gnode = (selected_pb == nullptr) ? nullptr : selected_pb->pb_graph_node;
    containing_block_index = new_containing_block_index;
    sinks.clear();
    sources.clear();
    in_selected_subtree.clear();

    auto& atom_ctx = g_vpr_ctx.atom();

    if (has_selection()) {
        add_all_children(selected_pb, containing_block_index, in_selected_subtree);

        for (auto blk_id : atom_ctx.netlist().blocks()) {
            const ClusterBlockId clb = atom_ctx.lookup().atom_clb(blk_id);
            const t_pb_graph_node* pb_graph_node = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(blk_id);
            // find the atom block that corrisponds to this pb.
            if (is_in_selected_subtree(pb_graph_node, clb)) {
                //Collect the sources of all nets driving this node
                for (auto pin_id : atom_ctx.netlist().block_input_pins(blk_id)) {
                    AtomNetId net_id = atom_ctx.netlist().pin_net(pin_id);
                    AtomPinId driver_pin_id = atom_ctx.netlist().net_driver(net_id);

                    AtomBlockId src_blk = atom_ctx.netlist().pin_block(driver_pin_id);

                    const ClusterBlockId src_clb = atom_ctx.lookup().atom_clb(src_blk);
                    const t_pb_graph_node* src_pb_graph_node = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(src_blk);

                    sources.insert(gnode_clb_pair(src_pb_graph_node, src_clb));
                }

                //Collect the sinks of all nets driven by this node
                for (auto pin_id : atom_ctx.netlist().block_output_pins(blk_id)) {
                    AtomNetId net_id = atom_ctx.netlist().pin_net(pin_id);
                    for (auto sink_pin_id : atom_ctx.netlist().net_sinks(net_id)) {
                        AtomBlockId sink_blk = atom_ctx.netlist().pin_block(sink_pin_id);

                        const ClusterBlockId sink_clb = atom_ctx.lookup().atom_clb(sink_blk);
                        const t_pb_graph_node* sink_pb_graph_node = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(sink_blk);

                        sinks.insert(gnode_clb_pair(sink_pb_graph_node, sink_clb));
                    }
                }
            }
        }
    }
}

void t_selected_sub_block_info::clear() {
    set(nullptr, ClusterBlockId::INVALID());
}

t_pb* t_selected_sub_block_info::get_selected_pb() const { return selected_pb; }

t_pb_graph_node* t_selected_sub_block_info::get_selected_pb_gnode() const { return selected_pb_gnode; }

ClusterBlockId t_selected_sub_block_info::get_containing_block() const { return containing_block_index; }

bool t_selected_sub_block_info::has_selection() const {
    return get_selected_pb_gnode() != nullptr && get_containing_block() != ClusterBlockId::INVALID();
}

bool t_selected_sub_block_info::is_selected(const t_pb_graph_node* test, const ClusterBlockId clb_index) const {
    return get_selected_pb_gnode() == test && get_containing_block() == clb_index;
}

bool t_selected_sub_block_info::is_sink_of_selected(const t_pb_graph_node* test, const ClusterBlockId clb_index) const {
    return sinks.find(gnode_clb_pair(test, clb_index)) != sinks.end();
}

bool t_selected_sub_block_info::is_source_of_selected(const t_pb_graph_node* test, const ClusterBlockId clb_index) const {
    return sources.find(gnode_clb_pair(test, clb_index)) != sources.end();
}

bool t_selected_sub_block_info::is_in_selected_subtree(const t_pb_graph_node* test, const ClusterBlockId clb_index) const {
    return in_selected_subtree.find(gnode_clb_pair(test, clb_index)) != in_selected_subtree.end();
}

/*
 * Begin definition of t_selected_sub_block_info::clb_pin_tuple functions.
 */

t_selected_sub_block_info::clb_pin_tuple::clb_pin_tuple(ClusterBlockId clb_index_, const t_pb_graph_node* pb_gnode_)
    : clb_index(clb_index_)
    , pb_gnode(pb_gnode_) {
}

t_selected_sub_block_info::clb_pin_tuple::clb_pin_tuple(const AtomPinId atom_pin) {
    auto& atom_ctx = g_vpr_ctx.atom();
    clb_index = atom_ctx.lookup().atom_clb(atom_ctx.netlist().pin_block(atom_pin));
    pb_gnode = atom_ctx.lookup().atom_pb_bimap().atom_pb_graph_node(atom_ctx.netlist().pin_block(atom_pin));
}

bool t_selected_sub_block_info::clb_pin_tuple::operator==(const clb_pin_tuple& rhs) const {
    return clb_index == rhs.clb_index && pb_gnode == rhs.pb_gnode;
}

/*
 * Begin definition of t_selected_sub_block_info::gnode_clb_pair functions
 */

t_selected_sub_block_info::gnode_clb_pair::gnode_clb_pair(const t_pb_graph_node* pb_gnode_, const ClusterBlockId clb_index_)
    : pb_gnode(pb_gnode_)
    , clb_index(clb_index_) {
}

bool t_selected_sub_block_info::gnode_clb_pair::operator==(const gnode_clb_pair& rhs) const {
    return clb_index == rhs.clb_index
           && pb_gnode == rhs.pb_gnode;
}

/**
 * @brief Recursively looks through pb graph to find block w. given name
 *
 * @param name name of block being searched for
 * @param pb current node to be examined
 * @return t_pb* t_pb ptr of block w. name "name". Returns nullptr if nothing found
 */
t_pb* find_atom_block_in_pb(const std::string& name, t_pb* pb) {
    //Checking if block is one being searched for
    std::string pbName(pb->name);
    if (pbName == name)
        return pb;
    //If block has no children, returning
    if (pb->child_pbs == nullptr)
        return nullptr;
    int num_child_types = pb->get_num_child_types();
    //Iterating through all child types
    for (int i = 0; i < num_child_types; ++i) {
        if (pb->child_pbs[i] == nullptr) continue;
        int num_children_of_type = pb->get_num_children_of_type(i);
        //Iterating through all of pb's children of given type
        for (int j = 0; j < num_children_of_type; ++j) {
            t_pb* child_pb = &pb->child_pbs[i][j];
            //If child exists, recursively calling function on it
            if (child_pb->name != nullptr) {
                t_pb* subtree_result = find_atom_block_in_pb(name, child_pb);
                //If a result is found, returning it to top of recursive calls
                if (subtree_result != nullptr) {
                    return subtree_result;
                }
            }
        }
    }
    return nullptr;
}

#endif // NO_GRAPHICS
