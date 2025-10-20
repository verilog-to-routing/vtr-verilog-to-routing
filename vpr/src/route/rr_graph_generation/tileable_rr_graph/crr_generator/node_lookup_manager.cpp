#include "node_lookup_manager.h"

#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

NodeLookupManager::NodeLookupManager() : fpga_grid_x_(0), fpga_grid_y_(0) {}

void NodeLookupManager::initialize(const RRGraph& graph, Coordinate fpga_grid_x,
                                   Coordinate fpga_grid_y) {
  VTR_LOG("Initializing NodeLookupManager for %d x %d grid with %zu nodes\n",
           fpga_grid_x, fpga_grid_y, static_cast<size_t>(graph.get_node_count()));

  fpga_grid_x_ = fpga_grid_x;
  fpga_grid_y_ = fpga_grid_y;

  // Clear existing data
  clear();

  // Initialize lookup structures
  column_lookup_.resize(static_cast<size_t>(fpga_grid_x) + 1);
  row_lookup_.resize(static_cast<size_t>(fpga_grid_y) + 1);
  for (int i = 0; i <= fpga_grid_x + 1; i++) {
    edge_sink_lookup_.push_back(std::vector<std::vector<const RREdge*>>(
        static_cast<size_t>(fpga_grid_y + 2), std::vector<const RREdge*>()));
  }

  // Index all nodes
  for (const auto& node : graph.get_nodes()) {
    index_node(node);
  }

  // Index all edges
  for (const auto& edge : graph.get_edges()) {
    index_edge(graph, edge);
  }

  VTR_LOG("Node lookup manager initialized successfully\n");
  print_statistics();
}

const RRNode* NodeLookupManager::get_node_by_hash(const NodeHash& hash) const {
  auto it = global_lookup_.find(hash);
  return (it != global_lookup_.end()) ? it->second : nullptr;
}

const std::unordered_map<NodeHash, const RRNode*, NodeHasher>&
NodeLookupManager::get_column_nodes(Coordinate x) const {
  if (x <= fpga_grid_x_ && x < static_cast<Coordinate>(column_lookup_.size())) {
    return column_lookup_[static_cast<size_t>(x)];
  }
  return empty_map_;
}

const std::unordered_map<NodeHash, const RRNode*, NodeHasher>&
NodeLookupManager::get_row_nodes(Coordinate y) const {
  if (y <= fpga_grid_y_ && y < static_cast<Coordinate>(row_lookup_.size())) {
    return row_lookup_[static_cast<size_t>(y)];
  }
  return empty_map_;
}

std::unordered_map<NodeHash, const RRNode*, NodeHasher>
NodeLookupManager::get_combined_nodes(Coordinate x, Coordinate y) const {
  std::unordered_map<NodeHash, const RRNode*, NodeHasher> combined;

  // Add column nodes
  const auto& col_nodes = get_column_nodes(x);
  combined.insert(col_nodes.begin(), col_nodes.end());

  // Add row nodes
  const auto& row_nodes = get_row_nodes(y);
  combined.insert(row_nodes.begin(), row_nodes.end());

  return combined;
}

std::vector<const RRNode*> NodeLookupManager::get_nodes_by_type(
    NodeType type) const {
  auto it = type_lookup_.find(type);
  return (it != type_lookup_.end()) ? it->second : std::vector<const RRNode*>{};
}

const std::vector<const RREdge*> NodeLookupManager::get_sink_edges_at_location(Coordinate x, Coordinate y) const {
  return edge_sink_lookup_[static_cast<size_t>(x)][static_cast<size_t>(y)];
}

bool NodeLookupManager::is_valid_coordinate(Coordinate x, Coordinate y) const {
  return x >= 0 && x <= fpga_grid_x_ && y >= 0 && y <= fpga_grid_y_;
}

void NodeLookupManager::print_statistics() const {
  VTR_LOG("=== Node Lookup Manager Statistics ===");
  VTR_LOG("Grid dimensions: %d x %d\n", fpga_grid_x_, fpga_grid_y_);
  VTR_LOG("Total nodes indexed: %zu\n", global_lookup_.size());

  // Count nodes by type
  for (const auto& [type, nodes] : type_lookup_) {
    VTR_LOG("  %s nodes: %zu\n", to_string(type).c_str(), nodes.size());
  }

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
  global_lookup_.clear();
  type_lookup_.clear();
}

NodeHash NodeLookupManager::build_node_hash(const RRNode& node) const {
  return std::make_tuple(node.get_type(), node.get_ptc(),
                         node.get_location().x_low, node.get_location().x_high,
                         node.get_location().y_low, node.get_location().y_high);
}

void NodeLookupManager::index_node(const RRNode& node) {
  NodeHash hash = build_node_hash(node);
  const RRNode* node_ptr = &node;

  // Add to global lookup
  global_lookup_[hash] = node_ptr;

  // Add to type lookup
  type_lookup_[node.get_type()].push_back(node_ptr);

  const Location& loc = node.get_location();

  VTR_ASSERT(loc.x_low <= fpga_grid_x_);
  VTR_ASSERT(loc.x_high <= fpga_grid_x_);
  VTR_ASSERT(loc.y_low <= fpga_grid_y_);
  VTR_ASSERT(loc.y_high <= fpga_grid_y_);

  // Skip spatial indexing for source/sink nodes
  if (node.get_type() == NodeType::SOURCE ||
      node.get_type() == NodeType::SINK) {
    return;
  }

  // Add to column lookup (for single-column nodes)
  if (loc.x_low == loc.x_high) {
    column_lookup_[static_cast<size_t>(loc.x_low)][hash] = node_ptr;
  }

  // Add to row lookup (for single-row nodes)
  if (loc.y_low == loc.y_high) {
    row_lookup_[static_cast<size_t>(loc.y_low)][hash] = node_ptr;
  }
}

void NodeLookupManager::index_edge(const RRGraph& graph, const RREdge& edge) {
  const RRNode* sink = graph.get_node(edge.get_sink_node());

  VTR_ASSERT(sink->get_location().x_high >= 0);
  VTR_ASSERT(sink->get_location().y_high >= 0);

  VTR_ASSERT(sink->get_location().x_high <= fpga_grid_x_);
  VTR_ASSERT(sink->get_location().y_high <= fpga_grid_y_);

  // Add to edge lookup
  edge_sink_lookup_[static_cast<size_t>(sink->get_location().x_low)]
                   [static_cast<size_t>(sink->get_location().y_low)].push_back(&edge);
}


}  // namespace crrgenerator
