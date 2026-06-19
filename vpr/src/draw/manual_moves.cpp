/*
 * @file 	manual_moves.cpp
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the function definitions needed for manual moves feature.
 *
 * Includes the graphics/gtk function for manual moves. The Manual Move Generator class is defined  manual_move_generator.h/cpp.
 * The manual move feature allows the user to select a move by choosing the block to move, x position, y position, layer_position, subtile position.
 * If the placer accepts the move, the user can accept or reject the move with respect to the delta cost,
 * delta timing and delta bounding box cost displayed on the UI. The manual move feature interacts with placement through
 * the ManualMoveGenerator class in the manual_move_generator.cpp/h files and in the place.cpp file by checking
 * if the manual move toggle button in the UI is active or not, and calls the function needed.
 */

#ifndef NO_GRAPHICS

#include "manual_moves.h"
#include "draw_debug.h"
#include "globals.h"
#include "draw.h"
#include "draw_global.h"
#include "draw_searchbar.h"
#include "buttons.h"
#include "physical_types_util.h"

#include <ezgl/qt/qtutils.hpp>

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>

void draw_manual_moves_window(const std::string& block_id) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (!draw_state->manual_moves_state.manual_move_window_is_open) {
        draw_state->manual_moves_state.manual_move_window_is_open = true;

        //Window settings-
        draw_state->manual_moves_state.manual_move_window = new QWidget;
        draw_state->manual_moves_state.manual_move_window->setAttribute(Qt::WA_DeleteOnClose);
        ezgl::center_window(draw_state->manual_moves_state.manual_move_window);
        draw_state->manual_moves_state.manual_move_window->setWindowTitle("Manual Moves Generator");
        draw_state->manual_moves_state.manual_move_window->setObjectName("manual_move_window");

        QWidget* grid = ezgl::grid_new();
        QLineEdit* block_entry = new QLineEdit;

        if (draw_state->manual_moves_state.user_highlighted_block) {
            block_entry->setText(block_id.c_str());
            draw_state->manual_moves_state.user_highlighted_block = false;
        }

        QLineEdit* x_position_entry = new QLineEdit;
        QLineEdit* y_position_entry = new QLineEdit;
        QLineEdit* layer_position_entry = new QLineEdit;
        QLineEdit* subtile_position_entry = new QLineEdit;
        QLabel* block_label = new QLabel("Block ID/Block Name:");
        QLabel* to_label = new QLabel("To Location:");
        QLabel* x = new QLabel("x:");
        QLabel* y = new QLabel("y:");
        QLabel* layer = new QLabel("layer:");
        QLabel* subtile = new QLabel("Subtile:");

        QPushButton* calculate_cost_button = new QPushButton("Calculate Costs");

        //Add all to grid
        ezgl::grid_attach(grid, block_label, 0, 0, 1, 1);
        ezgl::grid_attach(grid, block_entry, 0, 1, 1, 1);
        ezgl::grid_attach(grid, to_label, 2, 0, 1, 1);
        ezgl::grid_attach(grid, x, 1, 1, 1, 1);
        ezgl::grid_attach(grid, x_position_entry, 2, 1, 1, 1);
        ezgl::grid_attach(grid, y, 1, 2, 1, 1);
        ezgl::grid_attach(grid, y_position_entry, 2, 2, 1, 1);
        ezgl::grid_attach(grid, layer, 1, 3, 1, 1);
        ezgl::grid_attach(grid, layer_position_entry, 2, 3, 1, 1);
        ezgl::grid_attach(grid, subtile, 1, 4, 1, 1);
        ezgl::grid_attach(grid, subtile_position_entry, 2, 4, 1, 1);
        ezgl::grid_attach(grid, calculate_cost_button, 0, 5, 3, 1); //spans three columns

        //Set margins
        ezgl::widget_set_margin_bottom(grid, 20);
        ezgl::widget_set_margin_top(grid, 20);
        ezgl::widget_set_margin_start(grid, 20);
        ezgl::widget_set_margin_end(grid, 20);
        ezgl::widget_set_margin_bottom(block_label, 5);
        ezgl::widget_set_margin_bottom(to_label, 5);
        ezgl::widget_set_margin_top(calculate_cost_button, 15);
        ezgl::widget_set_margin_start(x, 13);
        ezgl::widget_set_margin_start(y, 13);
        ezgl::widget_set_halign(calculate_cost_button, Qt::AlignHCenter);

        //connect signals
        QObject::connect(calculate_cost_button, &QPushButton::clicked,
                         [grid]() { calculate_cost_callback(nullptr, grid); });
        QObject::connect(draw_state->manual_moves_state.manual_move_window, &QObject::destroyed,
                         []() { close_manual_moves_window(); });

        draw_state->manual_moves_state.manual_move_window->layout()->addWidget(grid);
        draw_state->manual_moves_state.manual_move_window->show();
    }
}

void calculate_cost_callback(QWidget* /*widget*/, QWidget* grid) {
    int block_id = -1;
    bool valid_input = true;

    t_draw_state* draw_state = get_draw_state_vars();

    //Loading the context/data structures needed.
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    //Getting entry values
    QLineEdit* block_entry = ezgl::grid_get_child_at<QLineEdit>(grid, 0, 1);
    std::string block_id_string = block_entry->text().toStdString();

    if (string_is_a_number(block_id_string)) { //for block ID
        block_id = std::atoi(block_id_string.c_str());
    } else { //for block name
        block_id = size_t(cluster_ctx.clb_nlist.find_block(block_id_string));
    }

    QLineEdit* x_position_entry = ezgl::grid_get_child_at<QLineEdit>(grid, 2, 1);
    QLineEdit* y_position_entry = ezgl::grid_get_child_at<QLineEdit>(grid, 2, 2);
    QLineEdit* layer_position_entry = ezgl::grid_get_child_at<QLineEdit>(grid, 2, 3);
    QLineEdit* subtile_position_entry = ezgl::grid_get_child_at<QLineEdit>(grid, 2, 4);

    int x_location = x_position_entry->text().toInt();
    int y_location = y_position_entry->text().toInt();
    int layer_location = layer_position_entry->text().toInt();
    int subtile_location = subtile_position_entry->text().toInt();

    if (block_entry->text().isEmpty()
        || x_position_entry->text().isEmpty()
        || y_position_entry->text().isEmpty()
        || layer_position_entry->text().isEmpty()
        || subtile_position_entry->text().isEmpty()) {
        invalid_breakpoint_entry_window("Not all fields are complete");
        valid_input = false;
    }

    t_pl_loc to = t_pl_loc(x_location, y_location, subtile_location, layer_location);
    valid_input = is_manual_move_legal(ClusterBlockId(block_id), to);

    if (valid_input) {
        draw_state->manual_moves_state.manual_move_info.valid_input = true;
        draw_state->manual_moves_state.manual_move_info.blockID = block_id;
        draw_state->manual_moves_state.manual_move_info.x_pos = x_location;
        draw_state->manual_moves_state.manual_move_info.y_pos = y_location;
        draw_state->manual_moves_state.manual_move_info.layer = layer_location;
        draw_state->manual_moves_state.manual_move_info.subtile = subtile_location;
        draw_state->manual_moves_state.manual_move_info.to_location = to;

        //Highlighting the block
        deselect_all();
        ClusterBlockId clb_index = ClusterBlockId(draw_state->manual_moves_state.manual_move_info.blockID);
        draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
        application->refresh_drawing();

        //Continues to move costs window.
        QWidget* proceed = find_button("ProceedButton");
        ezgl::press_proceed(proceed, &application);

    } else {
        draw_state->manual_moves_state.manual_move_info.valid_input = false;
    }
}

bool is_manual_move_legal(ClusterBlockId block_id, t_pl_loc to) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& grid_blocks = draw_state->get_graphics_blk_loc_registry_ref().grid_blocks();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

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
    auto physical_tile = device_ctx.grid.get_physical_type({to.x, to.y, to.layer});
    auto logical_block = cluster_ctx.clb_nlist.block_type(block_id);
    if (to.sub_tile < 0 || to.sub_tile >= physical_tile->capacity || !is_sub_tile_compatible(physical_tile, logical_block, to.sub_tile)) {
        invalid_breakpoint_entry_window("Blocks are not compatible");
        return false;
    }

    //If the destination block is user constrained, abort this swap
    ClusterBlockId b_to = grid_blocks.block_at_location(to);
    if (b_to) {
        if (block_locs[b_to].is_fixed) {
            invalid_breakpoint_entry_window("Block is fixed");
            return false;
        }
    }

    //If the block requested is already in that location.
    t_pl_loc current_block_loc = block_locs[block_id].loc;
    if (to.x == current_block_loc.x && to.y == current_block_loc.y && to.sub_tile == current_block_loc.sub_tile) {
        invalid_breakpoint_entry_window("The block is currently in this location");
        return false;
    }

    return true;
}

bool manual_move_is_selected() {
    t_draw_state* draw_state = get_draw_state_vars();
    QCheckBox* manual_moves = application->find_check_box("manualMove");
    draw_state->manual_moves_state.manual_move_enabled = manual_moves->isChecked();
    return draw_state->manual_moves_state.manual_move_enabled;
}

void manual_move_cost_summary_dialog() {
    t_draw_state* draw_state = get_draw_state_vars();

    QDialog* dialog = new QDialog(draw_state->manual_moves_state.manual_move_window);
    dialog->setWindowTitle("Move Costs");
    dialog->setWindowFlag(Qt::Dialog);
    dialog->setWindowModality(Qt::WindowModal);

    auto* layout = new QVBoxLayout(dialog);

    //Create elements for the dialog and printing costs to the user.
    QLabel* title_label = new QLabel;
    title_label->setTextFormat(Qt::RichText);
    title_label->setText("<b>Move Costs and Outcomes</b>");
    std::string delta_cost = "Delta Cost: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_cost) + "   ";
    QLabel* delta_cost_label = new QLabel(delta_cost.c_str());
    std::string delta_timing = "   Delta Timing: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_timing) + "   ";
    QLabel* delta_timing_label = new QLabel(delta_timing.c_str());
    std::string delta_bounding_box = "  Delta Bounding Box Cost: " + std::to_string(draw_state->manual_moves_state.manual_move_info.delta_bounding_box) + "   ";
    QLabel* delta_bounding_box_label = new QLabel(delta_bounding_box.c_str());
    std::string outcome = e_move_result_to_string(draw_state->manual_moves_state.manual_move_info.placer_move_outcome);
    std::string move_outcome = "  Annealing Decision: " + outcome + "   ";
    QLabel* move_outcome_label = new QLabel(move_outcome.c_str());
    QLabel* space_label1 = new QLabel("    ");
    QLabel* space_label2 = new QLabel("    ");

    layout->addWidget(title_label);
    layout->addWidget(space_label1);
    layout->addWidget(delta_cost_label);
    layout->addWidget(delta_timing_label);
    layout->addWidget(delta_bounding_box_label);
    layout->addWidget(move_outcome_label);
    layout->addWidget(space_label2);

    auto* buttonBox = new QDialogButtonBox(dialog);
    buttonBox->addButton("Accept", QDialogButtonBox::AcceptRole);
    buttonBox->addButton("Reject", QDialogButtonBox::RejectRole);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    //Show the dialog with all the labels.
    dialog->show();

    //Update message if user accepts the move.
    std::string msg = "Manual move accepted. Block #" + std::to_string(draw_state->manual_moves_state.manual_move_info.blockID);
    msg += " to location (" + std::to_string(draw_state->manual_moves_state.manual_move_info.x_pos) + ", " + std::to_string(draw_state->manual_moves_state.manual_move_info.y_pos) + ")";

    //Waiting for the user to respond to return to try_swa function.
    int result = dialog->exec();
    switch (result) {
        //If the user accepts the manual move
        case QDialog::Accepted:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = e_move_result::ACCEPTED;
            application->update_message(msg);
            break;
        //If the user rejects the manual move
        case QDialog::Rejected:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = e_move_result::REJECTED;
            application->update_message("Manual move was rejected");
            break;
        default:
            draw_state->manual_moves_state.manual_move_info.user_move_outcome = e_move_result::ABORTED;
            break;
    }

    //Destroys the move outcome dialog.
    dialog->deleteLater();
}

void manual_move_highlight_new_block_location() {
    t_draw_state* draw_state = get_draw_state_vars();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    //Unselects all blocks first
    deselect_all();
    //Highlighting the block
    ClusterBlockId clb_index = ClusterBlockId(draw_state->manual_moves_state.manual_move_info.blockID);
    draw_highlight_blocks_color(cluster_ctx.clb_nlist.block_type(clb_index), clb_index);
    application->refresh_drawing();
}

//Manual move window turns false, the window is destroyed.
void close_manual_moves_window() {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->manual_moves_state.manual_move_window_is_open = false;
}

bool string_is_a_number(const std::string& block_id) {
    return std::all_of(block_id.begin(), block_id.end(), isdigit);
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

e_create_move manual_move_display_and_propose(ManualMoveGenerator& manual_move_generator,
                                              t_pl_blocks_to_be_moved& blocks_affected,
                                              e_move_type& move_type,
                                              float rlim,
                                              const t_placer_opts& placer_opts,
                                              const PlacerCriticalities* criticalities) {
    draw_manual_moves_window("");
    update_screen(ScreenUpdatePriority::MAJOR, " ", e_pic_type::PLACEMENT, nullptr);
    move_type = e_move_type::MANUAL_MOVE;
    t_propose_action proposed_action{move_type, -1}; //no need to specify block type in manual move "propose_move" function
    return manual_move_generator.propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
}

#endif /*NO_GRAPHICS*/
