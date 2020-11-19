#include <cmath> /* Needed only for sqrt call (remove if sqrt removed) */
#include <fstream>
#include <iomanip>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph.h"
#include "rr_graph_indexed_data.h"
#include "read_xml_arch_file.h"

#include "histogram.h"

#include "echo_files.h"

/******************* Subroutines local to this module ************************/

static void load_rr_indexed_data_base_costs(enum e_base_cost_type base_cost_type);

static float get_delay_normalization_fac();

static void load_rr_indexed_data_T_values();

static void calculate_average_switch(int inode, double& avg_switch_R, double& avg_switch_T, double& avg_switch_Cinternal, int& num_switches, short& buffered, vtr::vector<RRNodeId, std::vector<RREdgeId>>& fan_in_list);

static void fixup_rr_indexed_data_T_values(size_t num_segment);

static std::vector<size_t> count_rr_segment_types();

static void print_rr_index_info(const char* fname, const std::vector<t_segment_inf>& segment_inf);

/******************** Subroutine definitions *********************************/

/* Allocates the device_ctx.rr_indexed_data array and loads it with appropriate values. *
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
void alloc_and_load_rr_indexed_data(const std::vector<t_segment_inf>& segment_inf,
                                    int wire_to_ipin_switch,
                                    enum e_base_cost_type base_cost_type) {
    int iseg, length, i, index;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    int num_segment = segment_inf.size();
    int num_rr_indexed_data = CHANX_COST_INDEX_START + (2 * num_segment); //2x for CHANX & CHANY
    device_ctx.rr_indexed_data.resize(num_rr_indexed_data);

    /* For rr_types that aren't CHANX or CHANY, base_cost is valid, but most     *
     * * other fields are invalid.  For IPINs, the T_linear field is also valid;   *
     * * all other fields are invalid.  For SOURCES, SINKs and OPINs, all fields   *
     * * other than base_cost are invalid. Mark invalid fields as OPEN for safety. */

    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    for (i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
        device_ctx.rr_indexed_data[i].ortho_cost_index = OPEN;
        device_ctx.rr_indexed_data[i].seg_index = OPEN;
        device_ctx.rr_indexed_data[i].inv_length = nan;
        device_ctx.rr_indexed_data[i].T_linear = 0.;
        device_ctx.rr_indexed_data[i].T_quadratic = 0.;
        device_ctx.rr_indexed_data[i].C_load = 0.;
    }
    device_ctx.rr_indexed_data[IPIN_COST_INDEX].T_linear = device_ctx.rr_switch_inf[wire_to_ipin_switch].Tdel;

    /* X-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + iseg;

        if ((index + num_segment) >= (int)device_ctx.rr_indexed_data.size()) {
            device_ctx.rr_indexed_data[index].ortho_cost_index = index;
        } else {
            device_ctx.rr_indexed_data[index].ortho_cost_index = index + num_segment;
        }

        if (segment_inf[iseg].longline)
            length = device_ctx.grid.width();
        else
            length = std::min<int>(segment_inf[iseg].length, device_ctx.grid.width());

        device_ctx.rr_indexed_data[index].inv_length = 1. / length;
        device_ctx.rr_indexed_data[index].seg_index = iseg;
    }

    /* Y-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + num_segment + iseg;

        if ((index - num_segment) < CHANX_COST_INDEX_START) {
            device_ctx.rr_indexed_data[index].ortho_cost_index = index;
        } else {
            device_ctx.rr_indexed_data[index].ortho_cost_index = index - num_segment;
        }

        if (segment_inf[iseg].longline)
            length = device_ctx.grid.height();
        else
            length = std::min<int>(segment_inf[iseg].length, device_ctx.grid.height());

        device_ctx.rr_indexed_data[index].inv_length = 1. / length;
        device_ctx.rr_indexed_data[index].seg_index = iseg;
    }

    load_rr_indexed_data_T_values();

    fixup_rr_indexed_data_T_values(num_segment);

    load_rr_indexed_data_base_costs(base_cost_type);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA)) {
        print_rr_index_info(getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA),
                            segment_inf);
    }
}

void load_rr_index_segments(const int num_segment) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    int iseg, i, index;

    for (i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
        device_ctx.rr_indexed_data[i].seg_index = OPEN;
    }

    /* X-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + iseg;
        device_ctx.rr_indexed_data[index].seg_index = iseg;
    }
    /* Y-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + num_segment + iseg;
        device_ctx.rr_indexed_data[index].seg_index = iseg;
    }
}

static void load_rr_indexed_data_base_costs(enum e_base_cost_type base_cost_type) {
    /* Loads the base_cost member of device_ctx.rr_indexed_data according to the specified *
     * base_cost_type.                                                          */

    float delay_normalization_fac;
    size_t index;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (base_cost_type == DEMAND_ONLY || base_cost_type == DEMAND_ONLY_NORMALIZED_LENGTH) {
        delay_normalization_fac = 1.;
    } else {
        delay_normalization_fac = get_delay_normalization_fac();
    }

    device_ctx.rr_indexed_data[SOURCE_COST_INDEX].base_cost = delay_normalization_fac;
    device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost = 0.;
    device_ctx.rr_indexed_data[OPIN_COST_INDEX].base_cost = delay_normalization_fac;
    device_ctx.rr_indexed_data[IPIN_COST_INDEX].base_cost = 0.95 * delay_normalization_fac;

    auto rr_segment_counts = count_rr_segment_types();
    size_t total_segments = std::accumulate(rr_segment_counts.begin(), rr_segment_counts.end(), 0u);

    /* Load base costs for CHANX and CHANY segments */
    float max_length = 0;
    float min_length = 1;
    if (base_cost_type == DELAY_NORMALIZED_LENGTH_BOUNDED) {
        for (index = CHANX_COST_INDEX_START; index < device_ctx.rr_indexed_data.size(); index++) {
            float length = (1 / device_ctx.rr_indexed_data[index].inv_length);
            max_length = std::max(max_length, length);
        }
    }

    //Future Work: Since we can now have wire types which don't connect to IPINs,
    //             perhaps consider lowering cost of wires which connect to IPINs
    //             so they get explored earlier (same rational as lowering IPIN costs)

    for (index = CHANX_COST_INDEX_START; index < device_ctx.rr_indexed_data.size(); index++) {
        if (base_cost_type == DELAY_NORMALIZED || base_cost_type == DEMAND_ONLY) {
            device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac;

        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH || base_cost_type == DEMAND_ONLY_NORMALIZED_LENGTH) {
            device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac / device_ctx.rr_indexed_data[index].inv_length;

        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH_BOUNDED) {
            float length = (1 / device_ctx.rr_indexed_data[index].inv_length);
            if (max_length != min_length) {
                float length_scale = 1.f + 3.f * (length - min_length) / (max_length - min_length);
                device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac * length_scale;
            } else {
                device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac;
            }

        } else if (base_cost_type == DELAY_NORMALIZED_FREQUENCY) {
            int seg_index = device_ctx.rr_indexed_data[index].seg_index;

            VTR_ASSERT(total_segments > 0);
            float freq_fac = float(rr_segment_counts[seg_index]) / total_segments;

            device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac / freq_fac;

        } else if (base_cost_type == DELAY_NORMALIZED_LENGTH_FREQUENCY) {
            int seg_index = device_ctx.rr_indexed_data[index].seg_index;

            VTR_ASSERT(total_segments > 0);
            float freq_fac = float(rr_segment_counts[seg_index]) / total_segments;

            //Base cost = delay_norm / (len * freq)
            //device_ctx.rr_indexed_data[index].base_cost = delay_normalization_fac / ((1. / device_ctx.rr_indexed_data[index].inv_length) * freq_fac);

            //Base cost = (delay_norm * len) * (1 + (1-freq))
            device_ctx.rr_indexed_data[index].base_cost = (delay_normalization_fac / device_ctx.rr_indexed_data[index].inv_length) * (1 + (1 - freq_fac));

        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unrecognized base cost type");
        }
    }

    /* Save a copy of the base costs -- if dynamic costing is used by the     *
     * router, the base_cost values will get changed all the time and being   *
     * able to restore them from a saved version is useful.                   */

    for (index = 0; index < device_ctx.rr_indexed_data.size(); index++) {
        device_ctx.rr_indexed_data[index].saved_base_cost = device_ctx.rr_indexed_data[index].base_cost;
    }
}

static std::vector<size_t> count_rr_segment_types() {
    std::vector<size_t> rr_segment_type_counts;

    auto& device_ctx = g_vpr_ctx.device();

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        if (device_ctx.rr_nodes[inode].type() != CHANX && device_ctx.rr_nodes[inode].type() != CHANY) continue;

        int cost_index = device_ctx.rr_nodes[inode].cost_index();

        int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

        VTR_ASSERT(seg_index != OPEN);

        if (seg_index >= int(rr_segment_type_counts.size())) {
            rr_segment_type_counts.resize(seg_index + 1, 0);
        }
        VTR_ASSERT(seg_index < int(rr_segment_type_counts.size()));

        ++rr_segment_type_counts[seg_index];
    }

    return rr_segment_type_counts;
}

static float get_delay_normalization_fac() {
    /* Returns the average delay to go 1 CLB distance along a wire.  */

    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_indexed_data = device_ctx.rr_indexed_data;

    float Tdel_sum = 0.0;
    int Tdel_num = 0;
    for (size_t cost_index = CHANX_COST_INDEX_START; cost_index < rr_indexed_data.size(); cost_index++) {
        float inv_length = device_ctx.rr_indexed_data[cost_index].inv_length;
        float T_value = rr_indexed_data[cost_index].T_linear * inv_length + rr_indexed_data[cost_index].T_quadratic * std::pow(inv_length, 2);

        if (T_value == 0.0) continue;

        Tdel_sum += T_value;
        Tdel_num += 1;
    }

    float delay_norm_fac = Tdel_sum / Tdel_num;

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH_INDEXED_DATA)) {
        std::ofstream out_file;

        out_file.open(getEchoFileName(E_ECHO_RR_GRAPH_INDEXED_DATA));
        out_file << "Delay normalization factor: " << delay_norm_fac << std::endl;

        out_file.close();
    }

    return delay_norm_fac;
}

static void load_rr_indexed_data_T_values() {
    /* Loads the average propagation times through segments of each index type   *
     * for either all CHANX segment types or all CHANY segment types.  It does   *
     * this by looking at all the segments in one channel in the middle of the   *
     * array and averaging the R, C, and Cinternal values of all segments of the *
     * same type and using them to compute average delay values for this type of *
     * segment. */

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;
    auto& rr_indexed_data = device_ctx.rr_indexed_data;

    auto fan_in_list = get_fan_in_list();

    std::vector<int> num_nodes_of_index(rr_indexed_data.size(), 0);
    std::vector<std::vector<float>> C_total(rr_indexed_data.size());
    std::vector<std::vector<float>> R_total(rr_indexed_data.size());

    /* August 2014: Not all wire-to-wire switches connecting from some wire segment will
     * necessarily have the same delay. i.e. a mux with less inputs will have smaller delay
     * than a mux with a greater number of inputs. So to account for these differences we will
     * get the average R/Tdel/Cinternal values by first averaging them for a single wire segment
     * (first for loop below), and then by averaging this value over all wire segments in the channel
     * (second for loop below) */
    std::vector<std::vector<float>> switch_R_total(rr_indexed_data.size());
    std::vector<std::vector<float>> switch_T_total(rr_indexed_data.size());
    std::vector<std::vector<float>> switch_Cinternal_total(rr_indexed_data.size());
    std::vector<short> switches_buffered(rr_indexed_data.size(), UNDEFINED);

    /*
     * Walk through the RR graph and collect all R and C values of all the nodes,
     * as well as their fan-in switches R, T_del, and Cinternal values.
     *
     * The median of R and C values for each cost index is assigned to the indexed
     * data.
     */
    for (size_t inode = 0; inode < rr_nodes.size(); inode++) {
        t_rr_type rr_type = rr_nodes[inode].type();

        if (rr_type != CHANX && rr_type != CHANY) {
            continue;
        }

        int cost_index = rr_nodes[inode].cost_index();

        /* get average switch parameters */
        double avg_switch_R = 0;
        double avg_switch_T = 0;
        double avg_switch_Cinternal = 0;
        int num_switches = 0;
        short buffered = UNDEFINED;
        calculate_average_switch(inode, avg_switch_R, avg_switch_T, avg_switch_Cinternal, num_switches, buffered, fan_in_list);

        if (num_switches == 0) {
            VTR_LOG_WARN("Node %d had no out-going switches\n", inode);
            continue;
        }
        VTR_ASSERT(num_switches > 0);

        num_nodes_of_index[cost_index]++;
        C_total[cost_index].push_back(rr_nodes[inode].C());
        R_total[cost_index].push_back(rr_nodes[inode].R());

        switch_R_total[cost_index].push_back(avg_switch_R);
        switch_T_total[cost_index].push_back(avg_switch_T);
        switch_Cinternal_total[cost_index].push_back(avg_switch_Cinternal);
        if (buffered == UNDEFINED) {
            /* this segment does not have any outgoing edges to other general routing wires */
            continue;
        }

        /* need to make sure all wire switches of a given wire segment type have the same 'buffered' value */
        if (switches_buffered[cost_index] == UNDEFINED) {
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

    for (size_t cost_index = CHANX_COST_INDEX_START;
         cost_index < rr_indexed_data.size(); cost_index++) {
        if (num_nodes_of_index[cost_index] == 0) { /* Segments don't exist. */
            VTR_LOG_WARN("Found no instances of RR node with cost index %d\n", cost_index);
            rr_indexed_data[cost_index].T_linear = 0.0;
            rr_indexed_data[cost_index].T_quadratic = 0.0;
            rr_indexed_data[cost_index].C_load = 0.0;
        } else {
            auto C_total_histogram = build_histogram(C_total[cost_index], 10);
            auto R_total_histogram = build_histogram(R_total[cost_index], 10);
            auto switch_R_total_histogram = build_histogram(switch_R_total[cost_index], 10);
            auto switch_T_total_histogram = build_histogram(switch_T_total[cost_index], 10);
            auto switch_Cinternal_total_histogram = build_histogram(switch_Cinternal_total[cost_index], 10);

            // Sort Rnode and Cnode
            float Cnode = vtr::median(C_total[cost_index]);
            float Rnode = vtr::median(R_total[cost_index]);
            float Rsw = get_histogram_mode(switch_R_total_histogram);
            float Tsw = get_histogram_mode(switch_T_total_histogram);
            float Cinternalsw = get_histogram_mode(switch_Cinternal_total_histogram);

            if (switches_buffered[cost_index]) {
                // Here, we are computing the linear time delay for buffered switches. Tlinear is
                // the estimated sum of the intrinsic time delay of the switch and the two transient
                // responses. The key assumption behind the estimate is that one switch will be turned on
                // from each wire and so we will correspondingly add one load for internal capacitance.
                // The first transient response is the product between the resistance of the switch with
                // the combined capacitance of the node and internal capacitance of the switch. The
                // second transient response is the result of the Rnode being distributed halfway along a
                // wire segment's length times the total capacitance.
                rr_indexed_data[cost_index].T_linear = Tsw + Rsw * (Cinternalsw + Cnode)
                                                       + 0.5 * Rnode * (Cnode + Cinternalsw);
                rr_indexed_data[cost_index].T_quadratic = 0.;
                rr_indexed_data[cost_index].C_load = 0.;
            } else { /* Pass transistor, does not have an internal capacitance*/
                rr_indexed_data[cost_index].C_load = Cnode;

                /* See Dec. 23, 1997 notes for deriviation of formulae. */

                rr_indexed_data[cost_index].T_linear = Tsw + 0.5 * Rsw * Cnode;
                rr_indexed_data[cost_index].T_quadratic = (Rsw + Rnode) * 0.5
                                                          * Cnode;
            }
        }
    }
}

static void calculate_average_switch(int inode, double& avg_switch_R, double& avg_switch_T, double& avg_switch_Cinternal, int& num_switches, short& buffered, vtr::vector<RRNodeId, std::vector<RREdgeId>>& fan_in_list) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_nodes = device_ctx.rr_nodes.view();

    auto node = RRNodeId(inode);

    avg_switch_R = 0;
    avg_switch_T = 0;
    avg_switch_Cinternal = 0;
    num_switches = 0;
    buffered = UNDEFINED;
    for (const auto& edge : fan_in_list[node]) {
        /* want to get C/R/Tdel/Cinternal of switches that connect this track segment to other track segments */
        if (rr_nodes.node_type(node) == CHANX || rr_nodes.node_type(node) == CHANY) {
            int switch_index = rr_nodes.edge_switch(edge);

            if (device_ctx.rr_switch_inf[switch_index].type() == SwitchType::SHORT) continue;

            avg_switch_R += device_ctx.rr_switch_inf[switch_index].R;
            avg_switch_T += device_ctx.rr_switch_inf[switch_index].Tdel;
            avg_switch_Cinternal += device_ctx.rr_switch_inf[switch_index].Cinternal;

            if (buffered == UNDEFINED) {
                if (device_ctx.rr_switch_inf[switch_index].buffered()) {
                    buffered = 1;
                } else {
                    buffered = 0;
                }
            } else if (buffered != device_ctx.rr_switch_inf[switch_index].buffered()) {
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
        avg_switch_R /= num_switches;
        avg_switch_T /= num_switches;
        avg_switch_Cinternal /= num_switches;
    }

    VTR_ASSERT(std::isfinite(avg_switch_R));
    VTR_ASSERT(std::isfinite(avg_switch_T));
    VTR_ASSERT(std::isfinite(avg_switch_Cinternal));
}

static void fixup_rr_indexed_data_T_values(size_t num_segment) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    // Scan CHANX/CHANY indexed data and search for uninitialized costs.
    //
    // This would occur if a segment ends up only being used as CHANX or a
    // CHANY, but not both.  If this occurs, then copying the orthogonal
    // pair's cost data is likely a better choice than leaving it uninitialized.
    //
    // The primary reason for this fixup is to avoid propagating negative
    // values in cost functions.
    for (size_t cost_index = CHANX_COST_INDEX_START;
         cost_index < CHANX_COST_INDEX_START + 2 * num_segment; cost_index++) {
        int ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;

        auto& indexed_data = device_ctx.rr_indexed_data[cost_index];
        auto& ortho_indexed_data = device_ctx.rr_indexed_data[ortho_cost_index];
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

static void print_rr_index_info(const char* fname, const std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.device();

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
    for (size_t cost_index = 0; cost_index < device_ctx.rr_indexed_data.size(); ++cost_index) {
        auto& index_data = device_ctx.rr_indexed_data[cost_index];

        std::ostringstream string_stream;

        if (cost_index == SOURCE_COST_INDEX) {
            string_stream << cost_index << " SOURCE";
        } else if (cost_index == SINK_COST_INDEX) {
            string_stream << cost_index << " SINK";
        } else if (cost_index == OPIN_COST_INDEX) {
            string_stream << cost_index << " OPIN";
        } else if (cost_index == IPIN_COST_INDEX) {
            string_stream << cost_index << " IPIN";
        } else if (cost_index <= IPIN_COST_INDEX + segment_inf.size()) {
            string_stream << cost_index << " CHANX " << segment_inf[index_data.seg_index].name.c_str();
        } else {
            string_stream << cost_index << " CHANY " << segment_inf[index_data.seg_index].name.c_str();
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
