#include "crr_generator.h"

#include <chrono>

#include "globals.h"
#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

CRRGraphGenerator::CRRGraphGenerator(const t_crr_opts& crr_opts,
                                     const RRGraphView& input_graph,
                                     const NodeLookupManager& node_lookup,
                                     const SwitchBlockManager& sb_manager,
                                     const std::string& output_graph_xml)
    : crr_opts_(crr_opts)
    , input_graph_(input_graph)
    , node_lookup_(node_lookup)
    , sb_manager_(sb_manager)
    , output_graph_xml_(output_graph_xml) {}

void CRRGraphGenerator::run() {
    auto start_time = std::chrono::steady_clock::now();

    VTR_LOG("Starting RR Graph parsing process\n");

    try {
        // Initialize all components
        initialize_components();

        // Build connections
        build_connections();

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
    connection_builder_ = std::make_unique<CRRConnectionBuilder>(input_graph_,
                                                                 node_lookup_,
                                                                 sb_manager_);

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
    connection_builder_->initialize(grid.width() - 2,
                                    grid.height() - 2,
                                    crr_opts_.annotated_rr_graph);

    VTR_LOG("CRR Graph Generator: Connection building completed\n");
}

} // namespace crrgenerator
