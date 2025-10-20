#pragma once

#include <chrono>
#include <memory>

#include "vpr_types.h"
#include "crr_common.h"
#include "crr_connection_builder.h"
#include "node_lookup_manager.h"
#include "custom_rr_graph.h"
#include "switch_block_manager.h"
#include "switch_manager.h"
#include "crr_thread_pool.h"
#include "xml_handler.h"

namespace crrgenerator {

/**
 * @brief Main orchestrator for RR graph parsing and generation
 *
 * This class coordinates all subsystems to read input graphs,
 * process switch block configurations, and generate output graphs.
 */
class CRRGraphGenerator {
public:
    CRRGraphGenerator(const t_crr_opts& crr_opts,
                      const RRGraph& input_graph,
                      const NodeLookupManager& node_lookup,
                      const SwitchBlockManager& sb_manager,
                      const std::string& output_graph_xml);
    ~CRRGraphGenerator() = default;

    /**
     * @brief Run the complete parsing and generation process
     */
    void run();

private:
    const t_crr_opts& crr_opts_;
    const RRGraph& input_graph_;
    const NodeLookupManager& node_lookup_;
    const SwitchBlockManager& sb_manager_;
    const std::string& output_graph_xml_;

    std::unique_ptr<RRGraph> output_graph_;
    std::unique_ptr<ConnectionBuilder> connection_builder_;
    std::unique_ptr<SwitchManager> switch_manager_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<XMLHandler> xml_handler_;

    // Processing methods
    void initialize_components();
    void process_switch_blocks();
    void build_connections();
    void create_output_graph();
    void add_custom_edges();
    void write_output_files();

    // Utility methods
    void print_processing_summary();
    void validate_processing_results();
};

}  // namespace crrgenerator