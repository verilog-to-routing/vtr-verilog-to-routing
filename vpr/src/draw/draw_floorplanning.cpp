#include <vector>

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "draw_floorplanning.h"
#include "vpr_constraints.h"
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_basic.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "move_utils.h"
#include "route_export.h"
#include "tatum/report/TimingPathCollector.hpp"

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "move_utils.h"
#endif

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
              * track CPU runtime.														   */
#    include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
       * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
       * which means tracking CPU time will not be the same as the actual wall clock time. *
       * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#    include <sys/time.h>
#endif

#ifndef NO_GRAPHICS

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* current_pb, const t_pb* pb_to_draw, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::color color, ezgl::renderer* g);

const std::vector<ezgl::color> kelly_max_contrast_colors = {
    //ezgl::color(242, 243, 244), //white: skip white since it doesn't contrast well with VPR's light background
    //ezgl::color(34, 34, 34),    //black: hard to differentiate between outline & primitive.
    ezgl::color(243, 195, 0),   //yellow
    ezgl::color(135, 86, 146),  //purple
    ezgl::color(243, 132, 0),   //orange
    ezgl::color(161, 202, 241), //light blue
    ezgl::color(190, 0, 50),    //red
    ezgl::color(194, 178, 128), //buf
    ezgl::color(132, 132, 130), //gray
    ezgl::color(0, 136, 86),    //green
    ezgl::color(230, 143, 172), //purplish pink
    ezgl::color(0, 103, 165),   //blue
    ezgl::color(249, 147, 121), //yellowish pink
    ezgl::color(96, 78, 151),   //violet
    ezgl::color(246, 166, 0),   //orange yellow
    ezgl::color(179, 68, 108),  //purplish red
    ezgl::color(220, 211, 0),   //greenish yellow
    ezgl::color(136, 45, 23),   //redish brown
    ezgl::color(141, 182, 0),   //yellow green
    ezgl::color(101, 69, 34),   //yellowish brown
    ezgl::color(226, 88, 34),   //reddish orange
    ezgl::color(43, 61, 38)     //olive green
};

/* Highlights partitions. */
void highlight_regions(ezgl::renderer* g) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto constraints = floorplanning_ctx.constraints;
    auto num_partitions = constraints.get_num_partitions();
    t_draw_coords* draw_coords = get_draw_coords_vars();

    for (int partitionID = 0; partitionID < num_partitions; partitionID++) {
        auto partition = constraints.get_partition((PartitionId)partitionID);
        auto& partition_region = partition.get_part_region();
        auto regions = partition_region.get_partition_region();

        bool name_drawn = false;
        ezgl::color partition_color = kelly_max_contrast_colors[partitionID % (kelly_max_contrast_colors.size())];
        g->set_color(partition_color, 30);

        // The units of space in the constraints xml file will be refered to as "tile units"
        // The units of space that'll be used by ezgl to draw will be refered to as "on screen units"

        // Find the coordinates of the region by retrieving from the xml file
        // which tiles are at the corner of the region, then translate that to on
        // the on screen units for ezgl to use.

        for (int region = 0; (size_t)region < regions.size(); region++) {
            auto tile_rect = regions[region].get_region_rect();

            ezgl::rectangle top_right = draw_coords->get_absolute_clb_bbox(tile_rect.xmax(),
                                                                           tile_rect.ymax(), 0);
            ezgl::rectangle bottom_left = draw_coords->get_absolute_clb_bbox(tile_rect.xmin(),
                                                                             tile_rect.ymin(), 0);

            ezgl::rectangle on_screen_rect(bottom_left.bottom_left(), top_right.top_right());

            // Ensures name is drawn only once per partition
            if (!name_drawn) {
                g->set_font_size(10);
                std::string partition_name = partition.get_name();

                g->set_color(partition_color, 230);

                g->draw_text(
                    on_screen_rect.center(),
                    partition_name.c_str(),
                    on_screen_rect.width() - 10,
                    on_screen_rect.height() - 10);

                name_drawn = true;

                g->set_color(partition_color, 30);
            }

            g->fill_rectangle(on_screen_rect);
        }
    }
}

/* Function to draw all constrained primitives */
void draw_constrained_atoms(ezgl::renderer* g) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto constraints = floorplanning_ctx.constraints;
    auto num_partitions = constraints.get_num_partitions();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (int partitionID = 0; partitionID < num_partitions; partitionID++) {
        auto atoms = constraints.get_part_atoms((PartitionId)partitionID);

        //iterate through every primitive in the partition
        for (size_t j = 0; j < atoms.size(); j++) {
            AtomBlockId const& const_atom = atoms[j];
            if (atom_ctx.lookup.atom_pb(const_atom) != nullptr) {
                const t_pb* pb = atom_ctx.lookup.atom_pb(const_atom);
                auto color = kelly_max_contrast_colors[partitionID % (kelly_max_contrast_colors.size())];
                ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atoms[j]);
                auto type = cluster_ctx.clb_nlist.block_type(clb_index);

                draw_internal_pb(clb_index, cluster_ctx.clb_nlist.block_pb(clb_index), pb, ezgl::rectangle({0, 0}, 0, 0), type, color, g);
            }
        }
    }
}

/* Helper subroutine to draw a specific subblock (pb_to_draw). This function traverses through the pb_graph
 * which a netlist block can map to, and checks each sub-block inside its parent blocks
 * to see if it is the sub-block to be drawn.
 */
static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* current_pb, const t_pb* pb_to_draw, const ezgl::rectangle& parent_bbox, const t_logical_block_type_ptr type, ezgl::color color, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    t_pb_type* pb_type = current_pb->pb_graph_node->pb_type;
    ezgl::rectangle temp = draw_coords->get_pb_bbox(clb_index, *current_pb->pb_graph_node);
    ezgl::rectangle abs_bbox = temp + parent_bbox.bottom_left();

    // if the current pb is not the pb_to_draw,
    if (current_pb != pb_to_draw) {
        int num_child_types = current_pb->get_num_child_types();
        for (int i = 0; i < num_child_types; ++i) {
            if (current_pb->child_pbs[i] == nullptr) {
                continue;
            }

            int num_pb = current_pb->get_num_children_of_type(i);
            for (int j = 0; j < num_pb; ++j) {
                t_pb* child_pb = &current_pb->child_pbs[i][j];

                VTR_ASSERT(child_pb != nullptr);

                t_pb_type* pb_child_type = child_pb->pb_graph_node->pb_type;

                if (pb_child_type == nullptr) {
                    continue;
                }

                // now recurse
                draw_internal_pb(clb_index, child_pb, pb_to_draw, abs_bbox, type, color, g);
            }
        }
    }

    //if the current pb is the pb we want to draw
    else {
        //Draws the pb as a rectangle on the screen
        g->set_color(color);

        g->fill_rectangle(abs_bbox);
        if (draw_state->draw_block_outlines) {
            g->draw_rectangle(abs_bbox);
        }

        g->set_color(ezgl::BLACK);

        //Draws the name of the pb
        if (current_pb->name != nullptr) {
            g->set_font_size(10);

            std::string pb_type_name(pb_type->name);
            std::string pb_name(current_pb->name);

            std::string blk_tag = pb_name + " (" + pb_type_name + ")";

            g->draw_text(
                abs_bbox.center(),
                blk_tag.c_str(),
                abs_bbox.width() + 10,
                abs_bbox.height() + 10);

        } else {
            g->draw_text(
                abs_bbox.center(),
                pb_type->name,
                abs_bbox.width() + 10,
                abs_bbox.height() + 10);
        }
    }
}

#endif
