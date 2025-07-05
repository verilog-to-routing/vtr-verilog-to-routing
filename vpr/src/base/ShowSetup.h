#pragma once

#include <ostream>
#include <string>
#include <vector>
#include "vpr_types.h"

struct t_logical_block_type;
struct t_vpr_setup;

struct ClusteredNetlistStats {
  private:
    void writeHuman(std::ostream& output) const;
    void writeJSON(std::ostream& output) const;
    void writeXML(std::ostream& output) const;

  public:
    ClusteredNetlistStats();

    enum OutputFormat {
        HumanReadable,
        JSON,
        XML
    };

    int num_nets;
    int num_blocks;
    int L_num_p_inputs;
    int L_num_p_outputs;
    std::vector<int> num_blocks_type;
    std::vector<t_logical_block_type> logical_block_types;

    void write(OutputFormat fmt, std::ostream& output) const;
};

void ShowSetup(const t_vpr_setup& vpr_setup);

/**
 * @brief Converts the enum e_stage_action to a string.
 * @param action The stage action to convert.
 * @return A string representation of the stage action.
 */
std::string stage_action_to_string(e_stage_action action);

void writeClusteredNetlistStats(const std::string& block_usage_filename);
