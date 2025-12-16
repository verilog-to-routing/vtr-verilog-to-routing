#include "node_lookup_manager.h"

#include "rr_graph_utils.h"

#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_time.h"

namespace crrgenerator {

NodeLookupManager::NodeLookupManager(const RRGraphView& rr_graph, size_t fpga_grid_x, size_t fpga_grid_y)
    : rr_graph_(rr_graph)
    , fpga_grid_x_(fpga_grid_x)
    , fpga_grid_y_(fpga_grid_y) {
    vtr::ScopedStartFinishTimer timer("Initialize NodeLookupManager");

    // Make sure lookup is not initialized
    VTR_ASSERT(column_lookup_.empty() && row_lookup_.empty());

    // Initialize lookup structures
    column_lookup_.resize(fpga_grid_x_ + 1);
    row_lookup_.resize(fpga_grid_y_ + 1);

    // Index all nodes
    for (RRNodeId node_id : rr_graph_.nodes()) {
        index_node(node_id);
    }

    VTR_LOG("Node lookup manager initialized successfully\n");
    print_statistics();
}

const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& NodeLookupManager::get_column_nodes(size_t x) const {
    VTR_ASSERT(x <= fpga_grid_x_ && x < column_lookup_.size());
    return column_lookup_[x];
}

const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& NodeLookupManager::get_row_nodes(size_t y) const {
    VTR_ASSERT(y <= fpga_grid_y_ && y < row_lookup_.size());
    return row_lookup_[y];
}

std::unordered_map<NodeHash, RRNodeId, NodeHasher> NodeLookupManager::get_combined_nodes(size_t x, size_t y) const {
    std::unordered_map<NodeHash, RRNodeId, NodeHasher> combined;

    // Add column nodes
    const auto& col_nodes = get_column_nodes(x);
    combined.insert(col_nodes.begin(), col_nodes.end());

    // Add row nodes
    const auto& row_nodes = get_row_nodes(y);
    combined.insert(row_nodes.begin(), row_nodes.end());

    return combined;
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
    const std::string& node_ptcs = node_ptc_number_to_string(rr_graph_, node_id);
    return std::make_tuple(rr_graph_.node_type(node_id),
                           node_ptcs,
                           rr_graph_.node_xlow(node_id), rr_graph_.node_xhigh(node_id),
                           rr_graph_.node_ylow(node_id), rr_graph_.node_yhigh(node_id));
}

void NodeLookupManager::index_node(RRNodeId node_id) {
    NodeHash hash = build_node_hash(node_id);

    short x_low = rr_graph_.node_xlow(node_id);
    short x_high = rr_graph_.node_xhigh(node_id);
    short y_low = rr_graph_.node_ylow(node_id);
    short y_high = rr_graph_.node_yhigh(node_id);

    VTR_ASSERT(static_cast<size_t>(x_low) <= fpga_grid_x_);
    VTR_ASSERT(static_cast<size_t>(x_high) <= fpga_grid_x_);
    VTR_ASSERT(static_cast<size_t>(y_low) <= fpga_grid_y_);
    VTR_ASSERT(static_cast<size_t>(y_high) <= fpga_grid_y_);

    // Skip spatial indexing for source/sink nodes
    if (rr_graph_.node_type(node_id) == e_rr_type::SOURCE || rr_graph_.node_type(node_id) == e_rr_type::SINK) {
        return;
    }

    // Add to column lookup (for single-column nodes)
    if (x_low == x_high) {
        column_lookup_[static_cast<size_t>(x_low)][hash] = node_id;
    }

    // Add to row lookup (for single-row nodes)
    if (y_low == y_high) {
        row_lookup_[static_cast<size_t>(y_low)][hash] = node_id;
    }
}

} // namespace crrgenerator
