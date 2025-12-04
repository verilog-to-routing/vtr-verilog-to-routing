#pragma once

#include <chrono>
#include <memory>

#include "vpr_types.h"
#include "crr_common.h"
#include "crr_connection_builder.h"
#include "node_lookup_manager.h"
#include "crr_switch_block_manager.h"

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
                      const RRGraphView& input_graph,
                      const NodeLookupManager& node_lookup,
                      const SwitchBlockManager& sb_manager);
    ~CRRGraphGenerator() = default;

    /**
     * @brief Run the complete parsing and generation process
     */
    void run();

    const CRRConnectionBuilder* get_connection_builder() const {
        return connection_builder_.get();
    }

  private:
    const t_crr_opts& crr_opts_;
    const RRGraphView& input_graph_;
    const NodeLookupManager& node_lookup_;
    const SwitchBlockManager& sb_manager_;

    std::unique_ptr<CRRConnectionBuilder> connection_builder_;

    // Processing methods
    void initialize_components();
    void process_switch_blocks();
    void build_connections();
};

} // namespace crrgenerator
