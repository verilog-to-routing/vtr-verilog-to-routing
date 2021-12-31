#ifndef SHOWSETUP_H
#define SHOWSETUP_H

void ShowSetup(const t_vpr_setup& vpr_setup);
void printClusteredNetlistStats(std::string block_usage_filename);
void writeClusteredNetlistStats(std::string block_usage_filename,
                                int num_nets,
                                int num_blocks,
                                int L_num_p_inputs,
                                int L_num_p_outputs,
                                std::vector<int> num_blocks_type,
                                std::vector<t_logical_block_type> logical_block_types);

#endif
