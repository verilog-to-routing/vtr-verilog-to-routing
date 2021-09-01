/*
 * @file 	manual_moves.cpp
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the function definitions needed for manual moves feature.
 *
 * Includes the graphics/gtk function for manual moves. The Manual Move Generator class is defined  manual_move_generator.h/cpp.
 * The manual move feature allows the user to select a move by choosing the block to move, x position, y position, subtile position. If the placer accepts the move, the user can accept or reject the move with respect to the delta cost, delta timing and delta bounding box cost displayed on the UI. The manual move feature interacts with placement through the ManualMoveGenerator class in the manual_move_generator.cpp/h files and in the place.cpp file by checking if the manual move toggle button in the UI is active or not, and calls the function needed. 
 */

#include "manual_moves.h"
#include "globals.h"
#include "draw.h"
#include "buttons.h"

#ifndef NO_GRAPHICS

void draw_manual_moves_window(std::string block_id) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (!draw_state->manual_moves_state.manual_move_window_is_open) {
        draw_state->manual_moves_state.manual_move_window_is_open = true;

        //Window settings-
        draw_state->manual_moves_state.manual_move_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_position((GtkWindow*)draw_state->manual_moves_state.manual_move_window, GTK_WIN_POS_CENTER);
        gtk_window_set_title((GtkWindow*)draw_state->manual_moves_state.manual_move_window, "Manual Moves Generator");
        gtk_widget_set_name(draw_state->manual_moves_state.manual_move_window, "manual_move_window");

        GtkWidget* grid = gtk_grid_new();
        GtkWidget* block_entry = gtk_entry_new();

        if (draw_state->manual_moves_state.user_highlighted_block) {
            gtk_entry_set_text((GtkEntry*)block_entry, block_id.c_str());
            draw_state->manual_moves_state.user_highlighted_block = false;
        }

        GtkWidget* x_position_entry = gtk_entry_new();
        GtkWidget* y_position_entry = gtk_entry_new();
        GtkWidget* subtile_position_entry = gtk_entry_new();
        GtkWidget* block_label = gtk_label_new("Block ID/Block Name:");
        GtkWidget* to_label = gtk_label_new("To Location:");
        GtkWidget* x = gtk_label_new("x:");
        GtkWidget* y = gtk_label_new("y:");
        GtkWidget* subtile = gtk_label_new("Subtile:");

        GtkWidget* calculate_cost_button = gtk_button_new_with_label("Calculate Costs");

        //Add all to grid
        gtk_grid_attach((GtkGrid*)grid, block_label, 0, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, block_entry, 0, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, to_label, 2, 0, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, x, 1, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, x_position_entry, 2, 1, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, y, 1, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, y_position_entry, 2, 2, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, subtile, 1, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, subtile_position_entry, 2, 3, 1, 1);
        gtk_grid_attach((GtkGrid*)grid, calculate_cost_button, 0, 4, 3, 1); //spans three columns

        //Set margins
        gtk_widget_set_margin_bottom(grid, 20);
        gtk_widget_set_margin_top(grid, 20);
        gtk_widget_set_margin_start(grid, 20);
        gtk_widget_set_margin_end(grid, 20);
        gtk_widget_set_margin_bottom(block_label, 5);
        gtk_widget_set_margin_bottom(to_label, 5);
        gtk_widget_set_margin_top(calculate_cost_button, 15);
        gtk_widget_set_margin_start(x, 13);
        gtk_widget_set_margin_start(y, 13);
        gtk_widget_set_halign(calculate_cost_button, GTK_ALIGN_CENTER);

        //connect signals
        g_signal_connect(calculate_cost_button, "clicked", G_CALLBACK(calculate_cost_callback), grid);
        g_signal_connect(G_OBJECT(draw_state->manual_moves_state.manual_move_window), "destroy", G_CALLBACK(close_manual_moves_window), NULL);

        gtk_container_add(GTK_CONTAINER(draw_state->manual_moves_state.manual_move_window), grid);
        gtk_widget_show_all(draw_state->manual_moves_state.manual_move_window);
    }
}

void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid) {
    int block_id = -1;
    int x_location = -1;
    int y_location = -1;
    int subtile_location = -1;
    bool valid_input = true;

    t_draw_state* draw_state = get_draw_state_vars();

    //Loading the context/data structures needed.
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Getting entry values
    GtkWidget* block_entry = gtk_grid_get_child_at((GtkGrid*)grid, 0, 1);
    std::string block_id_string = gtk_entry_get_text((GtkEntry*)block_entry);

    if (string_is_a_number(block_id_string)) { //for block ID
        block_id = std::atoi(block_id_string.c_str());
    } else { //for block name
        block_id = size_t(cluster_ctx.clb_nlist.find_block(gtk_entry_get_text((GtkEntry*)block_entry)));
    }

    GtkWidget* x_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 1);
    GtkWidget* y_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 2);
    GtkWidget* subtile_position_entry = gtk_grid_get_child_at((GtkGrid*)grid, 2, 3);

    x_location = std::atoi(gtk_entry_get_text((GtkEntry*)x_position_entry));
    y_location = std::atoi(gtk_entry_get_text((GtkEntry*)y_position_entry));
    subtile_location = std::atoi(gtk_entry_get_text((GtkEntry*)subtile_position_entry));

    if (std::string(gtk_entry_get_text((GtkEntry*)block_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)x_position_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)y_position_entry)).empty() || std::string(gtk_entry_get_text((GtkEntry*)subtile_position_entry)).empty()) {
        invalid_breakpoint_entry_window("Not all fields are complete");
        valid_input = false;
    }

    t_pl_loc to = t_pl_loc(x_location, y_location, subtile_location);
    valid_input = is_manual_move_legal(ClusterBlockId(block_id), to);

    if (valid_input) {
        draw_state->manual_moves_state.manual_move_info.valid_input = true;
        draw_state->manual_moves_state.manual_move_info.blockID = block_id;
        draw_state->manual_moves_state.manual_move_info.x_pos = x_location;
        draw_state->manual_moves_state.manual_move_info.y_pos = y_location;
        draw_state->manual_moves_state.manual_move_info.subtile = subtile_location;
        draw_state->manual_moves_state.manual_move_info.to_location = to;

        //Highlighting the block
        deselect_all();
        ClusterBlockId clb_index = ClusterBlockId(draw_state->manual_moves_state.manual_move_info.blockID);
        draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
        application.refresh_drawing();

        //Continues to move costs window.
        GtkWidget* proceed = find_button("ProceedButton");
        ezgl::press_proceed(proceed, &application);

    } else {
        draw_state->manual_moves_state.manual_move_info.valid_input = false;
    }
}

bool is_manual_move_legal(ClusterBlockId block_id, t_pl_loc to) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    //if the block is not found
    if ((!cluster_ctx.clb_nlist.valid_block_id(ClusterBlockId(block_id)))) {
        invalid_breakpoint_entry_window("Invalid block ID/Name");
        return false;
    }

    //If the dimensions are out of bounds
    if (to.x < 0 || to.x >= int(device_ctx.grid.width())
        || to.y < 0 || to.y >= int(device_ctx.grid.height())) {
        invalid_breakpoint_entry_window("Dimensions are out of bounds");
        return false;
    }

    //If the block s not compatible
    auto physical_tile = device_ctx.grid[to.x][to.y].type;
    auto logical_block = cluster_ctx.clb_nlist.block_type(block_id);
    if (to.sub_tile < 0 || to.sub_tile >= physical_tile->capacity || !is_sub_tile_compatible(physical_tile, logical_block, to.sub_tile)) {
        invalid_breakpoint_entry_window("Blocks are not compatible");
        return false;
    }

    //If the destination block is user constrained, abort this swap
    auto b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.sub_tile];
    if (b_to != INVALID_BLOCK_ID && b_to != EMPTY_BLOCK_ID) {
        if (place_ctx.block_locs[b_to].is_fixed) {
            invalid_breakpoint_entry_window("Block is fixed");
            return false;
        }
    }

    //If the block requested is already in that location.
    t_pl_loc current_block_loc = place_ctx.block_locs[block_id].loc;
    if (to.x == current_block_loc.x && to.y == current_block_loc.y && to.sub_tile == current_block_loc.sub_tile) {
        invalid_breakpoint_entry_window("The block is currently in this location");
        return false;
    }

    return true;
}

bool manual_move_is_selected() {
    t_draw_state* draw_state = get_draw_state_vars();
    GObject* manual_moves = application.get_object("manualMove");
    draw_state->manual_moves_state.manual_move_enabled = gtk_toggle_button_get_active((GtkToggleButton*)manual_moves);
    return draw_state->manual_moves_state.manual_move_enabled;
}

void manual_move_cost_summary_dialog() {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkWidget* dialog;
    GtkWidget* content_area;

    //Creating the dialog window
    dialog = gtk_dialog_new_with_buttons("Move Costs",
                                         (GtkWindow*)draw_state->manual_moves_state.manual_move_window,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         ("Accept"),
                                         GTK_RESPONSE_ACCEPT,
                                         ("Reject"),
                                         GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_window_set_transient_for((GtkWindow*)dialog, (GtkWindow*)draw_state->manual_moves_state.manual_move_window);

    //Create elements for the dialog and printing costs to the user.
    GtkWidget* title_label = gtk_label_new(NULL);
    gtk_label_set_markup((GtkLabel*)title_label, "<b>Move Costs and Outcomes</b>");
    std::string delta_cost = "Delta Cost: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_cost) + "   ";
    GtkWidget* delta_cost_label = gtk_label_new(delta_cost.c_str());
    std::string delta_timing = "   Delta Timing: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_timing) + "   ";
    GtkWidget* delta_timing_label = gtk_label_new(delta_timing.c_str());
    std::string delta_bounding_box = "  Delta Bounding Box Cost: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_bounding_box) + "   ";
    GtkWidget* delta_bounding_box_label = gtk_label_new(delta_bounding_box.c_str());
    std::string outcome = e_move_result_to_string(draw_state->manual_moves_state.manual_move_info.placer_move_outcome);
    std::string move_outcome = "  Annealing Decision: " + outcome + "   ";
    GtkWidget* move_outcome_label = gtk_label_new(move_outcome.c_str());
    GtkWidget* space_label1 = gtk_label_new("    ");
    GtkWidget* space_label2 = gtk_label_new("    ");

    //Attach elements to the content area of the dialog.
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), title_label);
    gtk_container_add(GTK_CONTAINER(content_area), space_label1);
    gtk_container_add(GTK_CONTAINER(content_area), delta_cost_label);
    gtk_container_add(GTK_CONTAINER(content_area), delta_timing_label);
    gtk_container_add(GTK_CONTAINER(content_area), delta_bounding_box_label);
    gtk_container_add(GTK_CONTAINER(content_area), move_outcome_label);
    gtk_container_add(GTK_CONTAINER(content_area), space_label2);

    //Show the dialog with all the labels.
    gtk_widget_show_all(dialog);

    //Update message if user accepts the move.
    std::string msg = "Manual move accepted. Block #" + std::to_string(draw_state->manual_moves_state.manual_move_info.blockID);
    msg += " to location (" + std::to_string(draw_state->manual_moves_state.manual_move_info.x_pos) + ", " + std::to_string(draw_state->manual_moves_state.manual_move_info.y_pos) + ")";

    //Waiting for the user to respond to return to try_swa function.
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    switch (result) {
        //If the user accepts the manual move
        case GTK_RESPONSE_ACCEPT:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = ACCEPTED;
            application.update_message(msg.c_str());
            break;
        //If the user rejects the manual move
        case GTK_RESPONSE_REJECT:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = REJECTED;
            application.update_message("Manual move was rejected");
            break;
        default:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = ABORTED;
            break;
    }

    //Destroys the move outcome dialog.
    gtk_widget_destroy(dialog);
}

void manual_move_highlight_new_block_location() {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    //Unselects all blocks first
    deselect_all();
    //Highlighting the block
    ClusterBlockId clb_index = ClusterBlockId(draw_state->manual_moves_state.manual_move_info.blockID);
    draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
    application.refresh_drawing();
}

//Manual move window turns false, the window is destroyed.
void close_manual_moves_window() {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->manual_moves_state.manual_move_window_is_open = false;
}

bool string_is_a_number(std::string block_id) {
    for (size_t i = 0; i < block_id.size(); i++) {
        //Returns 0 if the string does not have characters from 0-9
        if (isdigit(block_id[i]) == 0) {
            return false;
        }
    }
    return true;
}

//Updates ManualMovesInfo cost and placer move outcome variables. User_move_outcome is also updated.
e_move_result pl_do_manual_move(double d_cost, double d_timing, double d_bounding_box, e_move_result& move_outcome) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->manual_moves_state.manual_move_info.delta_cost = d_cost;
    draw_state->manual_moves_state.manual_move_info.delta_timing = d_timing;
    draw_state->manual_moves_state.manual_move_info.delta_bounding_box = d_bounding_box;
    draw_state->manual_moves_state.manual_move_info.placer_move_outcome = move_outcome;

    //Displays the delta cost to the user.
    manual_move_cost_summary_dialog();
    //User accepts or rejects the move.
    move_outcome = draw_state->manual_moves_state.manual_move_info.user_move_outcome;
    return move_outcome;
}

e_create_move manual_move_display_and_propose(ManualMoveGenerator& manual_move_generator, t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    draw_manual_moves_window("");
    update_screen(ScreenUpdatePriority::MAJOR, " ", PLACEMENT, nullptr);
    move_type = e_move_type::MANUAL_MOVE;
    return manual_move_generator.propose_move(blocks_affected, move_type, rlim, placer_opts, criticalities);
}

#endif /*NO_GRAPHICS*/
