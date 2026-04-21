/**
 * @file UI_SETUP.CPP
 * @author Sebastian Lievano
 * @date July 4th, 2022
 * @brief Manages setup for main.ui created buttons
 *
 * This file contains the various setup functions for all of the ui functions.
 * As of June 2022, gtk ui items are to be created through Glade/main.ui file (see Docs)
 * Each function here initializes a different set of ui buttons, connecting their callback functions
 */

#ifndef NO_GRAPHICS

#include "clustered_netlist.h"
#include "draw.h"
#include "draw_global.h"
#include "draw_toggle_functions.h"
#include "save_graphics.h"
#include "search_bar.h"
#include "ui_setup.h"

#include "ezgl/application.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QBoxLayout>
#include <QStringList>
#include <QCompleter>

/**
 * @brief Helper function to connect a toggle button to a callback function
 */
static void setup_checkbox_button(std::string button_id, ezgl::application* app, bool* toggle_state) {
    t_draw_state* draw_state = get_draw_state_vars();
    QCheckBox* checkbox_button = app->find_check_box(button_id.c_str());
    draw_state->checkbox_data.emplace_back(app, toggle_state);
    t_checkbox_data* data = &draw_state->checkbox_data.back();
    QObject::connect(checkbox_button, &QCheckBox::toggled, checkbox_button, [checkbox_button, data]() {
        toggle_checkbox_cbk(checkbox_button, data);
    });
}

void basic_button_setup(ezgl::application* app) {
    //button to enter window_mode, created in main.ui
    QPushButton* window = app->find_push_button("Window");
    QObject::connect(window, &QPushButton::clicked, window, [app]() {
        toggle_window_mode(/*widget=*/nullptr, app);
    });

    //button to search, created in main.ui
    QPushButton* search = app->find_push_button("Search");
    search->setText("Search");
    QObject::connect(search, &QPushButton::clicked, search, [app]() {
        search_and_highlight(/*widget=*/nullptr, app);
    });

    //button for save graphics, created in main.ui
    QPushButton* save = app->find_push_button("SaveGraphics");
    QObject::connect(save, &QPushButton::clicked, save, []() {
        save_graphics_dialog_box(/*widget=*/nullptr, /*app=*/nullptr);
    });

    //combo box for search type, created in main.ui
    QComboBox* search_type_combo = app->find_combo_box("SearchType");
    QObject::connect(search_type_combo, &QComboBox::currentIndexChanged, search_type_combo, [search_type_combo, app]() {
        search_type_changed(search_type_combo, app);
    });
}

/*
 * @brief sets up net related buttons and connects their signals
 *
 * Sets up the toggle nets combo box, net alpha spin button, and max fanout
 * spin button which are created in main.ui file. Found in Net Settings dropdown
 * @param app ezgl::application ptr
 */
void net_button_setup(ezgl::application* app) {

    t_draw_state* draw_state = get_draw_state_vars();
    SwitchButton* toggle_nets_switch = app->find_switch_button("ToggleNets");
    QObject::connect(toggle_nets_switch, &SwitchButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
        toggle_show_nets_cbk(toggle_nets_switch, checked, app);
    });

    // Manages net type
    QComboBox* toggle_nets = app->find_combo_box("ToggleNetType");
    QObject::connect(toggle_nets, &QComboBox::currentIndexChanged, toggle_nets, [toggle_nets, app]() {
        toggle_draw_nets_cbk(toggle_nets, app);
    });

    setup_checkbox_button("ToggleInterClusterNets", app, &draw_state->draw_inter_cluster_nets);

    setup_checkbox_button("ToggleIntraClusterNets", app, &draw_state->draw_intra_cluster_nets);

    setup_checkbox_button("FanInFanOut", app, &draw_state->highlight_fan_in_fan_out);

    //Manages net alpha
    QSpinBox* net_alpha = app->find_spin_box("NetAlpha");
    QObject::connect(net_alpha, &QSpinBox::valueChanged, net_alpha, [net_alpha, app]() {
        set_net_alpha_value_cbk(net_alpha, app);
    });
    net_alpha->setSingleStep(1);
    net_alpha->setRange(0, 255);

    //Manages net max fanout
    QSpinBox* max_fanout = app->find_spin_box("NetMaxFanout");
    QObject::connect(max_fanout, &QSpinBox::valueChanged, max_fanout, [max_fanout, app]() {
        set_net_max_fanout_cbk(max_fanout, app);
    });
    max_fanout->setSingleStep(1);
    max_fanout->setRange(0, get_max_fanout());
}

/*
 * @brief sets up block related buttons, connects their signals
 *
 * Connects signals and sets init. values for blk internals spin button,
 * blk pin util combo box,placement macros combo box, and noc combo bx created in
 * main.ui. Found in Block Settings dropdown
 * @param app
 */
void block_button_setup(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    //Toggle block internals
    QSpinBox* blk_internals_button = app->find_spin_box("ToggleBlkInternals");
    QObject::connect(blk_internals_button, &QSpinBox::valueChanged, blk_internals_button, [blk_internals_button, app]() {
        toggle_blk_internal_cbk(blk_internals_button, app);
    });
    blk_internals_button->setSingleStep(1);
    blk_internals_button->setRange(0, draw_state->max_sub_blk_lvl);

    //Toggle Block Pin Util
    QComboBox* blk_pin_util = app->find_combo_box("ToggleBlkPinUtil");
    QObject::connect(blk_pin_util, &QComboBox::currentIndexChanged, blk_pin_util, [blk_pin_util, app]() {
        toggle_blk_pin_util_cbk(blk_pin_util, app);
    });

    //Toggle Placement Macros
    QComboBox* placement_macros = app->find_combo_box("TogglePlacementMacros");
    QObject::connect(placement_macros, &QComboBox::currentIndexChanged, placement_macros, [placement_macros, app]() {
        placement_macros_cbk(placement_macros, app);
    });

    //Toggle NoC Display (based on startup cmd --noc on)
    if (!draw_state->show_noc_button) {
        app->hide_widget("NocLabel");
        app->hide_widget("ToggleNocBox");
    } else {
        QComboBox* toggleNocBox = app->find_combo_box("ToggleNocBox");
        QObject::connect(toggleNocBox, &QComboBox::currentIndexChanged, toggleNocBox, [toggleNocBox, app]() {
            toggle_noc_cbk(toggleNocBox, app);
        });
    }
}

/**
 * @brief configures and connects signals/functions for routing buttons
 *
 * Connects signals/sets default values for toggleRRButton, ToggleCongestion,
 * ToggleCongestionCost, ToggleRoutingBBox, RoutingExpansionCost, ToggleRoutingUtil
 * buttons.
 */
void routing_button_setup(ezgl::application* app) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    t_draw_state* draw_state = get_draw_state_vars();

    //Toggle RR
    SwitchButton* toggle_nets_switch = app->find_switch_button("ToggleRR");
    QObject::connect(toggle_nets_switch, &SwitchButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
        toggle_rr_cbk(toggle_nets_switch, checked, app);
    });

    // RR Checkboxes

    setup_checkbox_button("ToggleRRChannels", app, &draw_state->draw_channel_nodes);
    setup_checkbox_button("ToggleInterClusterPinNodes", app, &draw_state->draw_inter_cluster_pins);
    setup_checkbox_button("ToggleRRIntraClusterNodes", app, &draw_state->draw_intra_cluster_nodes);
    setup_checkbox_button("ToggleRRSBox", app, &draw_state->draw_switch_box_edges);
    setup_checkbox_button("ToggleRRCBox", app, &draw_state->draw_connection_box_edges);
    setup_checkbox_button("ToggleRRIntraClusterEdges", app, &draw_state->draw_intra_cluster_edges);
    setup_checkbox_button("ToggleHighlightRR", app, &draw_state->highlight_rr_edges);

    //Toggle Congestion
    QComboBox* toggle_congestion = app->find_combo_box("ToggleCongestion");
    QObject::connect(toggle_congestion, &QComboBox::currentIndexChanged, toggle_congestion, [toggle_congestion, app]() {
        toggle_cong_cbk(toggle_congestion, app);
    });

    //Toggle Congestion Cost
    QComboBox* toggle_cong_cost = app->find_combo_box("ToggleCongestionCost");
    QObject::connect(toggle_cong_cost, &QComboBox::currentIndexChanged, toggle_cong_cost, [toggle_cong_cost, app]() {
        toggle_cong_cost_cbk(toggle_cong_cost, app);
    });

    //Toggle Routing BB
    QSpinBox* toggle_routing_bbox = app->find_spin_box("ToggleRoutingBBox");
    QObject::connect(toggle_routing_bbox, &QSpinBox::valueChanged, toggle_routing_bbox, [toggle_routing_bbox, app]() {
        toggle_routing_bbox_cbk(toggle_routing_bbox, app);
    });
    toggle_routing_bbox->setSingleStep(1);
    toggle_routing_bbox->setRange(-1, route_ctx.route_bb.size() - 1);
    toggle_routing_bbox->setValue(-1);

    //Toggle Routing Expansion Costs
    QComboBox* toggle_expansion_cost = app->find_combo_box("ToggleRoutingExpansionCost");
    QObject::connect(toggle_expansion_cost, &QComboBox::currentIndexChanged, toggle_expansion_cost, [toggle_expansion_cost, app]() {
        toggle_expansion_cost_cbk(toggle_expansion_cost, app);
    });

    //Toggle Router Util
    QComboBox* toggle_router_util = app->find_combo_box("ToggleRoutingUtil");
    QObject::connect(toggle_router_util, &QComboBox::currentIndexChanged, toggle_router_util, [toggle_router_util, app]() {
        toggle_router_util_cbk(toggle_router_util, app);
    });
    app->show_widget("RoutingMenuButton");
}

void view_button_setup(ezgl::application* app) {
    int num_layers;

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    num_layers = device_ctx.grid.get_num_layers();

    // Hide the button if we only have one layer
    if (num_layers == 1) {
        app->hide_widget("3DMenuButton");
    } else {
        QWidget* box_widget = app->find_widget("LayerBox");
        QWidget* trans_box_widget = app->find_widget("TransparencyBox");

        QBoxLayout* box = qobject_cast<QBoxLayout*>(box_widget->layout());
        QBoxLayout* trans_box = qobject_cast<QBoxLayout*>(trans_box_widget->layout());

        // Create checkboxes and spin buttons for each layer
        for (int i = 0; i < num_layers; i++) {
            std::string label = "Layer " + std::to_string(i);
            std::string trans_label = "Transparency " + std::to_string(i);

            QCheckBox* checkbox = new QCheckBox(label.c_str());
            // Add margins to checkboxes to match the transparency spin button height
            ezgl::widget_set_margin_top(checkbox, 7);
            ezgl::widget_set_margin_bottom(checkbox, 7);

            ezgl::box_pack_start(box, checkbox, false, false, 0);

            QSpinBox* spin_button = new QSpinBox;
            spin_button->setRange(0, 255);
            spin_button->setSingleStep(1);
            spin_button->setObjectName(QString::fromStdString(trans_label));
            ezgl::box_pack_start(trans_box, spin_button, false, false, 0);

            if (i == 0) {
                // Set the initial state of the first checkbox to checked to represent the default view.
                checkbox->setChecked(true);
            }
            QObject::connect(checkbox, &QAbstractButton::toggled, checkbox, [checkbox]() {
                select_layer_cbk(checkbox, /*response_id=*/0, /*data=*/nullptr);
            });
            QObject::connect(spin_button, &QSpinBox::valueChanged, spin_button, [spin_button]() {
                transparency_cbk(spin_button, /*response_id=*/0, /*data=*/nullptr);
            });
        }

        // Set up the final row for cross-layer connections
        std::string label = "Cross Layer Connections";
        std::string trans_label = "CrossLayerConnectionsTransparency";

        QCheckBox* checkbox = new QCheckBox(label.c_str());
        ezgl::widget_set_margin_top(checkbox, 7);
        ezgl::widget_set_margin_bottom(checkbox, 7);
        ezgl::box_pack_start(box, checkbox, false, false, 0);

        QSpinBox* spin_button = new QSpinBox;
        spin_button->setRange(0, 255);
        spin_button->setSingleStep(1);
        spin_button->setObjectName(QString::fromStdString(trans_label));
        ezgl::box_pack_start(trans_box, spin_button, false, false, 0);

        QObject::connect(checkbox, &QAbstractButton::toggled, checkbox, [checkbox]() {
            cross_layer_checkbox_cbk(checkbox, /*response_id=*/0, /*data=*/nullptr);
        });
        QObject::connect(spin_button, &QSpinBox::valueChanged, spin_button, [spin_button]() {
            cross_layer_transparency_cbk(spin_button, /*response_id=*/0, /*data=*/nullptr);
        });

        // Make all widgets in the boxes appear
        box_widget->show();
        trans_box_widget->show();
    }
}

/*
 * @brief Loads required data for search autocomplete, sets up special completion fn
 */
void search_setup(ezgl::application* app) {
    load_block_names(app);
    load_net_names(app);
}

/**
 * @brief connects critical path button to its cbk fn
 *
 * @param app ezgl application
 */
void crit_path_button_setup(ezgl::application* app) {

    t_draw_state* draw_state = get_draw_state_vars();

    // Toggle Critical Path
    SwitchButton* toggle_nets_switch = app->find_switch_button("ToggleCritPath");
    QObject::connect(toggle_nets_switch, &SwitchButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
        toggle_crit_path_cbk(toggle_nets_switch, checked, app);
    });

    // Checkboxes for critical path
    setup_checkbox_button("ToggleCritPathFlylines", app, &draw_state->show_crit_path_flylines);
    setup_checkbox_button("ToggleCritPathRouting", app, &draw_state->show_crit_path_routing);
    setup_checkbox_button("ToggleCritPathDelays", app, &draw_state->show_crit_path_delays);
}

/**
 * @brief Hides or displays critical path routing / routing delay UI elements
 *
 * @param app ezgl app
 */
void hide_crit_path_routing(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    bool state = draw_state->setup_timing_info && draw_state->pic_on_screen == e_pic_type::ROUTING && draw_state->show_crit_path;

    app->find_widget("ToggleCritPathRouting")->setEnabled(state);
}

void hide_draw_routing(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    QComboBox* toggle_nets = app->find_combo_box("ToggleNetType");

    // Enable the option to draw routing only during the routing stage
    int route_item_index = toggle_nets->findText("Routing");
    if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
        if (route_item_index != -1) {
            toggle_nets->removeItem(route_item_index);
        }
    } else {
        if (route_item_index == -1) {
            toggle_nets->addItem("Routing", "2");
        }
    }
}

/**
 * @brief loads atom and cluster lvl names into gtk list store item used for completion
 *
 * @param app ezgl application used for ui
 */
void load_block_names(ezgl::application* app) {
    QLineEdit* textInput = app->find_line_edit("TextInput");
    if (!textInput) return;

    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    QStringList block_names;
    for (ClusterBlockId id : cluster_ctx.clb_nlist.blocks()) {
        block_names.append(QString::fromStdString(cluster_ctx.clb_nlist.block_name(id)));
    }
    for (AtomBlockId id : atom_ctx.netlist().blocks()) {
        block_names.append(QString::fromStdString(atom_ctx.netlist().block_name(id)));
    }

    QCompleter* completer = new QCompleter(block_names, textInput);
    completer->setObjectName("BlockNames");
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
}

/*
 * @brief loads atom net names into gtk list store item used for completion
 *
 * @param app ezgl application used for ui
 */
void load_net_names(ezgl::application* app) {
    QLineEdit* textInput = qobject_cast<QLineEdit*>(app->find_line_edit("TextInput"));
    if (!textInput) return;

    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    QStringList net_names;
    for (AtomNetId id : atom_ctx.netlist().nets()) {
        net_names.append(QString::fromStdString(atom_ctx.netlist().net_name(id)));
    }

    QCompleter* completer = new QCompleter(net_names, textInput);
    completer->setObjectName("NetNames");
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
}

#endif /* NO_GRAPHICS */
