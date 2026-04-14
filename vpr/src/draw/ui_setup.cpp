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
#include "gtkcomboboxhelper.h"

#include "ezgl/application.hpp"

#include "vpr_qtcompat.h"
#include <QStringList>
#include <QCompleter>

/**
 * @brief Helper function to connect a toggle button to a callback function
 */
static void setup_checkbox_button(std::string button_id, ezgl::application* app, bool* toggle_state) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkToggleButton* checkbox_button = GTK_TOGGLE_BUTTON(app->get_object(button_id.c_str()));
    draw_state->checkbox_data.emplace_back(app, toggle_state);
    t_checkbox_data* data = &draw_state->checkbox_data.back();
    QObject::connect(checkbox_button, &QAbstractButton::toggled, checkbox_button, [checkbox_button, data]() {
        toggle_checkbox_cbk(checkbox_button, data);
    });
}

void basic_button_setup(ezgl::application* app) {
    //button to enter window_mode, created in main.ui
    GtkButton* window = (GtkButton*)app->get_object("Window");
    QObject::connect(window, &QAbstractButton::clicked, window, [app]() {
        toggle_window_mode(/*widget=*/nullptr, app);
    });

    //button to search, created in main.ui
    GtkButton* search = (GtkButton*)app->get_object("Search");
    gtk_button_set_label(search, "Search");
    QObject::connect(search, &QAbstractButton::clicked, search, [app]() {
        search_and_highlight(/*widget=*/nullptr, app);
    });

    //button for save graphics, created in main.ui
    GtkButton* save = (GtkButton*)app->get_object("SaveGraphics");
    QObject::connect(save, &QAbstractButton::clicked, save, []() {
        save_graphics_dialog_box(/*widget=*/nullptr, /*app=*/nullptr);
    });

    //combo box for search type, created in main.ui
    GObject* search_type = (GObject*)app->get_object("SearchType");
    QComboBox* search_type_combo = qobject_cast<QComboBox*>(search_type);
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
    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleNets"));
    QObject::connect(toggle_nets_switch, &QAbstractButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
        toggle_show_nets_cbk(toggle_nets_switch, checked, app);
    });

    // Manages net type
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNetType"));
    QObject::connect(toggle_nets, &QComboBox::currentIndexChanged, toggle_nets, [toggle_nets, app]() {
        toggle_draw_nets_cbk(toggle_nets, app);
    });

    setup_checkbox_button("ToggleInterClusterNets", app, &draw_state->draw_inter_cluster_nets);

    setup_checkbox_button("ToggleIntraClusterNets", app, &draw_state->draw_intra_cluster_nets);

    setup_checkbox_button("FanInFanOut", app, &draw_state->highlight_fan_in_fan_out);

    //Manages net alpha
    GtkSpinButton* net_alpha = GTK_SPIN_BUTTON(app->get_object("NetAlpha"));
    QObject::connect(net_alpha, &QSpinBox::valueChanged, net_alpha, [net_alpha, app]() {
        set_net_alpha_value_cbk(net_alpha, app);
    });
    gtk_spin_button_set_increments(net_alpha, 1, 1);
    gtk_spin_button_set_range(net_alpha, 0, 255);

    //Manages net max fanout
    GtkSpinButton* max_fanout = GTK_SPIN_BUTTON(app->get_object("NetMaxFanout"));
    QObject::connect(max_fanout, &QSpinBox::valueChanged, max_fanout, [max_fanout, app]() {
        set_net_max_fanout_cbk(max_fanout, app);
    });
    gtk_spin_button_set_increments(max_fanout, 1, 1);
    gtk_spin_button_set_range(max_fanout, 0., (double)get_max_fanout());
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
    GtkSpinButton* blk_internals_button = GTK_SPIN_BUTTON(app->get_object("ToggleBlkInternals"));
    QObject::connect(blk_internals_button, &QSpinBox::valueChanged, blk_internals_button, [blk_internals_button, app]() {
        toggle_blk_internal_cbk(blk_internals_button, app);
    });
    gtk_spin_button_set_increments(blk_internals_button, 1, 1);
    gtk_spin_button_set_range(blk_internals_button, 0., (double)(draw_state->max_sub_blk_lvl));

    //Toggle Block Pin Util
    GtkComboBoxText* blk_pin_util = GTK_COMBO_BOX_TEXT(app->get_object("ToggleBlkPinUtil"));
    QObject::connect(blk_pin_util, &QComboBox::currentIndexChanged, blk_pin_util, [blk_pin_util, app]() {
        toggle_blk_pin_util_cbk(blk_pin_util, app);
    });

    //Toggle Placement Macros
    GtkComboBoxText* placement_macros = GTK_COMBO_BOX_TEXT(app->get_object("TogglePlacementMacros"));
    QObject::connect(placement_macros, &QComboBox::currentIndexChanged, placement_macros, [placement_macros, app]() {
        placement_macros_cbk(placement_macros, app);
    });

    //Toggle NoC Display (based on startup cmd --noc on)
    if (!draw_state->show_noc_button) {
        hide_widget("NocLabel", app);
        hide_widget("ToggleNocBox", app);
    } else {
        GtkComboBoxText* toggleNocBox = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNocBox"));
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
    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleRR"));
    QObject::connect(toggle_nets_switch, &QAbstractButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
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
    GtkComboBoxText* toggle_congestion = GTK_COMBO_BOX_TEXT(app->get_object("ToggleCongestion"));
    QObject::connect(toggle_congestion, &QComboBox::currentIndexChanged, toggle_congestion, [toggle_congestion, app]() {
        toggle_cong_cbk(toggle_congestion, app);
    });

    //Toggle Congestion Cost
    GtkComboBoxText* toggle_cong_cost = GTK_COMBO_BOX_TEXT(app->get_object("ToggleCongestionCost"));
    QObject::connect(toggle_cong_cost, &QComboBox::currentIndexChanged, toggle_cong_cost, [toggle_cong_cost, app]() {
        toggle_cong_cost_cbk(toggle_cong_cost, app);
    });

    //Toggle Routing BB
    GtkSpinButton* toggle_routing_bbox = GTK_SPIN_BUTTON(app->get_object("ToggleRoutingBBox"));
    QObject::connect(toggle_routing_bbox, &QSpinBox::valueChanged, toggle_routing_bbox, [toggle_routing_bbox, app]() {
        toggle_routing_bbox_cbk(toggle_routing_bbox, app);
    });
    gtk_spin_button_set_increments(toggle_routing_bbox, 1, 1);
    gtk_spin_button_set_range(toggle_routing_bbox, -1., (double)(route_ctx.route_bb.size() - 1));
    gtk_spin_button_set_value(toggle_routing_bbox, -1.);

    //Toggle Routing Expansion Costs
    GtkComboBoxText* toggle_expansion_cost = GTK_COMBO_BOX_TEXT(app->get_object("ToggleRoutingExpansionCost"));
    QObject::connect(toggle_expansion_cost, &QComboBox::currentIndexChanged, toggle_expansion_cost, [toggle_expansion_cost, app]() {
        toggle_expansion_cost_cbk(toggle_expansion_cost, app);
    });

    //Toggle Router Util
    GtkComboBoxText* toggle_router_util = GTK_COMBO_BOX_TEXT(app->get_object("ToggleRoutingUtil"));
    QObject::connect(toggle_router_util, &QComboBox::currentIndexChanged, toggle_router_util, [toggle_router_util, app]() {
        toggle_router_util_cbk(toggle_router_util, app);
    });
    show_widget("RoutingMenuButton", app);
}

void view_button_setup(ezgl::application* app) {
    int num_layers;

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    num_layers = device_ctx.grid.get_num_layers();

    // Hide the button if we only have one layer
    if (num_layers == 1) {
        hide_widget("3DMenuButton", app);
    } else {
        GtkBox* box = GTK_BOX(app->get_object("LayerBox"));
        GtkBox* trans_box = GTK_BOX(app->get_object("TransparencyBox"));

        // Create checkboxes and spin buttons for each layer
        for (int i = 0; i < num_layers; i++) {
            std::string label = "Layer " + std::to_string(i);
            std::string trans_label = "Transparency " + std::to_string(i);

            GtkWidget* checkbox = gtk_check_button_new_with_label(label.c_str());
            // Add margins to checkboxes to match the transparency spin button height
            gtk_widget_set_margin_top(checkbox, 7);
            gtk_widget_set_margin_bottom(checkbox, 7);

            gtk_box_pack_start(GTK_BOX(box), checkbox, FALSE, FALSE, 0);

            GtkWidget* spin_button = gtk_spin_button_new_with_range(0, 255, 1);
            gtk_widget_set_name(spin_button, g_strdup(trans_label.c_str()));
            gtk_box_pack_start(GTK_BOX(trans_box), spin_button, FALSE, FALSE, 0);

            if (i == 0) {
                // Set the initial state of the first checkbox to checked to represent the default view.
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), TRUE);
            }
            QObject::connect(GTK_BUTTON(checkbox), &QAbstractButton::toggled, GTK_BUTTON(checkbox), [checkbox]() {
                select_layer_cbk(checkbox, /*response_id=*/0, /*data=*/nullptr);
            });
            QObject::connect(GTK_SPIN_BUTTON(spin_button), &QSpinBox::valueChanged, GTK_SPIN_BUTTON(spin_button), [spin_button]() {
                transparency_cbk(spin_button, /*response_id=*/0, /*data=*/nullptr);
            });
        }

        // Set up the final row for cross-layer connections
        std::string label = "Cross Layer Connections";
        std::string trans_label = "CrossLayerConnectionsTransparency";

        GtkWidget* checkbox = gtk_check_button_new_with_label(label.c_str());
        gtk_widget_set_margin_top(checkbox, 7);
        gtk_widget_set_margin_bottom(checkbox, 7);
        gtk_box_pack_start(GTK_BOX(box), checkbox, FALSE, FALSE, 0);

        GtkWidget* spin_button = gtk_spin_button_new_with_range(0, 255, 1);
        gtk_widget_set_name(spin_button, g_strdup(trans_label.c_str()));
        gtk_box_pack_start(GTK_BOX(trans_box), spin_button, FALSE, FALSE, 0);

        QObject::connect(GTK_BUTTON(checkbox), &QAbstractButton::toggled, GTK_BUTTON(checkbox), [checkbox]() {
            cross_layer_checkbox_cbk(checkbox, /*response_id=*/0, /*data=*/nullptr);
        });
        QObject::connect(GTK_SPIN_BUTTON(spin_button), &QSpinBox::valueChanged, GTK_SPIN_BUTTON(spin_button), [spin_button]() {
            cross_layer_transparency_cbk(spin_button, /*response_id=*/0, /*data=*/nullptr);
        });

        // Make all widgets in the boxes appear
        gtk_widget_show_all(GTK_WIDGET(box));
        gtk_widget_show_all(GTK_WIDGET(trans_box));
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
    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleCritPath"));
    QObject::connect(toggle_nets_switch, &QAbstractButton::toggled, toggle_nets_switch, [toggle_nets_switch, app](bool checked) {
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

    gtk_widget_set_sensitive(GTK_WIDGET(app->get_object("ToggleCritPathRouting")), state);
}

void hide_draw_routing(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNetType"));

    // Enable the option to draw routing only during the routing stage
    int route_item_index = get_item_index_by_text(toggle_nets, "Routing");
    if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
        if (route_item_index != -1) {
            gtk_combo_box_text_remove(toggle_nets, route_item_index);
        }
    } else {
        if (route_item_index == -1) {
            gtk_combo_box_text_append(toggle_nets, "2", "Routing");
        }
    }
}

/*
 * @brief Hides the widget with the given name
 *
 * @param widgetName string of widget name in main.ui
 * @param app ezgl app
 */
void hide_widget(std::string widgetName, ezgl::application* app) {
    GtkWidget* widget = GTK_WIDGET(app->get_object(widgetName.c_str()));
    gtk_widget_hide(widget);
}

/**
 * @brief Hides the widget with the given name
 */
void show_widget(std::string widgetName, ezgl::application* app) {
    GtkWidget* widget = GTK_WIDGET(app->get_object(widgetName.c_str()));
    gtk_widget_show(widget);
}

/**
 * @brief loads atom and cluster lvl names into gtk list store item used for completion
 *
 * @param app ezgl application used for ui
 */
void load_block_names(ezgl::application* app) {
    QLineEdit* textInput = qobject_cast<QLineEdit*>(app->get_object("TextInput"));
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
    QLineEdit* textInput = qobject_cast<QLineEdit*>(app->get_object("TextInput"));
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
