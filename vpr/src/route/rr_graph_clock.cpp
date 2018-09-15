#include "rr_graph_clock.h"
#include "clock_network_types.h"

#include "globals.h"
#include "rr_graph2.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"

void ClockRRGraph::create_and_append_clock_rr_graph() {

    vtr::printf_info("Starting clock network routing resource graph generation...\n");
    clock_t begin = clock();

    // TODO: Eventually remove create_star_model. Currently used to simulate that a simple network
    //       functions
    //create_star_model_network();


    // Clocks
    std::vector<std::unique_ptr<ClockNetwork>> clock_networks;

    // Clock Ribs
    clock_networks.emplace_back(new ClockRib());
    ClockRib* rib = dynamic_cast<ClockRib*>(clock_networks.back().get());
    
    rib->set_num_instance(10);
    rib->set_clock_name("rib1");
    rib->set_metal_layer(0, 0);
    rib->set_initial_wire_location(0, 9, 0);
    rib->set_wire_repeat(9, 1);
    rib->set_drive_location(5);
    rib->set_drive_switch(2);
    rib->set_tap_locations(0,1);

    // Clock Spines
    clock_networks.emplace_back(new ClockSpine());
    ClockSpine* spine = dynamic_cast<ClockSpine*>(clock_networks.back().get());

    spine->set_num_instance(10);
    spine->set_clock_name("spine1");
    spine->set_metal_layer(0,0);
    spine->set_initial_wire_location(0,10,5);
    spine->set_wire_repeat(10,10);
    spine->set_drive_location(5);
    spine->set_drive_switch(2);
    spine->set_tap_locations(0,1);

    ClockRRGraph clock_graph = ClockRRGraph();
    clock_graph.allocate_lookup(clock_networks);
    clock_graph.create_clock_networks_wires(clock_networks);

//    std::vector<ClockConnection> clock_routing;
//    ClockConnection connection

    // TODO: Add a shrink to fit for the rr_nodes

    float elapsed_time = (float) (clock() - begin) / CLOCKS_PER_SEC;
    vtr::printf_info("Building clock network resource graph took %g seconds\n", elapsed_time);
}

void ClockRRGraph::create_clock_networks_wires(
        std::vector<std::unique_ptr<ClockNetwork>>& clock_networks) {
    for (auto& clock_network : clock_networks) {
        clock_network->create_rr_nodes_for_clock_network_wires(*this);
    }
}

void ClockRRGraph::allocate_lookup(const std::vector<std::unique_ptr<ClockNetwork>>& clock_networks) {

    auto& grid = g_vpr_ctx.mutable_device().grid;

    for(auto& clock_network : clock_networks) {
       
        SwitchPoints switch_points;
        
        /* TODO: loop over all switchs and insert into map. */
        // Currently only supporting two switch points
        // Single Drive
        std::string drive_switch_name = "drive";
        SwitchPoint drive_switch_locations(grid.width(), grid.height());
        switch_points.insert_switch(drive_switch_name, drive_switch_locations);
        // Single Tap
        std::string tap_switch_name = "tap";
        SwitchPoint tap_switch_locations(grid.width(), grid.height());
        switch_points.insert_switch(tap_switch_name, tap_switch_locations);

        clock_name_to_switch_points.emplace(clock_network->get_name(), switch_points);
    }
}

void ClockRRGraph::add_switch_location(
        std::string clock_name,
        std::string switch_name,
        int x,
        int y,
        int node_index) {
    clock_name_to_switch_points[clock_name].insert_switch_node_idx(switch_name, x, y, node_index);
}


//void ClockRRGraph::create_star_model_network() {
//
//    vtr::printf_info("Creating a clock network in the form of a star model\n");
//
//    auto& device_ctx = g_vpr_ctx.mutable_device();
//    auto& rr_nodes = device_ctx.rr_nodes;
//    auto& rr_node_indices = device_ctx.rr_node_indices;
//
//    // 1) Create the clock source wire (located at the center of the chip)
//
//    // a) Find the center of the chip
//    auto& grid = device_ctx.grid;
//    auto x_mid_dim = grid.width() / 2;
//    auto y_mid_dim = grid.height() / 2;
//
//    // b) Create clock source wire node at at the center of the chip
//    rr_nodes.emplace_back();
//    auto clock_source_idx = rr_nodes.size() - 1; // last inserted node
//    rr_nodes[clock_source_idx].set_coordinates(x_mid_dim, x_mid_dim, y_mid_dim, y_mid_dim);
//    rr_nodes[clock_source_idx].set_type(CHANX);
//    rr_nodes[clock_source_idx].set_capacity(1);
//
//    // Find all I/O inpads and connect all I/O output pins to the clock source wire
//    // through a switch.
//    // Resize the rr_nodes array to 2x the number of I/O input pins
//    for (size_t i = 0; i < grid.width(); ++i) {
//        for (size_t j = 0; j < grid.height(); j++) {
//
//            auto type = grid[i][j].type;
//            auto width_offset = grid[i][j].width_offset;
//            auto height_offset = grid[i][j].height_offset;
//            for (e_side side : SIDES) {
//                //Don't connect pins which are not adjacent to channels around the perimeter
//                if (   (i == 0 && side != RIGHT)
//                    || (i == int(grid.width() - 1) && side != LEFT)
//                    || (j == 0 && side != TOP)
//                    || (j == int(grid.width() - 1) && side != BOTTOM)) {
//                    continue;
//                }
//
//                for (int pin_index = 0; pin_index < type->num_pins; pin_index++) {
//                    /* We only are working with opins so skip non-drivers */
//                    if (type->class_inf[type->pin_class[pin_index]].type != DRIVER) {
//                        continue;
//                    }
//
//                    /* Can't do anything if pin isn't at this location */
//                    if (0 == type->pinloc[width_offset][height_offset][side][pin_index]) {
//                        continue;
//                    }
//                    vtr::printf_info("%s \n",type->pb_type->name);
//                    if (strcmp(type->pb_type->name, "io") != 0) {
//                        continue;
//                    }
//
//                    auto node_index = get_rr_node_index(rr_node_indices, i, j, OPIN, pin_index, side);
//                    rr_nodes[node_index].add_edge(clock_source_idx, 0);
//                    vtr::printf_info("At %d,%d output pin node %d\n", i, j, node_index);
//
//
//                }
//                for (auto pin_index : type->get_clock_pins_indices()) {
//
//
//                    /* Can't do anything if pin isn't at this location */
//                    if (0 == type->pinloc[width_offset][height_offset][side][pin_index]) {
//                        continue;
//                    }
//
//
//                    auto node_index = get_rr_node_index(rr_node_indices, i, j, IPIN, pin_index, side);
//                    rr_nodes[clock_source_idx].add_edge(node_index, 1);
//                    vtr::printf_info("At %d,%d input pin node %d\n", i, j, node_index);
//
//                }
//
//            }
//
//            // Loop over ports
//                // Is the port class clock_source
//                    // Assert pin is an output (check)
//                    // find node using t_rr_node_index (check))
//                    // get_rr_node_index(rr_nodes, i, j, OPIN, side) (check)
//                    // create an edge to clock source wire (check)
//                // Is the port a clock
//                    // find pin using t_rr_node_indices (rr_type is a IPIN)
//                    // create an edge from the clock source wire to the
//        }
//    }
//
//    // Find all clock pins and connect them to the clock source wire
//
//
//    vtr::printf_info("Finished creating star model clock network\n");
//}

