#include "alloc_and_load_rr_indexed_data.h"

#include <algorithm>
#include <cmath> /* Needed only for sqrt call (remove if sqrt removed) */
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "librrgraph_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"

#include "vpr_error.h"
#include "rr_graph_utils.h"
#include "rr_graph_cost.h"
#include "histogram.h"

/******************* Subroutines local to this module ************************/

static void load_rr_indexed_data_base_costs(const RRGraphView& rr_graph,
                                            vtr::vector<RRIndexedDataId,
                                            t_rr_indexed_data>& rr_indexed_data,
                                            e_base_cost_type base_cost_type,
                                            const bool echo_enabled,
                                            const char* echo_file_name);

static float get_delay_normalization_fac(const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data, const bool echo_enabled, const char* echo_file_name);

static void load_rr_indexed_data_T_values(const RRGraphView& rr_graph, vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data);

/**
 * @brief Computes average R, Tdel, and Cinternal of fan-in switches for a given node.
 *
 * Iterates over all incoming edges of @p inode, skipping SHORT switches,
 * and averages their electrical properties. Also counts shorts, tracks the
 * number of valid switches, and determines whether the node is buffered
 * (true if any fan-in switch is buffered).
 *
 * @note It is not safe to assume that each node of the same wire type has the same switches with the same
 * delays, therefore we take their average to take into account the possible differences
 */
static void calculate_average_switch(const RRGraphView& rr_graph,
                                     RRNodeId inode,
                                     double& avg_switch_R,
                                     double& avg_switch_T,
                                     double& avg_switch_Cinternal,
                                     int& num_switches,
                                     int& num_shorts,
                                     short& buffered,
                                     vtr::vector<RRNodeId, std::vector<RREdgeId>>& fan_in_list);

static void fixup_rr_indexed_data_T_values(vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data, size_t num_segment);

static std::vector<size_t> count_rr_segment_types(const RRGraphView& rr_graph,
                                                  const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data);

static void print_rr_index_info(const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                const char* fname,
                                const std::vector<t_segment_inf>& segment_inf,
                                size_t y_chan_cost_offset,
                                size_t z_chan_cost_offset);

/******************** Subroutine definitions *********************************/

/* Allocates the rr_indexed_data array and loads it with appropriate values. *
 * It currently stores the segment type (or OPEN if the index doesn't        *
 * correspond to an CHANX or CHANY type), the base cost of nodes of that     *
 * type, and some info to allow rapid estimates of time to get to a target   *
 * to be computed by the router.                                             *
 *
 * Right now all SOURCES have the same base cost; and similarly there's only *
 * one base cost for each of SINKs, OPINs, and IPINs (four total).  This can *
 * be changed just by allocating more space in the array below and changing  *
 * the cost_index values for these rr_nodes, if you want to make some pins   *
 * etc. more expensive than others.  I give each segment type in an          *
 * x-channel its own cost_index, and each segment type in a y-channel its    *
 * own cost_index.                                                           */
void alloc_and_load_rr_indexed_data(const RRGraphView& rr_graph,
                                    const DeviceGrid& grid,
                                    const std::vector<t_segment_inf>& segment_inf,
                                    const std::vector<t_segment_inf>& segment_inf_x,
                                    const std::vector<t_segment_inf>& segment_inf_y,
                                    const std::vector<t_segment_inf>& segment_inf_z,
                                    vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                    RRSwitchId wire_to_ipin_switch,
                                    e_base_cost_type base_cost_type,
                                    const bool echo_enabled,
                                    const char* echo_file_name) {
    const size_t total_num_segment = segment_inf_x.size() + segment_inf_y.size() + segment_inf_z.size();

    // CHAX & CHANY segment list sizes may differ, but if we're using uniform channels, they will have equal sizes
    int num_rr_indexed_data = CHANX_COST_INDEX_START + total_num_segment;
    rr_indexed_data.resize(num_rr_indexed_data);

    // For rr_types that aren't CHANX/CHANY/CHANZ, base_cost is valid, but most
    // other fields are invalid. For IPINs, the T_linear field is also valid;
    // all other fields are invalid. For SOURCES, SINKs and OPINs, all fields
    // other than base_cost are invalid. Mark invalid fields as LIBRRGRAPH_UNDEFINED_VAL
    // for safety.
    for (int i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
        rr_indexed_data[RRIndexedDataId(i)].ortho_cost_index = LIBRRGRAPH_UNDEFINED_VAL;
        rr_indexed_data[RRIndexedDataId(i)].seg_index = LIBRRGRAPH_UNDEFINED_VAL;
        rr_indexed_data[RRIndexedDataId(i)].inv_length = std::numeric_limits<float>::quiet_NaN();;
        rr_indexed_data[RRIndexedDataId(i)].T_linear = 0.;
        rr_indexed_data[RRIndexedDataId(i)].T_quadratic = 0.;
        rr_indexed_data[RRIndexedDataId(i)].C_load = 0.;
    }

    //TODO: SM: IPIN t_linear assumes wire_to_ipin_switch which corresponds to within die switch connection
    rr_indexed_data[RRIndexedDataId(IPIN_COST_INDEX)].T_linear = rr_graph.rr_switch_inf(wire_to_ipin_switch).Tdel;

    std::vector<int> ortho_costs = find_ortho_cost_index(rr_graph, segment_inf_x, segment_inf_y, e_parallel_axis::X_AXIS);

    /* AA: The code below should replace find_ortho_cost_index call once we deprecate the CLASSIC lookahead as it is the only lookahead
     * that actively uses the orthogonal cost indices. To avoid complicated dependencies with the rr_graph reader, regardless of the lookahead,
     * we walk to the rr_graph edges to get these indices. */
    /*
     *
     * std::vector<int> x_costs(segment_inf_x.size(), CHANX_COST_INDEX_START + segment_inf_x.size());
     * std::vector<int> y_costs(segment_inf_y.size(), CHANX_COST_INDEX_START);
     *
     * std::move(x_costs.begin(), x_costs.end(), std::back_inserter(ortho_costs));
     * std::move(y_costs.begin(), y_costs.end(), std::back_inserter(ortho_costs));
     */

    for (size_t iseg = 0; iseg < total_num_segment; ++iseg) {
        RRIndexedDataId index = RRIndexedDataId(iseg + CHANX_COST_INDEX_START);

        const t_segment_inf* seg_ptr = nullptr;
        int offset = 0;

        if (iseg < segment_inf_x.size()) {
            // X-directed
            seg_ptr = &segment_inf_x[iseg];
            rr_indexed_data[index].ortho_cost_index = ortho_costs[iseg];
        } else if (iseg < segment_inf_x.size() + segment_inf_y.size()) {
            // Y-directed
            offset = iseg - segment_inf_x.size();
            seg_ptr = &segment_inf_y[offset];
            rr_indexed_data[index].ortho_cost_index = ortho_costs[iseg];
        } else {
            // Z-directed
            offset = iseg - segment_inf_x.size() - segment_inf_y.size();
            seg_ptr = &segment_inf_z[offset];
            rr_indexed_data[index].ortho_cost_index = LIBRRGRAPH_UNDEFINED_VAL;
        }

        int length;
        if (seg_ptr->longline) {
            length = grid.width();
        } else {
            length = std::min<int>(seg_ptr->length, grid.width());
        }

        rr_indexed_data[index].inv_length = 1. / length;
        rr_indexed_data[index].seg_index = seg_ptr->seg_index;
    }

    load_rr_indexed_data_T_values(rr_graph, rr_indexed_data);

    fixup_rr_indexed_data_T_values(rr_indexed_data, total_num_segment);

    load_rr_indexed_data_base_costs(rr_graph, rr_indexed_data, base_cost_type, echo_enabled, echo_file_name);

    if (echo_enabled) {
        print_rr_index_info(rr_indexed_data,
                            echo_file_name,
                            segment_inf,
                            segment_inf_x.size(),
                            segment_inf_x.size() + segment_inf_y.size());
    }
}

/*  AA: We use a normalized product of frequency and length to find the segment that is most likely
 * to connect to in the perpendicular axis. Note that the size of segment_inf_x & segment_inf_y is not 
 * the same necessarily. The result vector will contain the indices in segment_inf_perp
 * of the most likely perp segments for each segment at index i in segment_inf_parallel.   
 * 
 * Note: We use the seg_index field of t_segment_inf to store the segment index  
 * in the **unified** t_segment_inf vector. We will temporarily use this field in
 * a copy passed to the function to store the index w.r.t the parallel axis segment list.*/

std::vector<int> find_ortho_cost_index(const RRGraphView& rr_graph,
                                       const std::vector<t_segment_inf>& segment_inf_x,
                                       const std::vector<t_segment_inf>& segment_inf_y,
                                       e_parallel_axis parallel_axis) {
    size_t num_segments = segment_inf_x.size() + segment_inf_y.size();

    // dest_nodes_count[seg][perp_seg] counts how many times each segment type is connected to perpendicular segment types
    std::vector<std::vector<size_t>> dest_nodes_count(num_segments);

    // x segments are perpendicular to y segments
    for (size_t iseg = 0; iseg < segment_inf_x.size(); iseg++) {
        dest_nodes_count[iseg].resize(segment_inf_y.size());
    }
    // y segments are perpendicular to x segments
    for (size_t iseg = segment_inf_x.size(); iseg < num_segments; iseg++) {
        dest_nodes_count[iseg].resize(segment_inf_x.size());
    }

    // Go through all rr_Nodes. Look at the ones with CHAN type. Count all outgoing edges to CHAN typed nodes from each CHAN type node.
    for (const RRNodeId rr_node : rr_graph.nodes()) {
        for (size_t iedge = 0; iedge < rr_graph.num_edges(rr_node); ++iedge) {
            RRNodeId to_node = rr_graph.edge_sink_node(rr_node, iedge);
            e_rr_type from_node_type = rr_graph.node_type(rr_node);
            e_rr_type to_node_type = rr_graph.node_type(to_node);

            size_t from_node_cost_index = (size_t)rr_graph.node_cost_index(rr_node);
            size_t to_node_cost_index = (size_t)rr_graph.node_cost_index(to_node);

            // if the type  is smaller than start index, means destination is not a CHAN type node.

            if ((from_node_type == e_rr_type::CHANX && to_node_type == e_rr_type::CHANY) || (from_node_type == e_rr_type::CHANY && to_node_type == e_rr_type::CHANX)) {
                if (to_node_type == e_rr_type::CHANY) {
                    dest_nodes_count[from_node_cost_index - CHANX_COST_INDEX_START][to_node_cost_index - (CHANX_COST_INDEX_START + segment_inf_x.size())]++;
                } else {
                    dest_nodes_count[from_node_cost_index - CHANX_COST_INDEX_START][to_node_cost_index - CHANX_COST_INDEX_START]++;
                }
            }
        }
    }

    std::vector<int> ortho_cost_indices(num_segments, 0);

    for (size_t iseg = 0; iseg < segment_inf_x.size(); iseg++) {
        ortho_cost_indices[iseg] = std::ranges::max_element(dest_nodes_count[iseg]) - dest_nodes_count[iseg].begin();
        ortho_cost_indices[iseg] += CHANX_COST_INDEX_START + segment_inf_x.size();
    }

    for (size_t iseg = segment_inf_x.size(); iseg < num_segments; iseg++) {
        ortho_cost_indices[iseg] = std::ranges::max_element(dest_nodes_count[iseg]) - dest_nodes_count[iseg].begin();
        ortho_cost_indices[iseg] += CHANX_COST_INDEX_START;
    }

    return ortho_cost_indices;

    /*Update seg_index */

#ifdef FREQ_LENGTH_ORTHO_COSTS
    const std::vector<t_segment_inf>& segment_inf_parallel = parallel_axis == X_AXIS ? segment_inf_x : segment_inf_y;
    const std::vector<t_segment_inf>& segment_inf_perp = parallel_axis == X_AXIS ? segment_inf_y : segment_inf_x;
    for (int i = 0; i < (int)segment_inf_perp.size(); ++i)
        segment_inf_perp[i].seg_index = i;

    std::vector<int> ortho_costs_indices;
    ortho_costs_indices.resize(segment_inf_parallel.size());

    int num_segments = (int)segment_inf_parallel.size();
    for (int seg_index = 0; seg_index < num_segments; ++seg_index) {
        auto segment = segment_inf_parallel[seg_index];
        auto lambda_cmp = [&segment](t_segment_inf& a, t_segment_inf& b) {
            float a_freq = a.frequency / (float)segment.frequency;
            float b_freq = b.frequency / (float)segment.frequency;
            float a_len = (float)segment.length / a.length;
            float b_len = (float)segment.length / b.length;

            float a_product = a_len * a_freq;
            float b_product = b_len * b_freq;

            if (std::abs(a_product - 1) < std::abs(b_product - 1))
                return true;
            else if (std::abs(a_product - 1) > std::abs(b_product - 1))
                return false;
            else {
                if ((segment.name == a.name && segment.parallel_axis == BOTH_AXIS) || (segment.length == a.length && segment.frequency == a.frequency))
                    return true;
                else {
                    if (a.frequency > b.frequency)
                        return true;
                    else if (a.frequency < b.frequency)
                        return false;
                    else
                        return a.length < b.length;
                }
            }
        };

        std::sort(segment_inf_perp.begin(), segment_inf_perp.end(), lambda_cmp);

        /* The compartor behaves as operator< mostly, so the first element in the 
         * sorted vector will have the lowest cost difference from segment. */
        ortho_costs_indices[seg_index] = segment_inf_perp[0].seg_index + start_channel_cost;
        ortho_costs_indices[seg_index] = parallel_axis == e_parallel_axis::X_AXIS ? ortho_costs_indices[seg_index] + num_segments : ortho_costs_indices[seg_index];
    }

    /*Pertubate indices to make sure all perp seg types have a corresponding perp segment.*/
#    ifdef PERTURB_ORTHO_COST_indices
    std::vector<int> perp_segments;
    std::unordered_multimap<int, int> indices_map;
    auto cmp_greater = [](std::pair<int, int> a, std::pair<int, int> b) {
        return a.second < b.second;
    };
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, decltype(cmp_greater)> indices_q_greater(cmp_greater);

    auto cmp_less = [](std::pair<int, int> a, std::pair<int, int> b) {
        return a.second > b.second;
    };

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, decltype(cmp_less)> indices_q_less(cmp_less);

    perp_segments.resize(segment_inf_perp.size(), 0);

    for (int i = 0; i < num_segments; ++i) {
        int index = parallel_axis == e_parallel_axis::X_AXIS ? ortho_costs_indices[i] - num_segments - start_channel_cost : ortho_costs_indices[i] - start_channel_cost;
        indices_map.insert(std::make_pair(index, i));
        perp_segments[index]++;
    }

    for (int i = 0; i < (int)perp_segments.size(); ++i) {
        auto pair = std::make_pair(i, perp_segments[i]);
        indices_q_greater.push(pair);
        indices_q_less.push(pair);
    }

    while (!indices_q_greater.empty()) {
        auto g_index_pair = indices_q_greater.top();
        auto l_index_pair = indices_q_less.top();

        if (l_index_pair.second != 0)
            break;

        indices_q_greater.pop();
        indices_q_less.pop();

        g_index_pair.second--;
        l_index_pair.second++;
        auto itr_to_change = indices_map.find(g_index_pair.first);
        VTR_ASSERT(itr_to_change != indices_map.end());
        int index = l_index_pair.first + start_channel_cost;
        index = parallel_axis == e_parallel_axis::X_AXIS ? index + num_segments : index;
        ortho_costs_indices[itr_to_change->second] = index;
        indices_map.erase(itr_to_change);

        indices_q_greater.push(g_index_pair);
        indices_q_less.push(l_index_pair);
    }
#    endif

    return ortho_costs_indices;

#else
    (void)parallel_axis;
#endif
}

static void load_rr_indexed_data_base_costs(const RRGraphView& rr_graph,
                                            vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                            e_base_cost_type base_cost_type,
                                            const bool echo_enabled,
                                            const char* echo_file_name) {
    // Loads the base_cost member of rr_indexed_data according to the specified base_cost_type.

    float delay_normalization_fac;
    if (base_cost_type == DEMAND_ONLY || base_cost_type == DEMAND_ONLY_NORMALIZED_LENGTH) {
        delay_normalization_fac = 1.;
    } else {
        delay_normalization_fac = get_delay_normalization_fac(rr_indexed_data, echo_enabled, echo_file_name);
    }

    rr_indexed_data[RRIndexedDataId(SOURCE_COST_INDEX)].base_cost = delay_normalization_fac;
    rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost = 0.;
    rr_indexed_data[RRIndexedDataId(MUX_COST_INDEX)].base_cost = delay_normalization_fac;
    rr_indexed_data[RRIndexedDataId(OPIN_COST_INDEX)].base_cost = delay_normalization_fac;
    // The IPIN_COST_INDEX base cost is changed from 0.95 to 0.875 so it is perfectly representable in binary format (this change is made for SPEC benchmark).
    // This number is perfectly representable in a binary mantissa (without round-off) so we can get the same routing result on different platforms.
    // Since the router cost calculations and heap use floating point numbers, normally we get slightly different round off with different compiler settings,
    // leading to different heap sorts and hence different routings.
    // To make result validation for SPEC easier, we choose all router parameters to result in calculations that fit perfectly in a 24-bit binary mantissa.
    // .875 = 1/2 + 1/4 + 1/8 can be perfectly represented in a binary mantissa with only the first 3 bits set.
    rr_indexed_data[RRIndexedDataId(IPIN_COST_INDEX)].base_cost = 0.875 * delay_normalization_fac;

    auto rr_segment_counts = count_rr_segment_types(rr_graph, rr_indexed_data);
    size_t total_segments = std::accumulate(rr_segment_counts.begin(), rr_segment_counts.end(), 0u);

    /* Load base costs for CHANX and CHANY segments */
    float max_length = 0;
    const float min_length = 1;
    if (base_cost_type == DELAY_NORMALIZED_LENGTH_BOUNDED) {
        for (size_t index = CHANX_COST_INDEX_START; index < rr_indexed_data.size(); index++) {
            float length = (1 / rr_indexed_data[RRIndexedDataId(index)].inv_length);
            max_length = std::max(max_length, length);
        }
    }

    //Future Work: Since we can now have wire types which don't connect to IPINs,
    //             perhaps consider lowering cost of wires which connect to IPINs
    //             so they get explored earlier (same rational as lowering IPIN costs)

    for (size_t index = CHANX_COST_INDEX_START; index < rr_indexed_data.size(); index++) {
        if (base_cost_type == DELAY_NORMALIZED || base_cost_type == DEMAND_ONLY) {
            rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac;
        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH || base_cost_type == DEMAND_ONLY_NORMALIZED_LENGTH) {
            rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac / rr_indexed_data[RRIndexedDataId(index)].inv_length;
        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH_BOUNDED) {
            float length = (1 / rr_indexed_data[RRIndexedDataId(index)].inv_length);
            if (max_length != min_length) {
                float length_scale = 1.f + 3.f * (length - min_length) / (max_length - min_length);
                rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac * length_scale;
            } else {
                rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac;
            }

        } else if (base_cost_type == DELAY_NORMALIZED_FREQUENCY) {
            int seg_index = rr_indexed_data[RRIndexedDataId(index)].seg_index;

            VTR_ASSERT(total_segments > 0);
            float freq_fac = float(rr_segment_counts[seg_index]) / total_segments;

            rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac / freq_fac;

        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH_FREQUENCY) {
            int seg_index = rr_indexed_data[RRIndexedDataId(index)].seg_index;

            VTR_ASSERT(total_segments > 0);
            float freq_fac = float(rr_segment_counts[seg_index]) / total_segments;

            //Base cost = delay_norm / (len * freq)
            //rr_indexed_data[RRIndexedDataId(index)].base_cost = delay_normalization_fac / ((1. / rr_indexed_data[RRIndexedDataId(index)].inv_length) * freq_fac);

            //Base cost = (delay_norm * len) * (1 + (1-freq))
            rr_indexed_data[RRIndexedDataId(index)].base_cost = (delay_normalization_fac / rr_indexed_data[RRIndexedDataId(index)].inv_length) * (1 + (1 - freq_fac));

        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unrecognized base cost type");
        }
    }

    /* Save a copy of the base costs -- if dynamic costing is used by the     *
     * router, the base_cost values will get changed all the time and being   *
     * able to restore them from a saved version is useful.                   */

    for (size_t index = 0; index < rr_indexed_data.size(); index++) {
        rr_indexed_data[RRIndexedDataId(index)].saved_base_cost = rr_indexed_data[RRIndexedDataId(index)].base_cost;
    }
}

static std::vector<size_t> count_rr_segment_types(const RRGraphView& rr_graph,
                                                  const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data) {
    std::vector<size_t> rr_segment_type_counts;

    for (const RRNodeId id : rr_graph.nodes()) {
        e_rr_type node_type = rr_graph.node_type(id);
        if (!is_chanxy(node_type) && !is_chanz(node_type)) {
            continue;
        }

        RRIndexedDataId cost_index = rr_graph.node_cost_index(id);

        int seg_index = rr_indexed_data[cost_index].seg_index;

        VTR_ASSERT(seg_index != LIBRRGRAPH_UNDEFINED_VAL);

        if (seg_index >= int(rr_segment_type_counts.size())) {
            rr_segment_type_counts.resize(seg_index + 1, 0);
        }
        VTR_ASSERT(seg_index < int(rr_segment_type_counts.size()));

        ++rr_segment_type_counts[seg_index];
    }

    return rr_segment_type_counts;
}

static float get_delay_normalization_fac(const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                         const bool echo_enabled,
                                         const char* echo_file_name) {
    /* Returns the average delay to go 1 CLB distance along a wire.  */

    float Tdel_sum = 0.0;
    int Tdel_num = 0;
    for (size_t cost_index = CHANX_COST_INDEX_START; cost_index < rr_indexed_data.size(); cost_index++) {
        float inv_length = rr_indexed_data[RRIndexedDataId(cost_index)].inv_length;
        float T_value = rr_indexed_data[RRIndexedDataId(cost_index)].T_linear * inv_length + rr_indexed_data[RRIndexedDataId(cost_index)].T_quadratic * std::pow(inv_length, 2);

        if (T_value == 0.0) continue;

        Tdel_sum += T_value;
        Tdel_num += 1;
    }

    if (Tdel_num == 0) {
        VTR_LOG_WARN("No valid cost index was found to get the delay normalization factor. Setting delay normalization factor to 1e-9 (1 ns)\n");
        return 1e-9;
    }

    float delay_norm_fac = Tdel_sum / Tdel_num;

    if (echo_enabled) {
        std::ofstream out_file;

        out_file.open(echo_file_name);
        out_file << "Delay normalization factor: " << delay_norm_fac << std::endl;

        out_file.close();
    }

    return delay_norm_fac;
}

/*
 * Scans all the RR nodes of CHAN type getting the medians for their R and C values (delays)
 * as well as the delay data of all the nodes' switches, averaging them to find the following
 * indexed data values for each wire type:
 *      - T_linear
 *      - T_quadratic
 *      - C_load
 *
 * The indexed data is used in different locations such as:
 *      - Base cost calculation for each cost_index
 *      - Lookahead map computation
 *      - Placement Delay Matrix computation
 */
static void load_rr_indexed_data_T_values(const RRGraphView& rr_graph,
                                          vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data) {
    vtr::vector<RRNodeId, std::vector<RREdgeId>> fan_in_list = get_fan_in_list(rr_graph);

    vtr::vector<RRIndexedDataId, int> num_nodes_of_index(rr_indexed_data.size(), 0);
    vtr::vector<RRIndexedDataId, std::vector<float>> C_total(rr_indexed_data.size());
    vtr::vector<RRIndexedDataId, std::vector<float>> R_total(rr_indexed_data.size());


    // Not all wire-to-wire switches connecting from some wire segment will necessarily have the same delay.
    // i.e. a mux with less inputs will have smaller delay than a mux with a greater number of inputs.
    // So to account for these differences we will get the average R/Tdel/Cinternal values by first averaging
    // them for a single wire segment, and then by averaging this value over all the average values corresponding
    // to the switches node
    vtr::vector<RRIndexedDataId, std::vector<float>> switch_R_total(rr_indexed_data.size());
    vtr::vector<RRIndexedDataId, std::vector<float>> switch_T_total(rr_indexed_data.size());
    vtr::vector<RRIndexedDataId, std::vector<float>> switch_Cinternal_total(rr_indexed_data.size());
    vtr::vector<RRIndexedDataId, short> switches_buffered(rr_indexed_data.size(), LIBRRGRAPH_UNDEFINED_VAL);

    // Walk through the RR graph and collect all R and C values of all the nodes,
    // as well as their fan-in switches R, T_del, and Cinternal values.
    // The median of R and C values for each cost index is assigned to the indexed data.
    for (const RRNodeId rr_id : rr_graph.nodes()) {
        e_rr_type rr_type = rr_graph.node_type(rr_id);

        if (!is_chanxy(rr_type) && !is_chanz(rr_type)) {
            continue;
        }

        RRIndexedDataId cost_index = rr_graph.node_cost_index(rr_id);

        // Get average switch parameters
        double avg_switch_R = 0;
        double avg_switch_T = 0;
        double avg_switch_Cinternal = 0;
        int num_switches = 0;
        int num_shorts = 0;
        short buffered = LIBRRGRAPH_UNDEFINED_VAL;
        calculate_average_switch(rr_graph, rr_id, avg_switch_R, avg_switch_T, avg_switch_Cinternal, num_switches, num_shorts, buffered, fan_in_list);

        if (num_switches == 0) {
            if (num_shorts == 0) {
                std::string node_cords = rr_graph.node_coordinate_to_string(RRNodeId(rr_id));
                VTR_LOG_WARN("Node: %d with RR_type: %s  at Location:%s, had no out-going switches\n", rr_id,
                             rr_graph.node_type_string(rr_id), node_cords.c_str());
            }
            continue;
        }
        VTR_ASSERT(num_switches > 0 || num_shorts > 0);

        num_nodes_of_index[cost_index]++;
        C_total[cost_index].push_back(rr_graph.node_C(rr_id));
        R_total[cost_index].push_back(rr_graph.node_R(rr_id));

        switch_R_total[cost_index].push_back(avg_switch_R);
        switch_T_total[cost_index].push_back(avg_switch_T);
        switch_Cinternal_total[cost_index].push_back(avg_switch_Cinternal);
        if (buffered == LIBRRGRAPH_UNDEFINED_VAL) {
            // This segment does not have any outgoing edges to other general routing wires
            continue;
        }

        // Need to make sure all wire switches of a given wire segment type have the same 'buffered' value
        if (switches_buffered[cost_index] == LIBRRGRAPH_UNDEFINED_VAL) {
            switches_buffered[cost_index] = buffered;
        } else {
            if (switches_buffered[cost_index] != buffered) {
                // If a previous buffering state is inconsistent with the current one,
                // the node should be treated as buffered, as there are only two possible
                // values for the buffering state (except for the UNDEFINED case).
                //
                // This means that at least one edge of this node has a buffered switch,
                // which prevails over unbuffered ones.
                switches_buffered[cost_index] = 1;
            }
        }
    }

    unsigned num_occurences_of_no_instances_with_cost_index = 0;
    for (size_t cost_index = CHANX_COST_INDEX_START; cost_index < rr_indexed_data.size(); cost_index++) {
        if (num_nodes_of_index[RRIndexedDataId(cost_index)] == 0) { // Segments don't exist.
            rr_indexed_data[RRIndexedDataId(cost_index)].T_linear = 0.0;
            rr_indexed_data[RRIndexedDataId(cost_index)].T_quadratic = 0.0;
            rr_indexed_data[RRIndexedDataId(cost_index)].C_load = 0.0;
            num_occurences_of_no_instances_with_cost_index++;
        } else {
            auto C_total_histogram = build_histogram(C_total[RRIndexedDataId(cost_index)], 10);
            auto R_total_histogram = build_histogram(R_total[RRIndexedDataId(cost_index)], 10);
            auto switch_R_total_histogram = build_histogram(switch_R_total[RRIndexedDataId(cost_index)], 10);
            auto switch_T_total_histogram = build_histogram(switch_T_total[RRIndexedDataId(cost_index)], 10);
            auto switch_Cinternal_total_histogram = build_histogram(switch_Cinternal_total[RRIndexedDataId(cost_index)], 10);

            // Sort Rnode and Cnode
            std::sort(C_total[RRIndexedDataId(cost_index)].begin(), C_total[RRIndexedDataId(cost_index)].end());
            std::sort(R_total[RRIndexedDataId(cost_index)].begin(), R_total[RRIndexedDataId(cost_index)].end());
            float Cnode = vtr::median_presorted<float>(C_total[RRIndexedDataId(cost_index)]);
            float Rnode = vtr::median_presorted<float>(R_total[RRIndexedDataId(cost_index)]);
            float Rsw = get_histogram_mode(switch_R_total_histogram);
            float Tsw = get_histogram_mode(switch_T_total_histogram);
            float Cinternalsw = get_histogram_mode(switch_Cinternal_total_histogram);

            if (switches_buffered[RRIndexedDataId(cost_index)]) {
                // Here, we are computing the linear time delay for buffered switches. Tlinear is
                // the estimated sum of the intrinsic time delay of the switch and the two transient
                // responses. The key assumption behind the estimate is that one switch will be turned on
                // from each wire and so we will correspondingly add one load for internal capacitance.
                // The first transient response is the product between the resistance of the switch with
                // the combined capacitance of the node and internal capacitance of the switch. The
                // multiplication by the second term by 0.5 is the result of the Rnode being distributed halfway along a
                // wire segment's length times the total capacitance.
                rr_indexed_data[RRIndexedDataId(cost_index)].T_linear = Tsw + Rsw * (Cinternalsw + Cnode)
                                                                        + 0.5 * Rnode * (Cnode + Cinternalsw);
                rr_indexed_data[RRIndexedDataId(cost_index)].T_quadratic = 0.;
                rr_indexed_data[RRIndexedDataId(cost_index)].C_load = 0.;
            } else { // Pass transistor, does not have an internal capacitance
                rr_indexed_data[RRIndexedDataId(cost_index)].C_load = Cnode;

                /* See Dec. 23, 1997 notes for deriviation of formulae. */

                rr_indexed_data[RRIndexedDataId(cost_index)].T_linear = Tsw + 0.5 * Rsw * Cnode;
                rr_indexed_data[RRIndexedDataId(cost_index)].T_quadratic = (Rsw + Rnode) * 0.5 * Cnode;
            }
        }
    }
    if (num_occurences_of_no_instances_with_cost_index > 0) {
        VTR_LOG_WARN("Found %u cost indices where no instances of RR nodes could be found\n",
                     num_occurences_of_no_instances_with_cost_index);
    }
}

static void calculate_average_switch(const RRGraphView& rr_graph,
                                     RRNodeId inode,
                                     double& avg_switch_R,
                                     double& avg_switch_T,
                                     double& avg_switch_Cinternal,
                                     int& num_switches,
                                     int& num_shorts,
                                     short& buffered,
                                     vtr::vector<RRNodeId, std::vector<RREdgeId>>& fan_in_list) {

    avg_switch_R = 0;
    avg_switch_T = 0;
    avg_switch_Cinternal = 0;
    num_switches = 0;
    num_shorts = 0;
    buffered = LIBRRGRAPH_UNDEFINED_VAL;
    
    for (const RREdgeId edge : fan_in_list[inode]) {
        // Want to get C/R/Tdel/Cinternal of switches that connect this track segment to other track segments
        e_rr_type node_type = rr_graph.node_type(inode);

        if (is_chanxy(node_type) || is_chanz(node_type)) {
            RRSwitchId switch_index = (RRSwitchId)rr_graph.rr_nodes().edge_switch(edge);

            if (rr_graph.rr_switch_inf(switch_index).type() == e_switch_type::SHORT) {
                num_shorts++;
                continue;
            }

            avg_switch_R += rr_graph.rr_switch_inf(switch_index).R;
            avg_switch_T += rr_graph.rr_switch_inf(switch_index).Tdel;
            avg_switch_Cinternal += rr_graph.rr_switch_inf(switch_index).Cinternal;

            if (buffered == LIBRRGRAPH_UNDEFINED_VAL) {
                if (rr_graph.rr_switch_inf(switch_index).buffered()) {
                    buffered = 1;
                } else {
                    buffered = 0;
                }
            } else if (buffered != rr_graph.rr_switch_inf((switch_index)).buffered()) {
                // If a previous buffering state is inconsistent with the current one,
                // the node should be treated as buffered, as there are only two possible
                // values for the buffering state (except for the UNDEFINED case).
                //
                // This means that at least one edge of this node has a buffered switch,
                // which prevails over unbuffered ones.
                buffered = 1;
            }

            num_switches++;
        }
    }

    if (num_switches > 0) {
        double inv_num_switches = 1.0 / num_switches;
        avg_switch_R *= inv_num_switches;
        avg_switch_T *= inv_num_switches;
        avg_switch_Cinternal *= inv_num_switches;
    }

    VTR_ASSERT(std::isfinite(avg_switch_R));
    VTR_ASSERT(std::isfinite(avg_switch_T));
    VTR_ASSERT(std::isfinite(avg_switch_Cinternal));
}

static void fixup_rr_indexed_data_T_values(vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                           size_t total_num_segments) {
    // Scan CHANX/CHANY indexed data and search for uninitialized costs.
    //
    // This would occur if a segment ends up only being used as CHANX or a
    // CHANY, but not both. If this occurs, then copying the orthogonal
    // pair's cost data is likely a better choice than leaving it uninitialized.
    //
    // The primary reason for this fixup is to avoid propagating negative
    // values in cost functions.
    for (size_t cost_index = CHANX_COST_INDEX_START; cost_index < CHANX_COST_INDEX_START + total_num_segments; cost_index++) {
        int ortho_cost_index = rr_indexed_data[RRIndexedDataId(cost_index)].ortho_cost_index;

        // CHANZ segments don't have orthogonal cost indices
        if (ortho_cost_index < 0) {
            continue;
        }

        auto& indexed_data = rr_indexed_data[RRIndexedDataId(cost_index)];
        auto& ortho_indexed_data = rr_indexed_data[RRIndexedDataId(ortho_cost_index)];
        // Check if this data is uninitialized, but the orthogonal data is
        // initialized.
        // Uninitialized data is set to zero by default.
        bool needs_fixup = indexed_data.T_linear == 0 && indexed_data.T_quadratic == 0 && indexed_data.C_load == 0;
        bool ortho_data_valid = ortho_indexed_data.T_linear != 0 || ortho_indexed_data.T_quadratic != 0 || ortho_indexed_data.C_load != 0;
        if (needs_fixup && ortho_data_valid) {
            // Copy orthogonal data over.
            indexed_data.T_linear = ortho_indexed_data.T_linear;
            indexed_data.T_quadratic = ortho_indexed_data.T_quadratic;
            indexed_data.C_load = ortho_indexed_data.C_load;
        }
    }
}

static void print_rr_index_info(const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                const char* fname,
                                const std::vector<t_segment_inf>& segment_inf,
                                size_t y_chan_cost_offset,
                                size_t z_chan_cost_offset) {
    std::ofstream out_file;

    out_file.open(fname, std::ios_base::app);
    out_file << std::left << std::setw(30) << "Cost Index";
    out_file << std::left << std::setw(20) << "Base Cost";
    out_file << std::left << std::setw(20) << "Ortho Cost Index";
    out_file << std::left << std::setw(20) << "Seg Index";
    out_file << std::left << std::setw(20) << "Inv. Length";
    out_file << std::left << std::setw(20) << "T. Linear";
    out_file << std::left << std::setw(20) << "T. Quadratic";
    out_file << std::left << std::setw(20) << "C. Load" << std::endl;
    for (size_t cost_index = 0; cost_index < rr_indexed_data.size(); ++cost_index) {
        const t_rr_indexed_data& index_data = rr_indexed_data[RRIndexedDataId(cost_index)];

        std::ostringstream string_stream;

        if (cost_index == SOURCE_COST_INDEX) {
            string_stream << cost_index << " SOURCE";
        } else if (cost_index == SINK_COST_INDEX) {
            string_stream << cost_index << " SINK";
        } else if (cost_index == MUX_COST_INDEX) {
            string_stream << cost_index << " MUX";
        } else if (cost_index == OPIN_COST_INDEX) {
            string_stream << cost_index << " OPIN";
        } else if (cost_index == IPIN_COST_INDEX) {
            string_stream << cost_index << " IPIN";
        } else if (cost_index <= IPIN_COST_INDEX + y_chan_cost_offset) {
            string_stream << cost_index << " CHANX " << segment_inf[index_data.seg_index].name;
        } else if (cost_index <= IPIN_COST_INDEX + z_chan_cost_offset){
            string_stream << cost_index << " CHANY " << segment_inf[index_data.seg_index].name;
        } else {
            string_stream << cost_index << " CHANZ " << segment_inf[index_data.seg_index].name;
        }

        std::string cost_index_str = string_stream.str();

        out_file << std::left << std::setw(30) << cost_index_str;
        out_file << std::left << std::setw(20) << index_data.base_cost;
        out_file << std::left << std::setw(20) << index_data.ortho_cost_index;
        out_file << std::left << std::setw(20) << index_data.seg_index;
        out_file << std::left << std::setw(20) << index_data.inv_length;
        out_file << std::left << std::setw(20) << index_data.T_linear;
        out_file << std::left << std::setw(20) << index_data.T_quadratic;
        out_file << std::left << std::setw(20) << index_data.C_load << std::endl;
    }

    out_file.close();
}
