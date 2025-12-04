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
#    include <capnp/schema.h>
#    include "serdes_utils.h"
#    include "rr_graph_uxsdcxx_capnp.h"
#endif

/************************ Subroutine definitions ****************************/

/* This function is used to write the rr_graph into xml format into a file with name: file_name */

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
                    bool echo_enabled,
                    const char* echo_file_name,
                    const int route_verbosity,
                    bool is_flat) {

    // If Cap'n Proto is enabled, a unique ID is assigned to the schema used to serialize the RR graph.
    // This ID is used to verify that the schema used to serialize the RR graph matches the
    // schema being used to deserialize it.
    // If Cap'n Proto is not enabled, the schema ID is 0 and no schema ID check is performed.
    unsigned long schema_file_id = 0;
#ifdef VTR_ENABLE_CAPNPROTO
    ::capnp::Schema schema = ::capnp::Schema::from<ucap::RrGraph>();
    schema_file_id = schema.getProto().getScopeId();
    VTR_LOGV(route_verbosity > 1, "Schema file ID: 0x%016lx\n", schema_file_id);
#endif

    RrGraphSerializer reader(
        /*graph_type=*/e_graph_type(),
        /*base_cost_type=*/e_base_cost_type(),
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
        arch_switch_inf,
        rr_graph_view->rr_segments(),
        physical_tile_types,
        grid,
        &rr_graph_builder->rr_node_metadata(),
        &rr_graph_builder->rr_edge_metadata(),
        &arch->strings,
        schema_file_id,
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
