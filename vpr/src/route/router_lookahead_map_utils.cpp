#include "router_lookahead_map_utils.h"

#include "globals.h"
#include "vpr_context.h"
#include "vtr_math.h"

/* Number of CLBs I think the average conn. goes. */
static const int CLB_DIST = 3;

util::PQ_Entry::PQ_Entry(
    int set_rr_node_ind,
    int switch_ind,
    float parent_delay,
    float parent_R_upstream,
    float parent_congestion_upstream,
    bool starting_node) {
    this->rr_node_ind = set_rr_node_ind;

    auto& device_ctx = g_vpr_ctx.device();
    this->delay = parent_delay;
    this->congestion_upstream = parent_congestion_upstream;
    this->R_upstream = parent_R_upstream;
    if (!starting_node) {
        int cost_index = device_ctx.rr_nodes[set_rr_node_ind].cost_index();

        float Tsw = device_ctx.rr_switch_inf[switch_ind].Tdel;
        float Rsw = device_ctx.rr_switch_inf[switch_ind].R;
        float Cnode = device_ctx.rr_nodes[set_rr_node_ind].C();
        float Rnode = device_ctx.rr_nodes[set_rr_node_ind].R();

        float T_linear = 0.f;
        float T_quadratic = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].buffered()) {
            T_linear = Tsw + Rsw * Cnode + 0.5 * Rnode * Cnode;
            T_quadratic = 0.;
        } else { /* Pass transistor */
            T_linear = Tsw + 0.5 * Rsw * Cnode;
            T_quadratic = (Rsw + Rnode) * 0.5 * Cnode;
        }

        float base_cost;
        if (device_ctx.rr_indexed_data[cost_index].inv_length < 0) {
            base_cost = device_ctx.rr_indexed_data[cost_index].base_cost;
        } else {
            float frac_num_seg = CLB_DIST * device_ctx.rr_indexed_data[cost_index].inv_length;

            base_cost = frac_num_seg * T_linear
                        + frac_num_seg * frac_num_seg * T_quadratic;
        }

        VTR_ASSERT(T_linear >= 0.);
        VTR_ASSERT(base_cost >= 0.);
        this->delay += T_linear;

        this->congestion_upstream += base_cost;
    }

    /* set the cost of this node */
    this->cost = this->delay;
}

/* returns cost entry with the smallest delay */
Cost_Entry Expansion_Cost_Entry::get_smallest_entry() const {
    Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (!smallest_entry.valid() || entry.delay < smallest_entry.delay) {
            smallest_entry = entry;
        }
    }

    return smallest_entry;
}

/* returns a cost entry that represents the average of all the recorded entries */
Cost_Entry Expansion_Cost_Entry::get_average_entry() const {
    float avg_delay = 0;
    float avg_congestion = 0;

    for (auto cost_entry : this->cost_vector) {
        avg_delay += cost_entry.delay;
        avg_congestion += cost_entry.congestion;
    }

    avg_delay /= (float)this->cost_vector.size();
    avg_congestion /= (float)this->cost_vector.size();

    return Cost_Entry(avg_delay, avg_congestion);
}

/* returns a cost entry that represents the geomean of all the recorded entries */
Cost_Entry Expansion_Cost_Entry::get_geomean_entry() const {
    float geomean_delay = 0;
    float geomean_cong = 0;
    for (auto cost_entry : this->cost_vector) {
        geomean_delay += log(cost_entry.delay);
        geomean_cong += log(cost_entry.congestion);
    }

    geomean_delay = exp(geomean_delay / (float)this->cost_vector.size());
    geomean_cong = exp(geomean_cong / (float)this->cost_vector.size());

    return Cost_Entry(geomean_delay, geomean_cong);
}

/* returns a cost entry that represents the medial of all recorded entries */
Cost_Entry Expansion_Cost_Entry::get_median_entry() const {
    /* find median by binning the delays of all entries and then chosing the bin
     * with the largest number of entries */

    int num_bins = 10;

    /* find entries with smallest and largest delays */
    Cost_Entry min_del_entry;
    Cost_Entry max_del_entry;
    for (auto entry : this->cost_vector) {
        if (!min_del_entry.valid() || entry.delay < min_del_entry.delay) {
            min_del_entry = entry;
        }
        if (!max_del_entry.valid() || entry.delay > max_del_entry.delay) {
            max_del_entry = entry;
        }
    }

    /* get the bin size */
    float delay_diff = max_del_entry.delay - min_del_entry.delay;
    float bin_size = delay_diff / (float)num_bins;

    /* sort the cost entries into bins */
    std::vector<std::vector<Cost_Entry> > entry_bins(num_bins, std::vector<Cost_Entry>());
    for (auto entry : this->cost_vector) {
        float bin_num = floor((entry.delay - min_del_entry.delay) / bin_size);

        VTR_ASSERT(vtr::nint(bin_num) >= 0 && vtr::nint(bin_num) <= num_bins);
        if (vtr::nint(bin_num) == num_bins) {
            /* largest entry will otherwise have an out-of-bounds bin number */
            bin_num -= 1;
        }
        entry_bins[vtr::nint(bin_num)].push_back(entry);
    }

    /* find the bin with the largest number of elements */
    int largest_bin = 0;
    int largest_size = 0;
    for (int ibin = 0; ibin < num_bins; ibin++) {
        if (entry_bins[ibin].size() > (unsigned)largest_size) {
            largest_bin = ibin;
            largest_size = (unsigned)entry_bins[ibin].size();
        }
    }

    /* get the representative delay of the largest bin */
    Cost_Entry representative_entry = entry_bins[largest_bin][0];

    return representative_entry;
}

/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
void expand_dijkstra_neighbours(util::PQ_Entry parent_entry,
                                std::vector<float>& node_visited_costs,
                                std::vector<bool>& node_expanded,
                                std::priority_queue<util::PQ_Entry>& pq) {
    auto& device_ctx = g_vpr_ctx.device();

    int parent_ind = parent_entry.rr_node_ind;

    auto& parent_node = device_ctx.rr_nodes[parent_ind];

    for (int iedge = 0; iedge < parent_node.num_edges(); iedge++) {
        int child_node_ind = parent_node.edge_sink_node(iedge);
        int switch_ind = parent_node.edge_switch(iedge);

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node_ind]) {
            continue;
        }

        util::PQ_Entry child_entry(child_node_ind, switch_ind, parent_entry.delay,
                                   parent_entry.R_upstream, parent_entry.congestion_upstream, false);

        VTR_ASSERT(child_entry.cost >= 0);

        /* skip this child if it has been visited with smaller cost */
        if (node_visited_costs[child_node_ind] >= 0 && node_visited_costs[child_node_ind] < child_entry.cost) {
            continue;
        }

        /* finally, record the cost with which the child was visited and put the child entry on the queue */
        node_visited_costs[child_node_ind] = child_entry.cost;
        pq.push(child_entry);
    }
}
