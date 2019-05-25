#ifndef SETUPVPR_H
#define SETUPVPR_H
#include <vector>
#include "logic_types.h"
#include "read_options.h"
#include "physical_types.h"
#include "vpr_types.h"

void SetupVPR(t_options* Options,
              const bool TimingEnabled,
              const bool readArchFile,
              t_file_name_opts* FileNameOpts,
              t_arch* Arch,
              t_model** user_models,
              t_model** library_models,
              t_netlist_opts* NetlistOpts,
              t_packer_opts* PackerOpts,
              t_placer_opts* PlacerOpts,
              t_annealing_sched* AnnealSched,
              t_router_opts* RouterOpts,
              t_analysis_opts* AnalysisOpts,
              t_det_routing_arch* RoutingArch,
              std::vector<t_lb_type_rr_node>** PackerRRGraphs,
              std::vector<t_segment_inf>& Segments,
              t_timing_inf* Timing,
              bool* ShowGraphics,
              int* GraphPause,
              t_power_opts* PowerOpts);
#endif
