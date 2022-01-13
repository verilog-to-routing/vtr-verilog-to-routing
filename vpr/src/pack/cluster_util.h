#include "globals.h"
#include "atom_netlist.h"
#include "pack_types.h"
#include "echo_files.h"

#include "timing_info.h"
#include "PreClusterDelayCalculator.h"
#include "PreClusterTimingGraphResolver.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

//calculate the initial timing at the start of packing stage
void calc_init_packing_timing (const t_packer_opts& packer_opts, 
							   const t_analysis_opts& analysis_opts,
							   const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode, 
							   std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc, 
							   std::shared_ptr<SetupTimingInfo>& timing_info, 
							   vtr::vector<AtomBlockId, float>& atom_criticality);

