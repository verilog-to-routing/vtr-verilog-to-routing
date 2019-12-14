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

#    include <cstdio>
#    include <algorithm>
#    include <string.h>

#    include "vtr_assert.h"
#    include "vtr_memory.h"

#    include "intra_logic_block.h"
#    include "globals.h"
#    include "atom_netlist.h"
#    include "vpr_utils.h"
#    include "draw_global.h"
#    include "draw.h"
#    include "draw_color.h"

/************************* Subroutines local to this file. *******************************/

static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, float parent_width, float parent_height);
static int draw_internal_find_max_lvl(const t_pb_type& pb_type);
static void draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, int num_pb_types, int type_index, int num_pb, int pb_index, float parent_width, float parent_height, float* blk_width, float* blk_height);
std::vector<AtomBlockId> collect_pb_atoms(const t_pb* pb);
void collect_pb_atoms_recurr(const t_pb* pb, std::vector<AtomBlockId>& atoms);
t_pb* highlight_sub_block_helper(const ClusterBlockId clb_index, t_pb* pb, const ezgl::point2d& local_pt, int max_depth);

#    ifndef NO_GRAPHICS
static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* pb, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::renderer* g);
void draw_atoms_fanin_fanout_flylines(const std::vector<AtomBlockId>& atoms, ezgl::renderer* g);
void draw_selected_pb_flylines(ezgl::renderer* g);
void draw_one_logical_connection(const AtomPinId src_pin, const AtomPinId sink_pin, ezgl::renderer* g);
#    endif /* NO_GRAPHICS */

/************************* Subroutine definitions begin *********************************/

void draw_internal_alloc_blk() {
    t_draw_coords* draw_coords;
    t_pb_graph_node* pb_graph_head;

    /* Call accessor function to retrieve global variables. */
    draw_coords = get_draw_coords_vars();

    /* Create a vector holding coordinate information for each type of physical logic
     * block.
     */
    auto& device_ctx = g_vpr_ctx.device();
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

#    ifndef NO_GRAPHICS
void draw_internal_draw_subblk(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (!draw_state->show_blk_internal) {
        return;
    }
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            /* Only the first block of a group should control drawing */
            if (device_ctx.grid[i][j].width_offset > 0 || device_ctx.grid[i][j].height_offset > 0)
                continue;

            /* Don't draw if tile is empty. This includes corners. */
            if (is_empty_type(device_ctx.grid[i][j].type))
                continue;

            int num_sub_tiles = device_ctx.grid[i][j].type->capacity;
            for (int k = 0; k < num_sub_tiles; ++k) {
                /* Don't draw if block is empty. */
                if (place_ctx.grid_blocks[i][j].blocks[k] == EMPTY_BLOCK_ID || place_ctx.grid_blocks[i][j].blocks[k] == INVALID_BLOCK_ID)
                    continue;

                /* Get block ID */
                ClusterBlockId bnum = place_ctx.grid_blocks[i][j].blocks[k];
                /* Safety check, that physical blocks exists in the CLB */
                if (cluster_ctx.clb_nlist.block_pb(bnum) == nullptr)
                    continue;
                draw_internal_pb(bnum, cluster_ctx.clb_nlist.block_pb(bnum), ezgl::rectangle({0, 0}, 0, 0), cluster_ctx.clb_nlist.block_type(bnum), g);
            }
        }
    }
    //Draw the atom-level net flylines for the selected pb
    //(inputs: blue, outputs: red, internal: orange)
    draw_selected_pb_flylines(g);
}
#    endif /* NO_GRAPHICS */

/* This function traverses through the pb_graph of a certain physical block type and
 * finds the maximum sub-block levels for that type.
 */
static int draw_internal_find_max_lvl(const t_pb_type& pb_type) {
    int i, j;
    t_mode mode;
    int max_levels = 0;

    /* If no modes, we have reached the end of pb_graph */
    if (pb_type.num_modes == 0)
        return (pb_type.depth);

    for (i = 0; i < pb_type.num_modes; ++i) {
        mode = pb_type.modes[i];

        for (j = 0; j < mode.num_pb_type_children; ++j) {
            max_levels = std::max(draw_internal_find_max_lvl(mode.pb_type_children[j]), max_levels);
        }
    }
    return max_levels;
}

/* Helper function for initializing bounding box values for each sub-block. This function
 * traverses through the pb_graph for a descriptor_type (given by type_descrip_index), and
 * calls helper function to compute bounding box values.
 */
static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, float parent_width, float parent_height) {
    int i, j, k;
    t_pb_type* pb_type;
    int num_modes, num_children, num_pb;
    t_mode mode;
    float blk_width = 0.;
    float blk_height = 0.;

    /* Get information about the pb_type */
    pb_type = pb_graph_node->pb_type;
    num_modes = pb_type->num_modes;

    /* If no modes, we have reached the end of pb_graph */
    if (num_modes == 0)
        return;

    for (i = 0; i < num_modes; ++i) {
        mode = pb_type->modes[i];
        num_children = mode.num_pb_type_children;

        for (j = 0; j < num_children; ++j) {
            /* Find the number of instances for each child pb_type. */
            num_pb = mode.pb_type_children[j].num_pb;

            for (k = 0; k < num_pb; ++k) {
                /* Compute bound box for block. Don't call if pb_type is root-level pb. */
                draw_internal_calc_coords(type_descrip_index,
                                          &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                          num_children, j, num_pb, k,
                                          parent_width, parent_height,
                                          &blk_width, &blk_height);

                /* Traverse to next level in the pb_graph */
                draw_internal_load_coords(type_descrip_index,
                                          &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                          blk_width, blk_height);
            }
        }
    }
    return;
}

/* Helper function which computes bounding box values for a sub-block. The coordinates
 * are relative to the left and bottom corner of the parent block.
 */
static void
draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node* pb_graph_node, int num_pb_types, int type_index, int num_pb, int pb_index, float parent_width, float parent_height, float* blk_width, float* blk_height) {
    float parent_drawing_width, parent_drawing_height;
    float sub_tile_x, sub_tile_y;
    float child_width, child_height;
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    // get the bbox for this pb type
    ezgl::rectangle& pb_bbox = get_draw_coords_vars()->blk_info.at(type_descrip_index).get_pb_bbox_ref(*pb_graph_node);

    const float FRACTION_PARENT_PADDING_X = 0.01;

    const float NORMAL_FRACTION_PARENT_HEIGHT = 0.90;
    float capacity_divisor = 1;
    const float FRACTION_PARENT_PADDING_BOTTOM = 0.01;

    const float FRACTION_CHILD_MARGIN_X = 0.025;
    const float FRACTION_CHILD_MARGIN_Y = 0.04;
    double left, bot, right, top;

    int capacity = device_ctx.physical_tile_types[type_descrip_index].capacity;
    if (capacity > 1 && device_ctx.grid.width() > 0 && device_ctx.grid.height() > 0 && place_ctx.grid_blocks[1][0].usage != 0
        && type_descrip_index == device_ctx.grid[1][0].type->index) {
        // that should test for io blocks, and setting capacity_divisor > 1
        // will squish every thing down
        capacity_divisor = capacity - 1;
    }

    /* Draw all child-level blocks in just most of the space inside their parent block. */
    parent_drawing_width = parent_width * (1 - FRACTION_PARENT_PADDING_X * 2);
    parent_drawing_height = parent_height * (NORMAL_FRACTION_PARENT_HEIGHT / capacity_divisor);

    /* The left and bottom corner (inside the parent block) of the space to draw
     * child blocks.
     */
    sub_tile_x = parent_width * FRACTION_PARENT_PADDING_X;
    sub_tile_y = parent_height * FRACTION_PARENT_PADDING_BOTTOM;

    /* Divide parent_drawing_width by the number of child types. */
    child_width = parent_drawing_width / num_pb_types;
    /* Divide parent_drawing_height by the number of instances of the pb_type. */
    child_height = parent_drawing_height / num_pb;

    /* The starting point to draw the physical block. */
    left = child_width * type_index + sub_tile_x + FRACTION_CHILD_MARGIN_X * child_width;
    bot = child_height * pb_index + sub_tile_y + FRACTION_CHILD_MARGIN_Y * child_height;

    /* Leave some space between different pb_types. */
    child_width *= 1 - FRACTION_CHILD_MARGIN_X * 2;
    /* Leave some space between different instances of the same type. */
    child_height *= 1 - FRACTION_CHILD_MARGIN_Y * 2;

    /* Endpoint for drawing the pb_type */
    right = left + child_width;
    top = bot + child_height;

    pb_bbox = ezgl::rectangle({right, top}, {left, bot});

    *blk_width = child_width;
    *blk_height = child_height;

    return;
}

#    ifndef NO_GRAPHICS
/* Helper subroutine to draw all sub-blocks. This function traverses through the pb_graph
 * which a netlist block can map to, and draws each sub-block inside its parent block. With
 * each click on the "Blk Internal" button, a new level is shown.
 */
static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* pb, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    t_selected_sub_block_info& sel_sub_info = get_selected_sub_block_info();

    t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    ezgl::rectangle temp = draw_coords->get_pb_bbox(clb_index, *pb->pb_graph_node);
    ezgl::rectangle abs_bbox = temp + parent_bbox.bottom_left();

    // if we've gone too far, don't draw anything
    if (pb_type->depth > draw_state->show_blk_internal) {
        return;
    }

    /// first draw box ///

    if (pb->name != nullptr) {
        // If block is used, draw it in colour with solid border.
        g->set_line_dash(ezgl::line_dash::none);

        // determine default background color
        if (sel_sub_info.is_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(SELECTED_COLOR);
        } else if (sel_sub_info.is_sink_of_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(DRIVES_IT_COLOR);
        } else if (sel_sub_info.is_source_of_selected(pb->pb_graph_node, clb_index)) {
            g->set_color(DRIVEN_BY_IT_COLOR);
        } else {
            g->set_color(draw_state->block_color(clb_index));
        }
    } else {
        // If block is not used, draw as empty block (ie. white
        // background with dashed border).

        g->set_line_dash(ezgl::line_dash::asymmetric_5_3);
        g->set_color(ezgl::WHITE);
    }
    g->fill_rectangle(abs_bbox);
    g->set_color(ezgl::BLACK);
    g->draw_rectangle(abs_bbox);

    /// then draw text ///

    if (pb->name != nullptr) {
        g->set_font_size(16); // note: calc_text_xbound(...) assumes this is 16
        if (pb_type->depth == draw_state->show_blk_internal || pb->child_pbs == nullptr) {
            // If this pb is at the lowest displayed level, or has no more children, then
            // label it in the center with its type and name

            int type_len = strlen(pb_type->name);
            int name_len = strlen(pb->name);
            int tot_len = type_len + name_len;
            char* blk_tag = (char*)vtr::malloc((tot_len + 8) * sizeof(char));

            sprintf(blk_tag, "%s (%s)", pb_type->name, pb->name);

            g->draw_text(
                abs_bbox.center(),
                blk_tag,
                abs_bbox.width(),
                abs_bbox.height());

            free(blk_tag);
        } else {
            // else (ie. has chilren, and isn't at the lowest displayed level)
            // just label its type, and put it up at the top so we can see it
            g->draw_text(
                ezgl::point2d(abs_bbox.center_x(),
                              abs_bbox.top() - (abs_bbox.height()) / 15.0),
                pb_type->name,
                abs_bbox.width(),
                abs_bbox.height());
        }
    } else {
        // If child block is not used, label it only by its type
        g->draw_text(
            abs_bbox.center(),
            pb_type->name,
            abs_bbox.width(),
            abs_bbox.height());
    }

    /// now recurse on the child pbs. ///

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

    auto& atom_nl = g_vpr_ctx.atom().nlist;

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
#    endif /* NO_GRAPHICS */

std::vector<AtomBlockId> collect_pb_atoms(const t_pb* pb) {
    std::vector<AtomBlockId> atoms;
    collect_pb_atoms_recurr(pb, atoms);
    return atoms;
}

void collect_pb_atoms_recurr(const t_pb* pb, std::vector<AtomBlockId>& atoms) {
    auto& atom_ctx = g_vpr_ctx.atom();

    if (pb->is_primitive()) {
        //Base case
        AtomBlockId blk = atom_ctx.lookup.pb_atom(pb);
        if (blk) {
            atoms.push_back(blk);
        }
    } else {
        //Recurse
        VTR_ASSERT_DEBUG(atom_ctx.lookup.pb_atom(pb) == AtomBlockId::INVALID());

        for (int itype = 0; itype < pb->get_num_child_types(); ++itype) {
            for (int ichild = 0; ichild < pb->get_num_children_of_type(itype); ++ichild) {
                collect_pb_atoms_recurr(&pb->child_pbs[itype][ichild], atoms);
            }
        }
    }
}

#    ifndef NO_GRAPHICS
void draw_logical_connections(ezgl::renderer* g) {
    const t_selected_sub_block_info& sel_subblk_info = get_selected_sub_block_info();
    t_draw_state* draw_state = get_draw_state_vars();

    auto& atom_ctx = g_vpr_ctx.atom();

    g->set_line_dash(ezgl::line_dash::none);

    // iterate over all the atom nets
    for (auto net_id : atom_ctx.nlist.nets()) {
        AtomPinId driver_pin_id = atom_ctx.nlist.net_driver(net_id);
        AtomBlockId src_blk_id = atom_ctx.nlist.pin_block(driver_pin_id);
        const t_pb_graph_node* src_pb_gnode = atom_ctx.lookup.atom_pb_graph_node(src_blk_id);
        ClusterBlockId src_clb = atom_ctx.lookup.atom_clb(src_blk_id);
        bool src_is_selected = sel_subblk_info.is_in_selected_subtree(src_pb_gnode, src_clb);
        bool src_is_src_of_selected = sel_subblk_info.is_source_of_selected(src_pb_gnode, src_clb);

        // iterate over the sinks
        for (auto sink_pin_id : atom_ctx.nlist.net_sinks(net_id)) {
            AtomBlockId sink_blk_id = atom_ctx.nlist.pin_block(sink_pin_id);
            const t_pb_graph_node* sink_pb_gnode = atom_ctx.lookup.atom_pb_graph_node(sink_blk_id);
            ClusterBlockId sink_clb = atom_ctx.lookup.atom_clb(sink_blk_id);

            if (src_is_selected && sel_subblk_info.is_sink_of_selected(sink_pb_gnode, sink_clb)) {
                g->set_color(DRIVES_IT_COLOR);
            } else if (src_is_src_of_selected && sel_subblk_info.is_in_selected_subtree(sink_pb_gnode, sink_clb)) {
                g->set_color(DRIVEN_BY_IT_COLOR);
            } else if (draw_state->show_nets == DRAW_LOGICAL_CONNECTIONS && (draw_state->showing_sub_blocks() || src_clb != sink_clb)) {
                g->set_color(ezgl::BLACK); // if showing all, draw the other ones in black
            } else {
                continue; // not showing all, and not the sperified block, so skip
            }

            draw_one_logical_connection(driver_pin_id, sink_pin_id, g);
        }
    }
}
#    endif /* NO_GRAPHICS */

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

    AtomPortId port_id = atom_ctx.nlist.pin_port(pin_id);
    const t_model_ports* model_port = atom_ctx.nlist.port_model(port_id);

    //Total up the port widths
    //  Note that we do this on the model since the atom netlist doesn't include unused ports
    int pin_cnt = 0;
    *pin_index = -1; //initialize
    const t_model* model = atom_ctx.nlist.block_model(blk_id);
    for (const t_model_ports* port : {model->inputs, model->outputs}) {
        while (port) {
            if (port == model_port) {
                //This is the port the pin is associated with, record it's index

                //Get the pin index in the port
                int atom_port_index = atom_ctx.nlist.pin_port_bit(pin_id);

                //The index of this pin in the model is the pins counted so-far
                //(i.e. accross previous ports) plus the index in the port
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

#    ifndef NO_GRAPHICS
/**
 * Draws ONE logical connection from src_pin in src_lblk to sink_pin in sink_lblk.
 * The *_abs_bbox parameters are for mild optmization, as the absolute bbox can be calculated
 * more effeciently elsewhere.
 */
void draw_one_logical_connection(const AtomPinId src_pin, const AtomPinId sink_pin, ezgl::renderer* g) {
    ezgl::point2d src_point = atom_pin_draw_coord(src_pin);
    ezgl::point2d sink_point = atom_pin_draw_coord(sink_pin);

    // draw a link connecting the pins.
    g->draw_line(src_point, sink_point);

    auto& atom_ctx = g_vpr_ctx.atom();
    if (atom_ctx.lookup.atom_clb(atom_ctx.nlist.pin_block(src_pin)) == atom_ctx.lookup.atom_clb(atom_ctx.nlist.pin_block(sink_pin))) {
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
#    endif /* NO_GRAPHICS */

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
        || pb_type->num_modes == 0) {
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

        for (auto blk_id : atom_ctx.nlist.blocks()) {
            const ClusterBlockId clb = atom_ctx.lookup.atom_clb(blk_id);
            const t_pb_graph_node* pb_graph_node = atom_ctx.lookup.atom_pb_graph_node(blk_id);
            // find the atom block that corrisponds to this pb.
            if (is_in_selected_subtree(pb_graph_node, clb)) {
                //Collect the sources of all nets driving this node
                for (auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
                    AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);
                    AtomPinId driver_pin_id = atom_ctx.nlist.net_driver(net_id);

                    AtomBlockId src_blk = atom_ctx.nlist.pin_block(driver_pin_id);

                    const ClusterBlockId src_clb = atom_ctx.lookup.atom_clb(src_blk);
                    const t_pb_graph_node* src_pb_graph_node = atom_ctx.lookup.atom_pb_graph_node(src_blk);

                    sources.insert(gnode_clb_pair(src_pb_graph_node, src_clb));
                }

                //Collect the sinks of all nets driven by this node
                for (auto pin_id : atom_ctx.nlist.block_output_pins(blk_id)) {
                    AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);
                    for (auto sink_pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                        AtomBlockId sink_blk = atom_ctx.nlist.pin_block(sink_pin_id);

                        const ClusterBlockId sink_clb = atom_ctx.lookup.atom_clb(sink_blk);
                        const t_pb_graph_node* sink_pb_graph_node = atom_ctx.lookup.atom_pb_graph_node(sink_blk);

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
    clb_index = atom_ctx.lookup.atom_clb(atom_ctx.nlist.pin_block(atom_pin));
    pb_gnode = atom_ctx.lookup.atom_pb_graph_node(atom_ctx.nlist.pin_block(atom_pin));
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

#endif // NO_GRAPHICS
