
#include "rr_graph_tile_nodes.h"

#include "rr_graph_builder.h"
#include "rr_rc_data.h"
#include "rr_graph_cost.h"
#include "physical_types_util.h"
#include "vpr_utils.h"
#include "globals.h"

void add_classes_rr_graph(RRGraphBuilder& rr_graph_builder,
                          const std::vector<int>& class_num_vec,
                          const t_physical_tile_loc& root_loc,
                          t_physical_tile_type_ptr physical_type) {
    DeviceContext& mutable_device_ctx = g_vpr_ctx.mutable_device();

    for (int class_num : class_num_vec) {
        e_pin_type class_type = get_class_type_from_class_physical_num(physical_type, class_num);
        RRNodeId class_inode = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type, root_loc, class_num);
        VTR_ASSERT(class_inode != RRNodeId::INVALID());
        int class_num_pins = get_class_num_pins_from_class_physical_num(physical_type, class_num);
        if (class_type == e_pin_type::DRIVER) {
            rr_graph_builder.set_node_cost_index(class_inode, RRIndexedDataId(SOURCE_COST_INDEX));
            rr_graph_builder.set_node_type(class_inode, e_rr_type::SOURCE);
        } else {
            VTR_ASSERT(class_type == e_pin_type::RECEIVER);

            rr_graph_builder.set_node_cost_index(class_inode, RRIndexedDataId(SINK_COST_INDEX));
            rr_graph_builder.set_node_type(class_inode, e_rr_type::SINK);
        }
        VTR_ASSERT(class_num_pins <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_capacity(class_inode, (short)class_num_pins);
        VTR_ASSERT(root_loc.x <= std::numeric_limits<short>::max() && root_loc.y <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_coordinates(class_inode, (short)root_loc.x, (short)root_loc.y, (short)(root_loc.x + physical_type->width - 1), (short)(root_loc.y + physical_type->height - 1));
        VTR_ASSERT(root_loc.layer_num <= std::numeric_limits<short>::max());
        rr_graph_builder.set_node_layer(class_inode, root_loc.layer_num, root_loc.layer_num);
        float R = 0.;
        float C = 0.;
        rr_graph_builder.set_node_rc_index(class_inode, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));
        rr_graph_builder.set_node_class_num(class_inode, class_num);
    }
}

void add_pins_rr_graph(RRGraphBuilder& rr_graph_builder,
                       const std::vector<int>& pin_num_vec,
                       const t_physical_tile_loc& root_loc,
                       t_physical_tile_type_ptr physical_type) {
    auto& mutable_device_ctx = g_vpr_ctx.mutable_device();
    const RRSpatialLookup& node_lookup = rr_graph_builder.node_lookup();
    for (int pin_num : pin_num_vec) {
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_num);
        VTR_ASSERT(pin_type == e_pin_type::DRIVER || pin_type == e_pin_type::RECEIVER);
        std::vector<int> x_offset_vec;
        std::vector<int> y_offset_vec;
        std::vector<e_side> pin_sides_vec;
        std::tie(x_offset_vec, y_offset_vec, pin_sides_vec) = get_pin_coordinates(physical_type, pin_num, std::vector<e_side>(TOTAL_2D_SIDES.begin(), TOTAL_2D_SIDES.end()));
        VTR_ASSERT(!pin_sides_vec.empty());
        for (int pin_coord = 0; pin_coord < (int)pin_sides_vec.size(); pin_coord++) {
            int x_offset = x_offset_vec[pin_coord];
            int y_offset = y_offset_vec[pin_coord];
            e_side pin_side = pin_sides_vec[pin_coord];
            e_rr_type node_type = (pin_type == e_pin_type::DRIVER) ? e_rr_type::OPIN : e_rr_type::IPIN;
            RRNodeId node_id = node_lookup.find_node(root_loc.layer_num,
                                                     root_loc.x + x_offset,
                                                     root_loc.y + y_offset,
                                                     node_type,
                                                     pin_num,
                                                     pin_side);
            if (node_id != RRNodeId::INVALID()) {
                if (pin_type == e_pin_type::RECEIVER) {
                    rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(IPIN_COST_INDEX));
                } else {
                    VTR_ASSERT(pin_type == e_pin_type::DRIVER);
                    rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(OPIN_COST_INDEX));
                }

                rr_graph_builder.set_node_type(node_id, node_type);
                rr_graph_builder.set_node_capacity(node_id, 1);
                float R = 0.;
                float C = 0.;
                rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(R, C, mutable_device_ctx.rr_rc_data)));
                rr_graph_builder.set_node_pin_num(node_id, pin_num);
                // Note that we store the grid tile location and side where the pin is located,
                // which greatly simplifies the drawing code
                // For those pins located on multiple sides, we save the rr node index
                // for the pin on all sides at which it exists
                // As such, multiple driver problem can be avoided.
                rr_graph_builder.set_node_coordinates(node_id,
                                                      root_loc.x + x_offset,
                                                      root_loc.y + y_offset,
                                                      root_loc.x + x_offset,
                                                      root_loc.y + y_offset);
                rr_graph_builder.set_node_layer(node_id, root_loc.layer_num, root_loc.layer_num);
                rr_graph_builder.add_node_side(node_id, pin_side);
            }
        }
    }
}

void connect_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                              const std::vector<int>& class_num_vec,
                              const t_physical_tile_loc& tile_loc,
                              t_rr_edge_info_set& rr_edges_to_create,
                              const int delayless_switch,
                              t_physical_tile_type_ptr physical_type_ptr,
                              bool switches_remapped) {
    for (int class_num : class_num_vec) {
        const std::vector<int>& pin_list = get_pin_list_from_class_physical_num(physical_type_ptr, class_num);
        e_pin_type class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        RRNodeId class_rr_node_id = get_class_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, tile_loc, class_num);
        VTR_ASSERT(class_rr_node_id != RRNodeId::INVALID());
        for (int pin_num : pin_list) {
            RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type_ptr, tile_loc, pin_num);
            if (pin_rr_node_id == RRNodeId::INVALID()) {
                VTR_LOG_ERROR("In block (%d, %d, %d) pin num: %d doesn't exist to be connected to class %d\n",
                              tile_loc.layer_num,
                              tile_loc.x,
                              tile_loc.y,
                              pin_num,
                              class_num);
                continue;
            }
            e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
            if (class_type == e_pin_type::DRIVER) {
                VTR_ASSERT(pin_type == e_pin_type::DRIVER);
                rr_edges_to_create.emplace_back(class_rr_node_id, pin_rr_node_id, delayless_switch, switches_remapped);
            } else {
                VTR_ASSERT(class_type == e_pin_type::RECEIVER);
                VTR_ASSERT(pin_type == e_pin_type::RECEIVER);
                rr_edges_to_create.emplace_back(pin_rr_node_id, class_rr_node_id, delayless_switch, switches_remapped);
            }
        }
    }
}
