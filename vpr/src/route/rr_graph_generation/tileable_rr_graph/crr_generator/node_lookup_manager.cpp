#include "node_lookup_manager.h"

#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

NodeLookupManager::NodeLookupManager(const RRGraphView& rr_graph, size_t fpga_grid_x, size_t fpga_grid_y)
    : rr_graph_(rr_graph)
    , fpga_grid_x_(fpga_grid_x)
    , fpga_grid_y_(fpga_grid_y) {}

void NodeLookupManager::initialize() {
    VTR_LOG("Initializing NodeLookupManager for %d x %d grid with %zu nodes\n",
            fpga_grid_x_, fpga_grid_y_, static_cast<size_t>(rr_graph_.num_nodes()));

    // Clear existing data
    clear();

    // Initialize lookup structures
    column_lookup_.resize(static_cast<size_t>(fpga_grid_x_) + 1);
    row_lookup_.resize(static_cast<size_t>(fpga_grid_y_) + 1);

    // Index all nodes
    for (RRNodeId node_id : rr_graph_.nodes()) {
        index_node(node_id);
    }

    VTR_LOG("Node lookup manager initialized successfully\n");
    print_statistics();
}

const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& NodeLookupManager::get_column_nodes(Coordinate x) const {
    if (x <= fpga_grid_x_ && x < static_cast<Coordinate>(column_lookup_.size())) {
        return column_lookup_[static_cast<size_t>(x)];
    }
    return std::unordered_map<NodeHash, RRNodeId, NodeHasher>();
}

const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& NodeLookupManager::get_row_nodes(Coordinate y) const {
    if (y <= fpga_grid_y_ && y < static_cast<Coordinate>(row_lookup_.size())) {
        return row_lookup_[static_cast<size_t>(y)];
    }
    return std::unordered_map<NodeHash, RRNodeId, NodeHasher>();
}

std::unordered_map<NodeHash, RRNodeId, NodeHasher> NodeLookupManager::get_combined_nodes(Coordinate x, Coordinate y) const {
    std::unordered_map<NodeHash, RRNodeId, NodeHasher> combined;

    // Add column nodes
    const auto& col_nodes = get_column_nodes(x);
    combined.insert(col_nodes.begin(), col_nodes.end());

    // Add row nodes
    const auto& row_nodes = get_row_nodes(y);
    combined.insert(row_nodes.begin(), row_nodes.end());

    return combined;
}

bool NodeLookupManager::is_valid_coordinate(Coordinate x, Coordinate y) const {
    return x >= 0 && x <= fpga_grid_x_ && y >= 0 && y <= fpga_grid_y_;
}

void NodeLookupManager::print_statistics() const {
    VTR_LOG("=== Node Lookup Manager Statistics ===\n");
    VTR_LOG("Grid dimensions: %d x %d\n", fpga_grid_x_, fpga_grid_y_);

    // Count nodes per column
    std::vector<size_t> col_counts;
    for (const auto& col_map : column_lookup_) {
        col_counts.push_back(col_map.size());
    }

    if (!col_counts.empty()) {
        auto [min_col, max_col] =
            std::minmax_element(col_counts.begin(), col_counts.end());
        VTR_LOG("Nodes per column: min=%d, max=%d, avg=%f\n", *min_col, *max_col,
                std::accumulate(col_counts.begin(), col_counts.end(), 0.0) / col_counts.size());
    }

    // Count nodes per row
    std::vector<size_t> row_counts;
    for (const auto& row_map : row_lookup_) {
        row_counts.push_back(row_map.size());
    }

    if (!row_counts.empty()) {
        auto [min_row, max_row] =
            std::minmax_element(row_counts.begin(), row_counts.end());
        VTR_LOG("Nodes per row: min=%d, max=%d, avg=%f\n", *min_row, *max_row,
                *max_row,
                std::accumulate(row_counts.begin(), row_counts.end(), 0.0) / row_counts.size());
    }
}

void NodeLookupManager::clear() {
    column_lookup_.clear();
    row_lookup_.clear();
}

NodeHash NodeLookupManager::build_node_hash(RRNodeId node_id) const {
    const std::string& node_ptcs = rr_graph_.rr_nodes().node_ptc_nums_to_string(node_id);
    return std::make_tuple(rr_graph_.node_type(node_id), node_ptcs,
                           rr_graph_.node_xlow(node_id), rr_graph_.node_xhigh(node_id),
                           rr_graph_.node_ylow(node_id), rr_graph_.node_yhigh(node_id));
}

void NodeLookupManager::index_node(RRNodeId node_id) {
    NodeHash hash = build_node_hash(node);
    const RRNode* node_ptr = &node;

    const Location& loc = node.get_location();

    VTR_ASSERT(loc.x_low <= fpga_grid_x_);
    VTR_ASSERT(loc.x_high <= fpga_grid_x_);
    VTR_ASSERT(loc.y_low <= fpga_grid_y_);
    VTR_ASSERT(loc.y_high <= fpga_grid_y_);

    // Skip spatial indexing for source/sink nodes
    if (node.get_type() == NodeType::SOURCE || node.get_type() == NodeType::SINK) {
        return;
    }

    // Add to column lookup (for single-column nodes)
    if (loc.x_low == loc.x_high) {
        column_lookup_[static_cast<size_t>(loc.x_low)][hash] = RRNodeId(node_ptr->get_id());
    }

    // Add to row lookup (for single-row nodes)
    if (loc.y_low == loc.y_high) {
        row_lookup_[static_cast<size_t>(loc.y_low)][hash] = RRNodeId(node_ptr->get_id());
    }
}

} // namespace crrgenerator
