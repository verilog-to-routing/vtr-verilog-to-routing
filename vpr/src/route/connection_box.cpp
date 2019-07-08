#include "connection_box.h"
#include "vtr_assert.h"
#include "globals.h"

ConnectionBoxes::ConnectionBoxes()
    : size_(std::make_pair(0, 0)) {
}

size_t ConnectionBoxes::num_connection_box_types() const {
    return boxes_.size();
}

std::pair<size_t, size_t> ConnectionBoxes::connection_box_grid_size() const {
    return size_;
}

const ConnectionBox* ConnectionBoxes::get_connection_box(ConnectionBoxId box) const {
    if (bool(box)) {
        return nullptr;
    }

    size_t index = size_t(box);
    if (index >= boxes_.size()) {
        return nullptr;
    }

    return &boxes_.at(index);
}

bool ConnectionBoxes::find_connection_box(int inode,
                                          ConnectionBoxId* box_id,
                                          std::pair<size_t, size_t>* box_location) const {
    VTR_ASSERT(box_id != nullptr);
    VTR_ASSERT(box_location != nullptr);

    const auto& conn_box_loc = ipin_map_[inode];
    if (conn_box_loc.box_id == ConnectionBoxId::INVALID()) {
        return false;
    }

    *box_id = conn_box_loc.box_id;
    *box_location = conn_box_loc.box_location;
    return true;
}

// Clear IPIN map and set connection box grid size and box ids.
void ConnectionBoxes::reset_boxes(std::pair<size_t, size_t> size,
                                  const std::vector<ConnectionBox> boxes) {
    clear();

    size_ = size;
    boxes_ = boxes;
}

void ConnectionBoxes::resize_nodes(size_t rr_node_size) {
    ipin_map_.resize(rr_node_size);
    canonical_loc_map_.resize(rr_node_size,
                              std::make_pair(-1, -1));
}

void ConnectionBoxes::clear() {
    ipin_map_.clear();
    size_ = std::make_pair(0, 0);
    boxes_.clear();
    canonical_loc_map_.clear();
    sink_to_ipin_.clear();
}

void ConnectionBoxes::add_connection_box(int inode, ConnectionBoxId box_id, std::pair<size_t, size_t> box_location) {
    // Ensure that box location is in bounds
    VTR_ASSERT(box_location.first < size_.first);
    VTR_ASSERT(box_location.second < size_.second);

    // Bounds check box_id
    VTR_ASSERT(bool(box_id));
    VTR_ASSERT(size_t(box_id) < boxes_.size());

    // Make sure sink map will not be invalidated upon insertion.
    VTR_ASSERT(sink_to_ipin_.size() == 0);

    ipin_map_[inode] = ConnBoxLoc(box_location, box_id);
}

void ConnectionBoxes::add_canonical_loc(int inode, std::pair<size_t, size_t> loc) {
    VTR_ASSERT(loc.first < size_.first);
    VTR_ASSERT(loc.second < size_.second);
    canonical_loc_map_[inode] = loc;
}

const std::pair<size_t, size_t>* ConnectionBoxes::find_canonical_loc(int inode) const {
    const auto& canon_loc = canonical_loc_map_[inode];
    if (canon_loc.first == size_t(-1)) {
        return nullptr;
    }

    return &canon_loc;
}

void ConnectionBoxes::create_sink_back_ref() {
    const auto& device_ctx = g_vpr_ctx.device();

    sink_to_ipin_.resize(device_ctx.rr_nodes.size(), {{0, 0, 0, 0}, 0});

    for (size_t i = 0; i < device_ctx.rr_nodes.size(); ++i) {
        const auto& ipin_node = device_ctx.rr_nodes[i];
        if (ipin_node.type() != IPIN) {
            continue;
        }

        if (ipin_map_[i].box_id == ConnectionBoxId::INVALID()) {
            continue;
        }

        for (auto edge : ipin_node.edges()) {
            int sink_inode = ipin_node.edge_sink_node(edge);
            VTR_ASSERT(device_ctx.rr_nodes[sink_inode].type() == SINK);
            VTR_ASSERT(sink_to_ipin_[sink_inode].ipin_count < 4);
            auto& sink_to_ipin = sink_to_ipin_[sink_inode];
            sink_to_ipin.ipin_nodes[sink_to_ipin.ipin_count++] = i;
        }
    }
}

const SinkToIpin& ConnectionBoxes::find_sink_connection_boxes(
    int inode) const {
    return sink_to_ipin_[inode];
}
