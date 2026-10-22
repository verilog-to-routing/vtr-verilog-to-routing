#include "cluster_pin_map.h"

#include "physical_types_util.h"
#include "vpr_types.h"
#include "vtr_assert.h"

ClusterPinMap::ClusterPinMap(t_physical_tile_type_ptr physical_type,
                             const t_sub_tile* sub_tile,
                             int rel_cap,
                             t_logical_block_type_ptr logical_block)
    : physical_type_(physical_type)
    , sub_tile_(sub_tile)
    , rel_cap_(rel_cap)
    , logical_block_(logical_block)
    , intra_pin_offset_(sub_tile->intra_pin_range[rel_cap].at(logical_block).low) {
    // The pins of a sub-tile instance on the tile are contiguous (see set_root_pin_to_pb_pin_map())
    int inst_num_pins = sub_tile->num_phy_pins / sub_tile->capacity.total();
    first_tile_pin_ = sub_tile->sub_tile_to_tile_pin_indices[rel_cap * inst_num_pins];
    tile_pin_pb_pins_.resize(inst_num_pins);
    root_pin_physical_nums_.assign(logical_block->pb_type->num_pins, UNDEFINED);
    for (int inst_pin = 0; inst_pin < inst_num_pins; inst_pin++) {
        int tile_pin = first_tile_pin_ + inst_pin;
        VTR_ASSERT(sub_tile->sub_tile_to_tile_pin_indices[rel_cap * inst_num_pins + inst_pin] == tile_pin);
        const t_pb_graph_pin* pin = physical_type->on_tile_pin_num_to_pb_pin.at(tile_pin).at(logical_block);
        VTR_ASSERT(pin->pin_count_in_cluster < (int)root_pin_physical_nums_.size());
        tile_pin_pb_pins_[inst_pin] = pin;
        root_pin_physical_nums_[pin->pin_count_in_cluster] = tile_pin;
    }
}

const t_pb_graph_pin* ClusterPinMap::pb_pin(int pin_physical_num) const {
    if (is_pin_on_tile(physical_type_, pin_physical_num)) {
        VTR_ASSERT_SAFE(pin_physical_num >= first_tile_pin_ && pin_physical_num - first_tile_pin_ < (int)tile_pin_pb_pins_.size());
        return tile_pin_pb_pins_[pin_physical_num - first_tile_pin_];
    }
    return physical_type_->pin_num_to_pb_pin.at(pin_physical_num);
}

int ClusterPinMap::physical_num(const t_pb_graph_pin* pin) const {
    if (pin->is_root_block_pin()) {
        int tile_pin = root_pin_physical_nums_[pin->pin_count_in_cluster];
        VTR_ASSERT(tile_pin != UNDEFINED);
        return tile_pin;
    }
    return intra_pin_offset_ + pin->pin_count_in_cluster;
}

bool ClusterPinMap::is_primitive_pin(int pin_physical_num) const {
    return !is_pin_on_tile(physical_type_, pin_physical_num) && pb_pin(pin_physical_num)->is_primitive_pin();
}

float ClusterPinMap::edge_delay(int src_pin_physical_num, int sink_pin_physical_num) const {
    return edge(pb_pin(src_pin_physical_num), pb_pin(sink_pin_physical_num))->delay_max;
}

const t_pb_graph_edge* ClusterPinMap::edge(const t_pb_graph_pin* from, const t_pb_graph_pin* to) {
    return from->output_edges[from->sink_pin_edge_idx_map.at(to)];
}
