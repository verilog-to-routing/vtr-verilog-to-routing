/*
 * The router lookahead provides an estimate of the cost from an intermediate node to the target node
 * during directed (A*-like) routing.
 *
 * The lookahead in this file requires an externally loaded lookahead cost map which will be queried 
 * based only on the distance between the currently explored node and the target sink.
 * 
 * This lookahead can be made admissible if the loaded cost map contains the minimum delay and minimum
 * congestion costs per distance.
 */

#include <algorithm>
#include <thread>
#include <unordered_map>
#include <vector>
#include <queue>
#include <fstream>
#include "globals.h"
#include "vtr_time.h"
#include "router_lookahead.h"
#include "router_lookahead_simple.h"
#include "router_lookahead_map_utils.h"
#include "rr_graph.h"
#include "connection_router_interface.h"
#include "arch_util.h"

#ifdef VTR_ENABLE_CAPNPROTO
#include "capnp/serialize.h"
#include "map_lookahead.capnp.h"
#include "ndmatrix_serdes.h"
#include "intra_cluster_serdes.h"
#include "mmap_file.h"
#include "serdes_utils.h"
#endif /* VTR_ENABLE_CAPNPROTO */

/******** File-Scope Variables ********/

//Look-up table from CHANX/CHANY (to SINKs) for various distances
static t_simple_cost_map simple_cost_map;

/******** File-Scope Functions ********/

static util::Cost_Entry get_wire_cost_entry(e_rr_type rr_type,
                                            int seg_index,
                                            int from_layer_num,
                                            int delta_x,
                                            int delta_y,
                                            int to_layer_num);

static void read_router_lookahead(const std::string& file);
static void write_router_lookahead(const std::string& file);

/******** Interface class member function definitions ********/

float SimpleLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    // Get the total cost using the combined delay and congestion costs
    auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);
    return delay_cost + cong_cost;
}

std::pair<float, float> SimpleLookahead::get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;

    float expected_delay_cost = std::numeric_limits<float>::max() / 1e12;
    float expected_cong_cost = std::numeric_limits<float>::max() / 1e12;

    e_rr_type from_type = rr_graph.node_type(from_node);
    util::Cost_Entry cost_entry(0, 0);
    if (is_chanxy(from_type)) {
        // TODO: handle CHANZ nodes
        int from_layer_num = rr_graph.node_layer_low(from_node);
        int to_layer_num = rr_graph.node_layer_low(to_node);

        auto [delta_x, delta_y] = util::get_xy_deltas(from_node, to_node);
        delta_x = abs(delta_x);
        delta_y = abs(delta_y);

        auto from_cost_index = rr_graph.node_cost_index(from_node);
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        cost_entry = get_wire_cost_entry(from_type, from_seg_index, from_layer_num, delta_x, delta_y, to_layer_num);
    }

    if (cost_entry.valid()) {
        float expected_delay = cost_entry.delay;
        float expected_cong = cost_entry.congestion;

        expected_delay_cost = params.criticality * expected_delay;
        expected_cong_cost = (1.0 - params.criticality) * expected_cong;
    }

    return std::make_pair(expected_delay_cost, expected_cong_cost);
}

void SimpleLookahead::read(const std::string& file) {
    const size_t num_layers = g_vpr_ctx.device().grid.get_num_layers();
    VTR_ASSERT_MSG(num_layers == 1, "Simple router look-ahead does not support 3-d architectures.");

    read_router_lookahead(file);
}

void SimpleLookahead::write(const std::string& file) const {
    const size_t num_layers = g_vpr_ctx.device().grid.get_num_layers();
    VTR_ASSERT_MSG(num_layers == 1, "Simple router look-ahead does not support 3-d architectures.");

    if (vtr::check_file_name_extension(file, ".csv")) {
        std::vector<int> simple_cost_map_size(simple_cost_map.ndims());
        for (size_t i = 0; i < simple_cost_map.ndims(); ++i) {
            simple_cost_map_size[i] = static_cast<int>(simple_cost_map.dim_size(i));
        }
        dump_readable_router_lookahead_map(file, simple_cost_map_size, get_wire_cost_entry);
    } else {
        VTR_ASSERT(vtr::check_file_name_extension(file, ".capnp") || vtr::check_file_name_extension(file, ".bin"));
        write_router_lookahead(file);
    }
}

/******** Function Definitions ********/

static util::Cost_Entry get_wire_cost_entry(e_rr_type rr_type, int seg_index, int from_layer_num, int delta_x, int delta_y, int to_layer_num) {
    VTR_ASSERT_SAFE(rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY);
    VTR_ASSERT_SAFE(from_layer_num < static_cast<int>(simple_cost_map.dim_size(0)));
    VTR_ASSERT_SAFE(to_layer_num < static_cast<int>(simple_cost_map.dim_size(1)));
    VTR_ASSERT_SAFE(seg_index < static_cast<int>(simple_cost_map.dim_size(3)));
    VTR_ASSERT_SAFE(delta_x < static_cast<int>(simple_cost_map.dim_size(4)));
    VTR_ASSERT_SAFE(delta_y < static_cast<int>(simple_cost_map.dim_size(5)));

    int chan_index = 0;
    if (rr_type == e_rr_type::CHANY) {
        chan_index = 1;
    }

    return simple_cost_map[from_layer_num][to_layer_num][chan_index][seg_index][delta_x][delta_y];
}

//
// When writing capnp targetted serialization, always allow compilation when
// VTR_ENABLE_CAPNPROTO=OFF.  Generally this means throwing an exception
// instead.
//
#ifndef VTR_ENABLE_CAPNPROTO

#define DISABLE_ERROR                               \
    "is disabled because VTR_ENABLE_CAPNPROTO=OFF." \
    "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable."

void read_router_lookahead(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "SimpleLookahead::read_router_lookahead " DISABLE_ERROR);
}

void write_router_lookahead(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "SimpleLookahead::write_router_lookahead " DISABLE_ERROR);
}

#else /* VTR_ENABLE_CAPNPROTO */

static void ToCostEntry(util::Cost_Entry* out, const VprMapCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
}

static void FromCostEntry(VprMapCostEntry::Builder* out, const util::Cost_Entry& in) {
    out->setDelay(in.delay);
    out->setCongestion(in.congestion);
}

void read_router_lookahead(const std::string& file) {
    vtr::ScopedStartFinishTimer timer("Loading router wire lookahead map");
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto map = reader.getRoot<VprMapLookahead>();

    ToNdMatrix<6, VprMapCostEntry, util::Cost_Entry>(&simple_cost_map, map.getCostMap(), ToCostEntry);
}

void write_router_lookahead(const std::string& file) {
    ::capnp::MallocMessageBuilder builder;

    auto map = builder.initRoot<VprMapLookahead>();

    auto cost_map = map.initCostMap();
    FromNdMatrix<6, VprMapCostEntry, util::Cost_Entry>(&cost_map, simple_cost_map, FromCostEntry);

    writeMessageToFile(file, &builder);
}

#endif
