/*This function loads in a routing resource graph written in xml format
 * into vpr when the option --read_rr_graph <file name> is specified.
 * When it is not specified the build_rr_graph function is then called.
 * This is done using the libpugixml library. This is useful
 * when specific routing resources should remain constant or when
 * some information left out in the architecture can be specified
 * in the routing resource graph. The routing resource graph file is
 * contained in a <rr_graph> tag. Its subtags describe the rr graph's
 * various features such as nodes, edges, and segments. Information such
 * as blocks, grids, and segments are verified with the architecture
 * to ensure it matches. An error will through if any feature does not match.
 * Other elements such as edges, nodes, and switches
 * are overwritten by the rr graph file if one is specified. If an optional
 * identifier such as capacitance is not specified, it is set to 0*/

#include "rr_graph_reader.h"

#include "rr_graph_uxsdcxx_serializer.h"
#include "rr_graph_uxsdcxx.h"

#include <fstream>

#include "vtr_time.h"
#include "globals.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "rr_graph_uxsdcxx_capnp.h"
#    include "mmap_file.h"
#endif

/************************ Subroutine definitions ****************************/

/*loads the given RR_graph file into the appropriate data structures
 * as specified by read_rr_graph_name. Set up correct routing data
 * structures as well*/
void load_rr_file(const t_graph_type graph_type,
                  const DeviceGrid& grid,
                  const std::vector<t_segment_inf>& segment_inf,
                  const enum e_base_cost_type base_cost_type,
                  int* wire_to_rr_ipin_switch,
                  const char* read_rr_graph_name,
                  bool read_edge_metadata,
                  bool do_check_rr_graph) {
    vtr::ScopedStartFinishTimer timer("Loading routing resource graph");

    auto& device_ctx = g_vpr_ctx.mutable_device();

    device_ctx.rr_segments = segment_inf;

    RrGraphSerializer reader(
        graph_type,
        base_cost_type,
        wire_to_rr_ipin_switch,
        do_check_rr_graph,
        read_rr_graph_name,
        &device_ctx.read_rr_graph_filename,
        read_edge_metadata,
        &device_ctx.chan_width,
        &device_ctx.rr_nodes,
        &device_ctx.rr_switch_inf,
        &device_ctx.rr_indexed_data,
        &device_ctx.rr_node_indices,
        &device_ctx.connection_boxes,
        device_ctx.num_arch_switches,
        device_ctx.arch_switch_inf,
        device_ctx.rr_segments,
        device_ctx.physical_tile_types,
        grid,
        &device_ctx.rr_node_metadata,
        &device_ctx.rr_edge_metadata,
        &device_ctx.arch->strings);

    if (vtr::check_file_name_extension(read_rr_graph_name, ".xml")) {
        try {
            std::ifstream file(read_rr_graph_name);
            void* context;
            uxsd::load_rr_graph_xml(reader, context, read_rr_graph_name, file);
        } catch (pugiutil::XmlError& e) {
            vpr_throw(VPR_ERROR_ROUTE, read_rr_graph_name, e.line(), "%s", e.what());
        }
#ifdef VTR_ENABLE_CAPNPROTO
    } else if (vtr::check_file_name_extension(read_rr_graph_name, ".bin")) {
        MmapFile f(read_rr_graph_name);
        void* context;
        uxsd::load_rr_graph_capnp(reader, f.getData(), context, read_rr_graph_name);
#endif
    } else {
        VTR_LOG_WARN(
            "RR graph file '%s' may be in incorrect format. "
            "Expecting .xml or .bin format\n",
            read_rr_graph_name);
    }
}
