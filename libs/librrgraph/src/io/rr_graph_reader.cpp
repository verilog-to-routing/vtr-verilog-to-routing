/* This function loads in a routing resource graph written in xml format
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
 * identifier such as capacitance is not specified, it is set to 0 */

#include "rr_graph_reader.h"

#include "rr_graph_uxsdcxx_serializer.h"
#include "rr_graph_uxsdcxx.h"

#include <fstream>

#include "vtr_time.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "rr_graph_uxsdcxx_capnp.h"
#    include "mmap_file.h"
#endif

/************************ Subroutine definitions ****************************/
/* loads the given RR_graph file into the appropriate data structures
 * as specified by read_rr_graph_name. Set up correct routing data
 * structures as well*/

/**FIXME: To make rr_graph_reader independent of vpr_context, the below
 * parameters are a workaround to passing the data structures of DeviceContext. 
 * Needs a solution to reduce the number of parameters passed in.*/

void load_rr_file(RRGraphBuilder* rr_graph_builder,
                  RRGraphView* rr_graph,
                  const std::vector<t_physical_tile_type>& physical_tile_types,
                  const std::vector<t_segment_inf>& segment_inf,
                  vtr::vector<RRIndexedDataId, t_rr_indexed_data>* rr_indexed_data,
                  std::vector<t_rr_rc_data>* rr_rc_data,
                  const DeviceGrid& grid,
                  const std::vector<t_arch_switch_inf>& arch_switch_inf,
                  const t_graph_type graph_type,
                  const t_arch* arch,
                  t_chan_width* chan_width,
                  const enum e_base_cost_type base_cost_type,
                  int* wire_to_rr_ipin_switch,
                  int* wire_to_rr_ipin_switch_between_dice,
                  const char* read_rr_graph_name,
                  std::string* read_rr_graph_filename,
                  bool read_edge_metadata,
                  bool do_check_rr_graph,
                  bool echo_enabled,
                  const char* echo_file_name,
                  bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Loading routing resource graph");

    size_t num_segments = segment_inf.size();
    rr_graph_builder->reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        rr_graph_builder->add_rr_segment(segment_inf[(iseg)]);
    }

    RrGraphSerializer reader(
        graph_type,
        base_cost_type,
        wire_to_rr_ipin_switch,
        wire_to_rr_ipin_switch_between_dice,
        do_check_rr_graph,
        read_rr_graph_name,
        read_rr_graph_filename,
        read_edge_metadata,
        echo_enabled,
        echo_file_name,
        chan_width,
        &rr_graph_builder->rr_nodes(),
        rr_graph_builder,
        rr_graph,
        &rr_graph_builder->rr_switch(),
        &rr_graph_builder->rr_switch_offset_inf(),
        rr_indexed_data,
        rr_rc_data,
        arch_switch_inf,
        rr_graph->rr_segments(),
        physical_tile_types,
        grid,
        &rr_graph_builder->rr_node_metadata(),
        &rr_graph_builder->rr_edge_metadata(),
        &arch->strings,
        is_flat);

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

void load_rr_edge_attribute_offset_file(RRGraphBuilder& rr_graph_builder,
                                        const RRGraphView& rr_graph,
                                        std::string_view rr_edge_attribute_offset_filename) {
    std::ifstream file(rr_edge_attribute_offset_filename.data());

    if (!file) {
        VTR_LOG_ERROR("Failed to open RR edge offset file: %s ", rr_edge_attribute_offset_filename.data());
    }

    std::map<t_rr_switch_override_inf, RRSwitchOffsetInfoId> unique_edge_offsets;
    std::string line;
    bool first_line = true;
    while (std::getline(file, line)) {
        // Ignore first line if it starts with #
        if (first_line && !line.empty() && line[0] == '#') {
            first_line = false;
            continue;
        }
        first_line = false;

        std::istringstream iss(line);
        int edge_id;
        t_rr_switch_override_inf rr_switch_detailed_inf;

        if (!(iss >> edge_id >> rr_switch_detailed_inf.Tdel)) {
            throw std::runtime_error("Invalid line format: " + line);
        }

        if (edge_id < 0) {
            throw std::runtime_error("Negative integer found: " + std::to_string(edge_id));
        }

        auto switch_idx = rr_graph.rr_nodes().edge_switch((RREdgeId)edge_id);
        const t_rr_switch_inf& switch_inf = rr_graph.rr_switch_inf((RRSwitchId)switch_idx);

        rr_switch_detailed_inf.Tdel += switch_inf.Tdel;

        // check if the detailed info has already been added
        auto it = unique_edge_offsets.find(rr_switch_detailed_inf);

        RRSwitchOffsetInfoId rr_switch_offset_id;
        if (it == unique_edge_offsets.end()) {
            // if the detailed info is seen for the first time, add it to RR graph and record it in `unique_edge_offsets`
            rr_switch_offset_id = rr_graph_builder.add_rr_switch_offset_info(rr_switch_detailed_inf);
            unique_edge_offsets.insert({rr_switch_detailed_inf, rr_switch_offset_id});
        } else {
            // the detailed info has already been added to the RR graph
            rr_switch_offset_id = it->second;
        }

        rr_graph_builder.set_edge_offset_id((RREdgeId)edge_id, rr_switch_offset_id);
    }
}

