#include "crr_generator.h"

#include <chrono>

#include "globals.h"
#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

CRRGraphGenerator::CRRGraphGenerator(const t_crr_opts& crr_opts,
                                     const RRGraph& input_graph,
                                     const NodeLookupManager& node_lookup,
                                     const SwitchBlockManager& sb_manager,
                                     const std::string& output_graph_xml)
      : crr_opts_(crr_opts),
        input_graph_(input_graph),
        node_lookup_(node_lookup),
        sb_manager_(sb_manager),
        output_graph_xml_(output_graph_xml) {}

void CRRGraphGenerator::run() {
    auto start_time = std::chrono::steady_clock::now();

    VTR_LOG("Starting RR Graph parsing process\n");

    try {
        // Initialize all components
        initialize_components();

        // Build connections
        build_connections();

        // Create output graph
        create_output_graph();

        // Write output files
        write_output_files();

        // Print summary
        print_processing_summary();

        auto end_time = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

        VTR_LOG("RR Graph parsing completed successfully in %ds\n", duration.count());
    } catch (const std::exception& e) {
        VTR_LOG_ERROR("RR Graph parsing failed: %s\n", e.what());
    }
}

void CRRGraphGenerator::initialize_components() {
    VTR_LOG("CRR Graph Generator: Initializing components\n");

    // Initialize XML handler
    xml_handler_ = std::make_unique<XMLHandler>();

    // Initialize connection builder
    connection_builder_ = std::make_unique<CRRConnectionBuilder>(input_graph_, node_lookup_, sb_manager_);

    // Initialize thread pool if parallel processing is enabled
    VTR_ASSERT(crr_opts_.crr_num_threads > 0);
    thread_pool_ = std::make_unique<CRRThreadPool>(crr_opts_.crr_num_threads);
    VTR_LOG("CRR Graph Generator: Parallel processing enabled with %lu threads\n",
                thread_pool_->get_thread_count());

    VTR_LOG("CRR Graph Generator: All components initialized successfully\n");
}

void CRRGraphGenerator::build_connections() {
  VTR_LOG("CRR Graph Generator: Building connections\n");

  // Initialize connection builder
  const DeviceGrid& grid = g_vpr_ctx.device().grid;
  connection_builder_->initialize(grid.width(),
                                  grid.height(),
                                  crr_opts_.annotated_rr_graph);

  VTR_LOG("CRR Graph Generator: Connection building completed\n");
}

void CRRGraphGenerator::create_output_graph() {
  VTR_LOG("CRR Graph Generator: Creating output graph\n");

  // Create new graph based on input graph
  output_graph_ = std::make_unique<RRGraph>(input_graph_);

  // Add new switches to output graph
  if (crr_opts_.annotated_rr_graph) {
    size_t nxt_switch_id =
        connection_builder_->get_default_swithes_map().size();
    int max_sw_delay_ps = sb_manager_.get_max_switch_delay_ps();
    std::string default_sw_type = "";
    Timing default_timing;
    Sizing default_sizing;

    // Get default timing and sizing from input graph
    for (const auto& graph_sw : input_graph_.get_switches()) {
      std::string sw_name = graph_sw.get_name();
      std::transform(sw_name.begin(), sw_name.end(), sw_name.begin(),
                    ::tolower);
      if (sw_name.find("l1") != std::string::npos ||
          sw_name.find("l4") != std::string::npos) {
        default_sw_type = graph_sw.get_type();
        default_timing = graph_sw.get_timing();
        default_sizing = graph_sw.get_sizing();
      }
    }

    // Add sw_zero switch - we don't use delayless switch because OpenFPGA doesn't work with it.
    Switch sw_zero(static_cast<SwitchId>(nxt_switch_id), "sw_zero", default_sw_type, Timing(0, 0), default_sizing);
    output_graph_->add_switch(sw_zero);

    // Add new switches to output graph
    for (int curr_sw_delay_ps = static_cast<int>(nxt_switch_id) + 1;
        curr_sw_delay_ps <= max_sw_delay_ps; curr_sw_delay_ps++) {
      float curr_sw_delay_s = static_cast<float>(curr_sw_delay_ps) * 1e-12f;
      Switch curr_switch(
          static_cast<SwitchId>(curr_sw_delay_ps),
          "sw_" + std::to_string(curr_sw_delay_ps), default_sw_type,
          Timing(default_timing.Cin, curr_sw_delay_s), default_sizing);
      output_graph_->add_switch(curr_switch);
    }
  }

  // Get all connections from connection builder
  auto all_connections = connection_builder_->get_all_connections();

  auto preserved_edges = input_graph_.get_preserved_edges(
      crr_opts_.preserve_input_pins,
      crr_opts_.preserve_output_pins);

  VTR_LOG("CRR Graph Generator: Number of preserved edges: %zu\n", preserved_edges.size());

  // Clear existing edges and add new ones
  output_graph_->get_edges().clear();

  // Add preserved edges
  for (const auto& edge : preserved_edges) {
    output_graph_->add_edge(edge);
  }

  add_custom_edges();

  output_graph_->sort_nodes();
  output_graph_->sort_edges();
  output_graph_->shrink_to_fit();

  VTR_LOG("CRR Graph Generator: Output graph created with %zu edges\n",
           output_graph_->get_edge_count());
}

void CRRGraphGenerator::add_custom_edges() {
  VTR_LOG("CRR Graph Generator: Adding custom edges\n");

  // Add new connections
  const DeviceGrid& grid = g_vpr_ctx.device().grid;
  size_t total_tiles =
      static_cast<size_t>((grid.width() + 1) * (grid.height() + 1));
  ProgressTracker tracker(total_tiles, "Adding new connections");
  std::vector<std::future<void>> futures;
  std::mutex graph_mutex;

  for (Coordinate tile_x = 0; tile_x <= grid.width(); tile_x++) {
    for (Coordinate tile_y = 0; tile_y <= grid.height(); tile_y++) {
      auto fut = thread_pool_->submit([this, tile_x, tile_y, &graph_mutex, &tracker]() {
        std::vector<Connection> tile_connections =
            connection_builder_->get_tile_connections(tile_x, tile_y);
        
        {
          std::lock_guard<std::mutex> lock(graph_mutex);
          for (const auto& connection : tile_connections) {
            output_graph_->add_edge(connection.src_node(),
                                    connection.sink_node(),
                                    connection.switch_id());
          }
        }
        
        tracker.increment();
      });
      futures.push_back(std::move(fut));
    }
  }

  // Wait for all tasks to complete
  for (auto& fut : futures) {
    fut.get();
  }
}

void CRRGraphGenerator::write_output_files() {
  if (!output_graph_xml_.empty()) {
    VTR_LOG("CRR Graph Generator: Writing output files\n");
    xml_handler_->write_rr_graph(output_graph_xml_, *output_graph_);
    VTR_LOG("CRR Graph Generator: Output files written successfully\n");
  } else {
    VTR_LOG("CRR Graph Generator: No output file specified\n");
  }
}

void CRRGraphGenerator::print_processing_summary() {
  VTR_LOG("CRR Graph Generator: === Processing Summary ===\n");

  VTR_LOG("CRR Graph Generator: Input graph:\n");
  VTR_LOG("  Nodes: %zu\n", input_graph_.get_node_count());
  VTR_LOG("  Edges: %zu\n", input_graph_.get_edge_count());
  VTR_LOG("  Switches: %zu\n", input_graph_.get_switch_count());

  VTR_LOG("CRR Graph Generator: Output graph:\n");
  VTR_LOG("  Nodes: %zu\n", output_graph_->get_node_count());
  VTR_LOG("  Edges: %zu\n", output_graph_->get_edge_count());
  VTR_LOG("  Switches: %zu\n", output_graph_->get_switch_count());

  int new_edges =
      static_cast<int>(output_graph_->get_edge_count()) -
      static_cast<int>(input_graph_.get_edge_count());
  VTR_LOG("  New edges added: %d\n", new_edges);

  VTR_LOG("Switch blocks processed: %zu\n", sb_manager_.get_all_patterns().size());
  VTR_LOG("Total connections: %zu\n", sb_manager_.get_total_connections());

  VTR_LOG("Grid locations processed: %dx%d\n", g_vpr_ctx.device().grid.width(), g_vpr_ctx.device().grid.height());

}

void CRRGraphGenerator::validate_processing_results() {
  // Basic validation
  if (!output_graph_) {
    throw RRGeneratorException("Output graph was not created");
  }

  if (output_graph_->get_node_count() == 0) {
    throw RRGeneratorException("Output graph has no nodes");
  }

  if (output_graph_->get_edge_count() == 0) {
    throw RRGeneratorException("Output graph has no edges");
  }

  if (output_graph_->get_switch_count() == 0) {
    throw RRGeneratorException("Output graph has no switches");
  }

  VTR_LOG("Processing results validation passed\n");
}

}  // namespace crrgenerator