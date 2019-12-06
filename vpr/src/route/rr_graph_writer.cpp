/*
 * This file defines the writing rr graph function in XML format.
 * The rr graph is separated into channels, nodes, switches,
 * grids, edges, block types, and segments. Each tag has several
 * children tags such as timing, location, or some general
 * details. Each tag has attributes to describe them */

#include "rr_graph_writer.h"

#include <cstdio>
#include <fstream>
#include "rr_graph_uxsdcxx_interface_impl.h"
#include "rr_graph_uxsdcxx.h"
#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "ndmatrix_serdes.h"
#    include "serdes_utils.h"
#    include "rr_graph_uxsdcxx_capnp.h"
#endif

#include "globals.h"

/************************ Subroutine definitions ****************************/

/* This function is used to write the rr_graph into xml format into a a file with name: file_name */
void write_rr_graph(const char* file_name, const std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    RrGraphImpl reader(
        /*is_global_graph=*/false,
        /*read_edge_metadata=*/false,
        &device_ctx.chan_width,
        &device_ctx.rr_nodes,
        &device_ctx.rr_switch_inf,
        &device_ctx.rr_indexed_data,
        &device_ctx.rr_node_indices,
        device_ctx.num_arch_switches,
        device_ctx.arch_switch_inf,
        segment_inf,
        device_ctx.physical_tile_types,
        device_ctx.grid,
        device_ctx.rr_node_metadata,
        device_ctx.rr_edge_metadata);

    std::fstream fp;
    fp.open(file_name, std::fstream::out | std::fstream::trunc);
    if (vtr::check_file_name_extension(file_name, ".xml")) {
        uxsd::write_rr_graph_xml(reader, fp);
#ifdef VTR_ENABLE_CAPNPROTO
    } else if (vtr::check_file_name_extension(file_name, ".capnp")) {
        ::capnp::MallocMessageBuilder builder;
        auto rr_graph = builder.initRoot<ucap::RrGraph>();
        uxsd::write_rr_graph_capnp(reader, rr_graph);
        writeMessageToFile(file_name, &builder);
#endif
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Unknown extension on output %s",
                        file_name);
    }
}
