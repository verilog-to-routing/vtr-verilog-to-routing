#include <cmath> /* Needed only for sqrt call (remove if sqrt removed) */

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph.h"
#include "rr_graph_indexed_data.h"
#include "read_xml_arch_file.h"

/******************* Subroutines local to this module ************************/

static void load_rr_indexed_data_base_costs(int nodes_per_chan,
                                            const t_rr_node_indices& L_rr_node_indices,
                                            enum e_base_cost_type base_cost_type);

static float get_delay_normalization_fac(int nodes_per_chan,
                                         const t_rr_node_indices& L_rr_node_indices);

static void load_rr_indexed_data_T_values(int index_start,
                                          int num_indices_to_load,
                                          t_rr_type rr_type,
                                          int nodes_per_chan,
                                          const t_rr_node_indices& L_rr_node_indices);

static void calculate_average_switch(int inode, double& avg_switch_R, double& avg_switch_T, double& avg_switch_Cinternal, int& num_switches, short& buffered);

static void fixup_rr_indexed_data_T_values(size_t num_segment);

static std::vector<size_t> count_rr_segment_types();

static void print_rr_index_info(const std::vector<t_segment_inf>& segment_inf);

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
                                    const t_rr_node_indices& L_rr_node_indices,
                                    const int nodes_per_chan,
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
    load_rr_indexed_data_T_values(CHANX_COST_INDEX_START, num_segment, CHANX,
                                  nodes_per_chan, L_rr_node_indices);

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
    load_rr_indexed_data_T_values((CHANX_COST_INDEX_START + num_segment),
                                  num_segment, CHANY, nodes_per_chan, L_rr_node_indices);

    fixup_rr_indexed_data_T_values(num_segment);

    load_rr_indexed_data_base_costs(nodes_per_chan, L_rr_node_indices,
                                    base_cost_type);

    if (false) print_rr_index_info(segment_inf);
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

static void load_rr_indexed_data_base_costs(int nodes_per_chan,
                                            const t_rr_node_indices& L_rr_node_indices,
                                            enum e_base_cost_type base_cost_type) {
    /* Loads the base_cost member of device_ctx.rr_indexed_data according to the specified *
     * base_cost_type.                                                          */

    float delay_normalization_fac;
    size_t index;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (base_cost_type == DEMAND_ONLY || base_cost_type == DEMAND_ONLY_NORMALIZED_LENGTH) {
        delay_normalization_fac = 1.;
    } else {
        delay_normalization_fac = get_delay_normalization_fac(nodes_per_chan, L_rr_node_indices);
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

static float get_delay_normalization_fac(int nodes_per_chan,
                                         const t_rr_node_indices& L_rr_node_indices) {
    /* Returns the average delay to go 1 CLB distance along a wire.  */

    const int clb_dist = 3; /* Number of CLBs I think the average conn. goes. */

    int inode, itrack, cost_index;
    float Tdel, Tdel_sum, frac_num_seg;

    auto& device_ctx = g_vpr_ctx.device();

    Tdel_sum = 0.;

    for (itrack = 0; itrack < nodes_per_chan; itrack++) {
        inode = find_average_rr_node_index(device_ctx.grid.width(), device_ctx.grid.height(), CHANX, itrack,
                                           L_rr_node_indices);
        if (inode == -1)
            continue;
        cost_index = device_ctx.rr_nodes[inode].cost_index();
        frac_num_seg = clb_dist * device_ctx.rr_indexed_data[cost_index].inv_length;
        Tdel = frac_num_seg * device_ctx.rr_indexed_data[cost_index].T_linear
               + frac_num_seg * frac_num_seg
                     * device_ctx.rr_indexed_data[cost_index].T_quadratic;
        Tdel_sum += Tdel / (float)clb_dist;
    }

    for (itrack = 0; itrack < nodes_per_chan; itrack++) {
        inode = find_average_rr_node_index(device_ctx.grid.width(), device_ctx.grid.height(), CHANY, itrack,
                                           L_rr_node_indices);
        if (inode == -1)
            continue;
        cost_index = device_ctx.rr_nodes[inode].cost_index();
        frac_num_seg = clb_dist * device_ctx.rr_indexed_data[cost_index].inv_length;
        Tdel = frac_num_seg * device_ctx.rr_indexed_data[cost_index].T_linear
               + frac_num_seg * frac_num_seg
                     * device_ctx.rr_indexed_data[cost_index].T_quadratic;
        Tdel_sum += Tdel / (float)clb_dist;
    }

    return (Tdel_sum / (2. * nodes_per_chan));
}

static void load_rr_indexed_data_T_values(int index_start,
                                          int num_indices_to_load,
                                          t_rr_type rr_type,
                                          int nodes_per_chan,
                                          const t_rr_node_indices& L_rr_node_indices) {
    /* Loads the average propagation times through segments of each index type   *
     * for either all CHANX segment types or all CHANY segment types.  It does   *
     * this by looking at all the segments in one channel in the middle of the   *
     * array and averaging the R, C, and Cinternal values of all segments of the *
     * same type and using them to compute average delay values for this type of *
     * segment. */

    int itrack, cost_index;
    float *C_total, *R_total;                                         /* [0..device_ctx.rr_indexed_data.size() - 1] */
    double *switch_R_total, *switch_T_total, *switch_Cinternal_total; /* [0..device_ctx.rr_indexed_data.size() - 1] */
    short* switches_buffered;
    int* num_nodes_of_index; /* [0..device_ctx.rr_indexed_data.size() - 1] */
    float Rnode, Cnode, Rsw, Tsw, Cinternalsw;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    num_nodes_of_index = (int*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(int));
    C_total = (float*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(float));
    R_total = (float*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(float));

    /* August 2014: Not all wire-to-wire switches connecting from some wire segment will
     * necessarily have the same delay. i.e. a mux with less inputs will have smaller delay
     * than a mux with a greater number of inputs. So to account for these differences we will
     * get the average R/Tdel/Cinternal values by first averaging them for a single wire segment
     * (first for loop below), and then by averaging this value over all wire segments in the channel
     * (second for loop below) */
    switch_R_total = (double*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(double));
    switch_T_total = (double*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(double));
    switch_Cinternal_total = (double*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(double));
    switches_buffered = (short*)vtr::calloc(device_ctx.rr_indexed_data.size(), sizeof(short));

    /* initialize switches_buffered array */
    for (int i = index_start; i < index_start + num_indices_to_load; i++) {
        switches_buffered[i] = UNDEFINED;
    }

    /* Get average C and R values for all the segments of this type in one      *
     * channel segment, near the middle of the fpga.                            */

    for (itrack = 0; itrack < nodes_per_chan; itrack++) {
        int inode = find_average_rr_node_index(device_ctx.grid.width(), device_ctx.grid.height(), rr_type, itrack,
                                               L_rr_node_indices);
        if (inode == -1)
            continue;
        cost_index = device_ctx.rr_nodes[inode].cost_index();
        num_nodes_of_index[cost_index]++;
        C_total[cost_index] += device_ctx.rr_nodes[inode].C();
        R_total[cost_index] += device_ctx.rr_nodes[inode].R();

        /* get average switch parameters */
        double avg_switch_R = 0;
        double avg_switch_T = 0;
        double avg_switch_Cinternal = 0;
        int num_switches = 0;
        short buffered = UNDEFINED;
        calculate_average_switch(inode, avg_switch_R, avg_switch_T, avg_switch_Cinternal, num_switches, buffered);

        if (num_switches == 0) {
            VTR_LOG_WARN("Track %d had no out-going switches\n", itrack);
            continue;
        }
        VTR_ASSERT(num_switches > 0);

        switch_R_total[cost_index] += avg_switch_R;
        switch_T_total[cost_index] += avg_switch_T;
        switch_Cinternal_total[cost_index] += avg_switch_Cinternal;
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

    for (cost_index = index_start;
         cost_index < index_start + num_indices_to_load; cost_index++) {
        if (num_nodes_of_index[cost_index] != 0) continue;

        // Did not find segments of this type
        //'Unusual' wire types (e.g. clock networks) which aren't in every
        //channel won't get picked up by the above, so try an exhaustive search to
        //find them

        for (int inode = 0; inode < int(device_ctx.rr_nodes.size()); ++inode) {
            if (num_nodes_of_index[cost_index] > nodes_per_chan) break; //Sampled (more than) enough

            if (device_ctx.rr_nodes[inode].cost_index() != cost_index) continue;

            ++num_nodes_of_index[cost_index];

            C_total[cost_index] += device_ctx.rr_nodes[inode].C();
            R_total[cost_index] += device_ctx.rr_nodes[inode].R();

            double avg_switch_R = 0;
            double avg_switch_T = 0;
            double avg_switch_Cinternal = 0;
            int num_switches = 0;
            short buffered = UNDEFINED;
            calculate_average_switch(inode, avg_switch_R, avg_switch_T, avg_switch_Cinternal, num_switches, buffered);

            switch_R_total[cost_index] += avg_switch_R;
            switch_T_total[cost_index] += avg_switch_T;
            switch_Cinternal_total[cost_index] += avg_switch_Cinternal;
        }
    }

    for (cost_index = index_start;
         cost_index < index_start + num_indices_to_load; cost_index++) {
        if (num_nodes_of_index[cost_index] == 0) { /* Segments don't exist. */
            VTR_LOG_WARN("Found no instances of RR node with cost index %d\n", cost_index);
            device_ctx.rr_indexed_data[cost_index].T_linear = std::numeric_limits<float>::quiet_NaN();
            device_ctx.rr_indexed_data[cost_index].T_quadratic = std::numeric_limits<float>::quiet_NaN();
            device_ctx.rr_indexed_data[cost_index].C_load = std::numeric_limits<float>::quiet_NaN();
        } else {
            Rnode = R_total[cost_index] / num_nodes_of_index[cost_index];
            Cnode = C_total[cost_index] / num_nodes_of_index[cost_index];
            Rsw = (float)switch_R_total[cost_index] / num_nodes_of_index[cost_index];
            Tsw = (float)switch_T_total[cost_index] / num_nodes_of_index[cost_index];
            Cinternalsw = (float)switch_Cinternal_total[cost_index] / num_nodes_of_index[cost_index];

            if (switches_buffered[cost_index]) {
                // Here, we are computing the linear time delay for buffered switches. Tlinear is
                // the estimated sum of the intrinsic time delay of the switch and the two transient
                // responses. The key assumption behind the estimate is that one switch will be turned on
                // from each wire and so we will correspondingly add one load for internal capacitance.
                // The first transient response is the product between the resistance of the switch with
                // the combined capacitance of the node and internal capacitance of the switch. The
                // second transient response is the result of the Rnode being distributed halfway along a
                // wire segment's length times the total capacitance.
                device_ctx.rr_indexed_data[cost_index].T_linear = Tsw + Rsw * (Cinternalsw + Cnode)
                                                                  + 0.5 * Rnode * (Cnode + Cinternalsw);
                device_ctx.rr_indexed_data[cost_index].T_quadratic = 0.;
                device_ctx.rr_indexed_data[cost_index].C_load = 0.;
            } else { /* Pass transistor, does not have an internal capacitance*/
                device_ctx.rr_indexed_data[cost_index].C_load = Cnode;

                /* See Dec. 23, 1997 notes for deriviation of formulae. */

                device_ctx.rr_indexed_data[cost_index].T_linear = Tsw + 0.5 * Rsw * Cnode;
                device_ctx.rr_indexed_data[cost_index].T_quadratic = (Rsw + Rnode) * 0.5
                                                                     * Cnode;
            }
        }
    }

    free(num_nodes_of_index);
    free(C_total);
    free(R_total);
    free(switch_R_total);
    free(switch_T_total);
    free(switch_Cinternal_total);
    free(switches_buffered);
}

static void calculate_average_switch(int inode, double& avg_switch_R, double& avg_switch_T, double& avg_switch_Cinternal, int& num_switches, short& buffered) {
    auto& device_ctx = g_vpr_ctx.device();
    int num_edges = device_ctx.rr_nodes[inode].num_edges();
    avg_switch_R = 0;
    avg_switch_T = 0;
    avg_switch_Cinternal = 0;
    num_switches = 0;
    buffered = UNDEFINED;
    for (int iedge = 0; iedge < num_edges; iedge++) {
        int to_node_index = device_ctx.rr_nodes[inode].edge_sink_node(iedge);
        /* want to get C/R/Tdel/Cinternal of switches that connect this track segment to other track segments */
        if (device_ctx.rr_nodes[to_node_index].type() == CHANX || device_ctx.rr_nodes[to_node_index].type() == CHANY) {
            int switch_index = device_ctx.rr_nodes[inode].edge_switch(iedge);
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

        // Check if this data is uninitialized, but the orthogonal data is
        // initialized.
        if (!std::isfinite(device_ctx.rr_indexed_data[cost_index].T_linear)
            && std::isfinite(device_ctx.rr_indexed_data[ortho_cost_index].T_linear)) {
            // Copy orthogonal data over.
            device_ctx.rr_indexed_data[cost_index].T_linear = device_ctx.rr_indexed_data[ortho_cost_index].T_linear;
            device_ctx.rr_indexed_data[cost_index].T_quadratic = device_ctx.rr_indexed_data[ortho_cost_index].T_quadratic;
            device_ctx.rr_indexed_data[cost_index].C_load = device_ctx.rr_indexed_data[ortho_cost_index].C_load;
        }
    }
}

static void print_rr_index_info(const std::vector<t_segment_inf>& segment_inf) {
    auto& device_ctx = g_vpr_ctx.device();

    for (size_t cost_index = 0; cost_index < device_ctx.rr_indexed_data.size(); ++cost_index) {
        auto& index_data = device_ctx.rr_indexed_data[cost_index];

        if (cost_index == SOURCE_COST_INDEX) {
            VTR_LOG("Cost Index %zu SOURCE\n", cost_index);
        } else if (cost_index == SINK_COST_INDEX) {
            VTR_LOG("Cost Index %zu SINK\n", cost_index);
        } else if (cost_index == OPIN_COST_INDEX) {
            VTR_LOG("Cost Index %zu OPIN\n", cost_index);
        } else if (cost_index == IPIN_COST_INDEX) {
            VTR_LOG("Cost Index %zu IPIN\n", cost_index);
        } else if (cost_index <= IPIN_COST_INDEX + segment_inf.size()) {
            VTR_LOG("Cost Index %zu CHANX %s\n", cost_index, segment_inf[index_data.seg_index].name.c_str());
        } else {
            VTR_LOG("Cost Index %zu CHANY %s\n", cost_index, segment_inf[index_data.seg_index].name.c_str());
        }

        VTR_LOG("\tbase_cost=%g\n", index_data.base_cost);
        VTR_LOG("\tortho_cost_index=%d\n", index_data.ortho_cost_index);
        VTR_LOG("\tseg_index=%d\n", index_data.seg_index);
        VTR_LOG("\tinv_length=%g\n", index_data.inv_length);
        VTR_LOG("\tT_linear=%g\n", index_data.T_linear);
        VTR_LOG("\tT_quadratic=%g\n", index_data.T_quadratic);
        VTR_LOG("\tC_load=%g\n", index_data.C_load);
    }
}
