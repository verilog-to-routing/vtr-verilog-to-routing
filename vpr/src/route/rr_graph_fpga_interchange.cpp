#include "rr_graph_fpga_interchange.h"

#include "arch_util.h"
#include "vtr_time.h"
#include "globals.h"

void build_rr_graph_fpga_interchange(const t_graph_type graph_type,
                                     const DeviceGrid& grid,
                                     const std::vector<t_segment_inf>& segment_inf,
                                     const enum e_base_cost_type base_cost_type,
                                     int* wire_to_rr_ipin_switch,
                                     const char* read_rr_graph_name,
                                     bool read_edge_metadata,
                                     bool do_check_rr_graph) {
    vtr::ScopedStartFinishTimer timer("Building RR Graph from device database");

    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.rr_segments = segment_inf;

    VTR_LOG("%s\n", get_arch_file_name());
};
