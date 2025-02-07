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
#include <unordered_set>

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

RREdgeId process_line(const std::string& line,
                      std::vector<float>& overridden_values,
                      const RRGraphView& rr_graph) {
    std::istringstream iss(line);
    char ch;
    RREdgeId edge_id;

    if (std::isdigit(line[0])) {
        // Line starts with an integer
        int first;
        iss >> first;
        edge_id = (RREdgeId)first;
    } else if (line[0] == '(') {
        // Line starts with (first, second)
        int first, second;
        iss >> ch >> first >> ch >> second >> ch;

        RRNodeId src_node_id = RRNodeId(first);
        RRNodeId sink_node_id = RRNodeId(second);

        for (RREdgeId outgoing_edge_id : rr_graph.rr_nodes().edge_range(src_node_id)) {
            if (rr_graph.rr_nodes().edge_sink_node(outgoing_edge_id) == sink_node_id) {
                edge_id = outgoing_edge_id;
                break;
            }
        }

        if (!edge_id.is_valid()) {
            VTR_LOG_ERROR("Couldn't find an edge connecting node %d to node %d\n", src_node_id, sink_node_id);
        }

    } else {
        VTR_LOG_ERROR("Invalid line format:  %s\n", line.c_str());
    }

    float value;
    while (iss >> value) {
        overridden_values.push_back(value);
    }

    return edge_id;
}

struct t_rr_switch_inf_hash {
    std::size_t operator()(const t_rr_switch_inf& s) const {
        std::size_t seed = 0;

        // Helper function for hashing
        auto hash_combine = [&seed](auto&& val) {
            seed ^= std::hash<std::decay_t<decltype(val)>>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        // Combine all relevant fields
        hash_combine(s.R);
        hash_combine(s.Cin);
        hash_combine(s.Cout);
        hash_combine(s.Cinternal);
        hash_combine(s.Tdel);
        hash_combine(s.mux_trans_size);
        hash_combine(s.buf_size);
        hash_combine(static_cast<int>(s.power_buffer_type));
        hash_combine(s.power_buffer_size);
        hash_combine(s.intra_tile);
        hash_combine(static_cast<int>(s.type()));

        return seed;
    }
};

struct t_rr_switch_inf_equal {
    bool operator()(const t_rr_switch_inf& lhs, const t_rr_switch_inf& rhs) const {
        return lhs.R == rhs.R &&
               lhs.Cin == rhs.Cin &&
               lhs.Cout == rhs.Cout &&
               lhs.Cinternal == rhs.Cinternal &&
               lhs.Tdel == rhs.Tdel &&
               lhs.mux_trans_size == rhs.mux_trans_size &&
               lhs.buf_size == rhs.buf_size &&
               lhs.power_buffer_type == rhs.power_buffer_type &&
               lhs.power_buffer_size == rhs.power_buffer_size &&
               lhs.intra_tile == rhs.intra_tile &&
               lhs.type() == rhs.type();
    }
};

void load_rr_edge_overrides(std::string_view filename,
                            RRGraphBuilder& rr_graph_builder,
                            RRGraphView& rr_graph) {
    std::ifstream file(filename.data());
    if (!file) {
        VTR_LOG_ERROR("Failed to open the RR edge override file: %s\n", filename.data());
    }

    std::unordered_map<t_rr_switch_inf, RRSwitchId, t_rr_switch_inf_hash, t_rr_switch_inf_equal> unique_switch_info;
    for (const auto& [rr_sw_idx, sw] : rr_graph.rr_switch().pairs()) {
        unique_switch_info.insert({sw, rr_sw_idx});
    }

    std::string line;
    std::vector<float> overridden_values;
    bool firstLine = true;

    while (std::getline(file, line)) {
        if (firstLine) {
            if (line.empty() || line[0] != '#') {
                VTR_LOG_ERROR("Error: First line must start with #\n");
            }
            firstLine = false;
            continue;  // Ignore first line
        }


        if (!line.empty()) {
            overridden_values.clear();
            RREdgeId edge_id = process_line(line, overridden_values, rr_graph);
            RRSwitchId curr_switch_id = (RRSwitchId)rr_graph.edge_switch(edge_id);
            t_rr_switch_inf switch_override_info = rr_graph.rr_switch_inf(curr_switch_id);

            switch_override_info.Tdel = (overridden_values.size() >= 1) ? overridden_values[0] : switch_override_info.Tdel;
            switch_override_info.R = (overridden_values.size() >= 2) ? overridden_values[1] : switch_override_info.R;
            switch_override_info.Cin = (overridden_values.size() >= 3) ? overridden_values[2] : switch_override_info.Cin;
            switch_override_info.Cout = (overridden_values.size() >= 4) ? overridden_values[3] : switch_override_info.Cout;
            switch_override_info.Cinternal = (overridden_values.size() >= 5) ? overridden_values[4] : switch_override_info.Cinternal;

            RRSwitchId new_switch_id;
            auto it = unique_switch_info.find(switch_override_info);
            if (it == unique_switch_info.end()) {
                new_switch_id = rr_graph_builder.add_rr_switch(switch_override_info);
                unique_switch_info.insert({switch_override_info, new_switch_id});
            } else {
                new_switch_id = it->second;
            }
            rr_graph_builder.override_edge_switch(edge_id, new_switch_id);
        }
    }
}