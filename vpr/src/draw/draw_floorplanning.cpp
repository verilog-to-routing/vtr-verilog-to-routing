#include <vector>

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "draw_floorplanning.h"
#include "user_place_constraints.h"
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

static void draw_internal_pb(const ClusterBlockId clb_index, t_pb* current_pb,
                             const t_pb* pb_to_draw, const ezgl::rectangle& parent_bbox,
                             const t_logical_block_type_ptr type, ezgl::color color,
                             ezgl::renderer* g);

const std::vector<ezgl::color> kelly_max_contrast_colors_no_black = {
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

#    define DEFAULT_HIGHLIGHT_ALPHA 30
#    define CLICKED_HIGHLIGHT_ALPHA 100

//Keeps track of how translucent each partition should be drawn on screen.
static std::vector<int> highlight_alpha;

//Helper function to highlight a partition
static void highlight_partition(ezgl::renderer* g, int partitionID, int alpha) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto constraints = floorplanning_ctx.constraints;
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    const auto& partition = constraints.get_partition((PartitionId)partitionID);
    const auto& partition_region = partition.get_part_region();
    const auto& regions = partition_region.get_regions();

    bool name_drawn = false;
    ezgl::color partition_color = kelly_max_contrast_colors_no_black[partitionID % (kelly_max_contrast_colors_no_black.size())];

    // The units of space in the constraints xml file will be referred to as "tile units"
    // The units of space that'll be used by ezgl to draw will be referred to as "on screen units"

    // Find the coordinates of the region by retrieving from the xml file
    // which tiles are at the corner of the region, then translate that to on
    // the on screen units for ezgl to use.

    for (int region = 0; (size_t)region < regions.size(); region++) {
        const vtr::Rect<int>& reg_coord = regions[region].get_rect();
        const auto [layer_low, layer_high] = regions[region].get_layer_range();

        for (int layer = layer_low; layer <= layer_high; layer++) {
            if (!draw_state->draw_layer_display[layer].visible) {
                continue;
            }

            int alpha_layer_part = alpha * draw_state->draw_layer_display[layer].alpha / 255;
            g->set_color(partition_color, alpha_layer_part);

            ezgl::rectangle top_right = draw_coords->get_absolute_clb_bbox(layer, reg_coord.xmax(), reg_coord.ymax(), 0);
            ezgl::rectangle bottom_left = draw_coords->get_absolute_clb_bbox(layer, reg_coord.xmin(), reg_coord.ymin(), 0);

            ezgl::rectangle on_screen_rect(bottom_left.bottom_left(), top_right.top_right());

            if (!name_drawn) {
                g->set_font_size(10);
                const std::string& partition_name = partition.get_name();

                g->set_color(partition_color, 230);

                g->draw_text(
                    on_screen_rect.center(),
                    partition_name,
                    on_screen_rect.width() - 10,
                    on_screen_rect.height() - 10);

                name_drawn = true;

                g->set_color(partition_color, alpha);
            }

            g->fill_rectangle(on_screen_rect);
        }
    }
}

//Iterates through all partitions and draws each region of each partition
void highlight_all_regions(ezgl::renderer* g) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    const auto& constraints = floorplanning_ctx.constraints;
    auto num_partitions = constraints.get_num_partitions();

    //keeps track of what alpha level each partition is
    if (highlight_alpha.empty()) {
        highlight_alpha.resize(num_partitions);
        std::fill(highlight_alpha.begin(), highlight_alpha.end(),
                  DEFAULT_HIGHLIGHT_ALPHA);
    }

    //draws the partitions
    for (int partitionID = 0; partitionID < num_partitions; partitionID++) {
        highlight_partition(g, partitionID, highlight_alpha[partitionID]);
    }
}

// Draws atoms that are constrained to a partition in the colour of their respective partition.
void draw_constrained_atoms(ezgl::renderer* g) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    const auto& constraints = floorplanning_ctx.constraints;
    int num_partitions = constraints.get_num_partitions();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (int partitionID = 0; partitionID < num_partitions; partitionID++) {
        auto atoms = constraints.get_part_atoms((PartitionId)partitionID);

        for (const AtomBlockId atom_id : atoms) {
            if (atom_ctx.lookup.atom_pb(atom_id) != nullptr) {
                const t_pb* pb = atom_ctx.lookup.atom_pb(atom_id);
                auto color = kelly_max_contrast_colors_no_black[partitionID % (kelly_max_contrast_colors_no_black.size())];
                ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_id);
                auto type = cluster_ctx.clb_nlist.block_type(clb_index);

                draw_internal_pb(clb_index, cluster_ctx.clb_nlist.block_pb(clb_index), pb, ezgl::rectangle({0, 0}, 0, 0), type, color, g);
            }
        }
    }
}

//Recursive function to find where the constrained atom is and draws it
static void draw_internal_pb(const ClusterBlockId clb_index,
                             t_pb* current_pb,
                             const t_pb* pb_to_draw,
                             const ezgl::rectangle& parent_bbox,
                             const t_logical_block_type_ptr type,
                             ezgl::color color, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();

    t_pb_type* pb_type = current_pb->pb_graph_node->pb_type;
    ezgl::rectangle temp = draw_coords->get_pb_bbox(clb_index, *current_pb->pb_graph_node);
    ezgl::rectangle abs_bbox = temp + parent_bbox.bottom_left();

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

    else {
        g->set_color(color);

        g->fill_rectangle(abs_bbox);
        if (draw_state->draw_block_outlines) {
            g->draw_rectangle(abs_bbox);
        }

        g->set_color(ezgl::BLACK);
        if (current_pb->name != nullptr) {
            g->set_font_size(10);

            std::string pb_type_name(pb_type->name);
            std::string pb_name(current_pb->name);

            std::string blk_tag = pb_name + " (" + pb_type_name + ")";

            g->draw_text(
                abs_bbox.center(),
                blk_tag,
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

enum {
    COL_NAME = 0,
    NUM_COLS
};

//Highlights partition clicked on in the legend.
void highlight_selected_partition(GtkWidget* widget) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto constraints = floorplanning_ctx.constraints;
    auto num_partitions = constraints.get_num_partitions();

    ezgl::renderer* g = application.get_renderer();
    GtkTreeIter iter;
    GtkTreeModel* model;
    gchar* row_value;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
        gtk_tree_model_get(model, &iter, COL_NAME, &row_value, -1);

        std::string row_value_str(row_value);

        //variable that represents the end of the partition's name in the row value
        auto name_end = row_value_str.find('(');
        if (name_end != std::string::npos) {
            //Isolates the part of the row value that is the partition name
            std::string partition_name = row_value_str.substr(0, name_end - 1);

            for (auto partitionID = 0; partitionID < num_partitions; partitionID++) {
                if (constraints.get_partition((PartitionId)partitionID).get_name() == partition_name) {
                    if (highlight_alpha.empty())
                        return;

                    if (highlight_alpha[partitionID] == CLICKED_HIGHLIGHT_ALPHA) {
                        highlight_alpha[partitionID] = DEFAULT_HIGHLIGHT_ALPHA;
                    } else {
                        highlight_alpha[partitionID] = CLICKED_HIGHLIGHT_ALPHA;
                    }

                    highlight_partition(g, partitionID, highlight_alpha[partitionID]);
                    break;
                }
            }
        }

        g_free(row_value);
        gtk_tree_selection_unselect_all(GTK_TREE_SELECTION(widget));
    }
    application.refresh_drawing();
}

//Fills in the legend
static GtkTreeModel* create_and_fill_model() {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    const auto& constraints = floorplanning_ctx.constraints;
    int num_partitions = constraints.get_num_partitions();

    GtkTreeStore* store = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING);

    for (int partitionID = 0; partitionID < num_partitions; partitionID++) {
        auto atoms = constraints.get_part_atoms((PartitionId)partitionID);
        const auto& partition = constraints.get_partition((PartitionId)partitionID);

        std::string partition_name(partition.get_name()
                                   + " (" + std::to_string(atoms.size()) + " primitives)");

        GtkTreeIter iter, child_iter;
        gtk_tree_store_append(store, &iter, nullptr);
        gtk_tree_store_set(store, &iter,
                           COL_NAME, partition_name.c_str(),
                           -1);

        for (AtomBlockId const_atom : atoms) {
            std::string atom_name = (atom_ctx.lookup.atom_pb(const_atom))->name;
            gtk_tree_store_append(store, &child_iter, &iter);
            gtk_tree_store_set(store, &child_iter,
                               COL_NAME, atom_name.c_str(),
                               -1);
        }
    }

    return GTK_TREE_MODEL(store);
}

GtkWidget* setup_floorplanning_legend(GtkWidget* content_tree) {
    GtkCellRenderer* renderer;

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(content_tree),
                                                -1,
                                                "Partition",
                                                renderer,
                                                "text", COL_NAME,
                                                NULL);

    GtkTreeModel* model = create_and_fill_model();

    gtk_tree_view_set_model(GTK_TREE_VIEW(content_tree), model);

    g_object_unref(model);

    return content_tree;
}

#endif
