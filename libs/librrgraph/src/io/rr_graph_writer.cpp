/*
 * This file defines the writing rr graph function in XML format.
 * The rr graph is separated into channels, nodes, switches,
 * grids, edges, block types, and segments. Each tag has several
 * children tags such as timing, location, or some general
 * details. Each tag has attributes to describe them */

#include "rr_graph_writer.h"

#include <cstdio>
#include <fstream>
#include <limits>
#include "rr_graph_uxsdcxx_serializer.h"
#include "rr_graph_uxsdcxx.h"
#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "ndmatrix_serdes.h"
#    include "serdes_utils.h"
#    include "rr_graph_uxsdcxx_capnp.h"
#endif

/************************ Subroutine definitions ****************************/

/* This function is used to write the rr_graph into xml format into a a file with name: file_name */

/**FIXME: To make rr_graph_reader independent of vpr_context, the below
 * parameters are a workaround to passing the data structures of DeviceContext. 
 * Needs a solution to reduce the number of parameters passed in.*/

void write_rr_graph(RRGraphBuilder* rr_graph_builder,
                    RRGraphView* rr_graph_view,
                    const std::vector<t_physical_tile_type>& physical_tile_types,
                    vtr::vector<RRIndexedDataId, t_rr_indexed_data>* rr_indexed_data,
                    std::vector<t_rr_rc_data>* rr_rc_data,
                    const DeviceGrid& grid,
                    const std::vector<t_arch_switch_inf>& arch_switch_inf,
                    const t_arch* arch,
                    t_chan_width* chan_width,
                    const char* file_name,
                    const int virtual_clock_network_root_idx,
                    bool echo_enabled,
                    const char* echo_file_name,
                    bool is_flat) {

    RrGraphSerializer reader(
        /*graph_type=*/t_graph_type(),
        /*base_cost_type=*/e_base_cost_type(),
        /*wire_to_rr_ipin_switch=*/nullptr,
        /*wire_to_rr_ipin_switch_between_dice=*/nullptr,
        /*do_check_rr_graph=*/false,
        /*read_rr_graph_name=*/nullptr,
        /*read_rr_graph_filename=*/nullptr,
        /*read_edge_metadata=*/false,
        echo_enabled,
        echo_file_name,
        chan_width,
        &rr_graph_builder->rr_nodes(),
        rr_graph_builder,
        rr_graph_view,
        &rr_graph_builder->rr_switch(),
        rr_indexed_data,
        rr_rc_data,
        virtual_clock_network_root_idx,
        arch_switch_inf,
        rr_graph_view->rr_segments(),
        physical_tile_types,
        grid,
        &rr_graph_builder->rr_node_metadata(),
        &rr_graph_builder->rr_edge_metadata(),
        &arch->strings,
        is_flat);

    if (vtr::check_file_name_extension(file_name, ".xml")) {
        std::fstream fp;
        fp.open(file_name, std::fstream::out | std::fstream::trunc);
        fp.precision(std::numeric_limits<float>::max_digits10);
        void* context;
        uxsd::write_rr_graph_xml(reader, context, fp);
#ifdef VTR_ENABLE_CAPNPROTO
    } else if (vtr::check_file_name_extension(file_name, ".bin")) {
        ::capnp::MallocMessageBuilder builder;
        auto rr_graph = builder.initRoot<ucap::RrGraph>();
        void* context;
        uxsd::write_rr_graph_capnp(reader, context, rr_graph);
        writeMessageToFile(file_name, &builder);
#endif
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Unknown extension on output %s",
                        file_name);
    }
}
