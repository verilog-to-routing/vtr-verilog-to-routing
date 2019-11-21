#include "router_lookahead_map_utils.h"

#include "globals.h"
#include "vpr_context.h"
#include "vtr_math.h"
#include "route_common.h"

namespace util {

/* Number of CLBs I think the average conn. goes. */
static const int CLB_DIST = 3;

PQ_Entry::PQ_Entry(
    int set_rr_node_ind,
    int switch_ind,
    float parent_delay,
    float parent_R_upstream,
    float parent_congestion_upstream,
    bool starting_node,
    float Tsw_adjust) {
    this->rr_node_ind = set_rr_node_ind;

    auto& device_ctx = g_vpr_ctx.device();
    this->delay = parent_delay;
    this->congestion_upstream = parent_congestion_upstream;
    this->R_upstream = parent_R_upstream;
    if (!starting_node) {
        float Tsw = device_ctx.rr_switch_inf[switch_ind].Tdel;
        Tsw += Tsw_adjust;
        VTR_ASSERT(Tsw >= 0.f);
        float Rsw = device_ctx.rr_switch_inf[switch_ind].R;
        float Cnode = device_ctx.rr_nodes[set_rr_node_ind].C();
        float Rnode = device_ctx.rr_nodes[set_rr_node_ind].R();

        float T_linear = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].buffered()) {
            T_linear = Tsw + Rsw * Cnode + 0.5 * Rnode * Cnode;
        } else { /* Pass transistor */
            T_linear = Tsw + 0.5 * Rsw * Cnode;
        }

        float base_cost = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].configurable()) {
            base_cost = get_rr_cong_cost(set_rr_node_ind);
        }

        VTR_ASSERT(T_linear >= 0.);
        VTR_ASSERT(base_cost >= 0.);
        this->delay += T_linear;

        this->congestion_upstream += base_cost;
    }

    /* set the cost of this node */
    this->cost = this->delay;
}

util::PQ_Entry_Delay::PQ_Entry_Delay(
    int set_rr_node_ind,
    int switch_ind,
    const util::PQ_Entry_Delay* parent) {
    this->rr_node_ind = set_rr_node_ind;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        float Tsw = device_ctx.rr_switch_inf[switch_ind].Tdel;
        float Rsw = device_ctx.rr_switch_inf[switch_ind].R;
        float Cnode = device_ctx.rr_nodes[set_rr_node_ind].C();
        float Rnode = device_ctx.rr_nodes[set_rr_node_ind].R();

        float T_linear = 0.f;
        if (device_ctx.rr_switch_inf[switch_ind].buffered()) {
            T_linear = Tsw + Rsw * Cnode + 0.5 * Rnode * Cnode;
        } else { /* Pass transistor */
            T_linear = Tsw + 0.5 * Rsw * Cnode;
        }

        VTR_ASSERT(T_linear >= 0.);
        this->delay_cost = parent->delay_cost + T_linear;
    } else {
        this->delay_cost = 0.f;
    }
}

util::PQ_Entry_Base_Cost::PQ_Entry_Base_Cost(
    int set_rr_node_ind,
    int switch_ind,
    const util::PQ_Entry_Base_Cost* parent) {
    this->rr_node_ind = set_rr_node_ind;

    if (parent != nullptr) {
        auto& device_ctx = g_vpr_ctx.device();
        this->base_cost = parent->base_cost + (device_ctx.rr_switch_inf[switch_ind].configurable() ? get_rr_cong_cost(set_rr_node_ind) : 0);
    } else {
        this->base_cost = 0.f;
    }
}

/* returns cost entry with the smallest delay */
util::Cost_Entry util::Expansion_Cost_Entry::get_smallest_entry() const {
    util::Cost_Entry smallest_entry;

    for (auto entry : this->cost_vector) {
        if (!smallest_entry.valid() || entry.delay < smallest_entry.delay) {
            smallest_entry = entry;
        }
    }

    return smallest_entry;
}

/* returns a cost entry that represents the average of all the recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_average_entry() const {
    float avg_delay = 0;
    float avg_congestion = 0;

    for (auto cost_entry : this->cost_vector) {
        avg_delay += cost_entry.delay;
        avg_congestion += cost_entry.congestion;
    }

    avg_delay /= (float)this->cost_vector.size();
    avg_congestion /= (float)this->cost_vector.size();

    return util::Cost_Entry(avg_delay, avg_congestion);
}

/* returns a cost entry that represents the geomean of all the recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_geomean_entry() const {
    float geomean_delay = 0;
    float geomean_cong = 0;
    for (auto cost_entry : this->cost_vector) {
        geomean_delay += log(cost_entry.delay);
        geomean_cong += log(cost_entry.congestion);
    }

    geomean_delay = exp(geomean_delay / (float)this->cost_vector.size());
    geomean_cong = exp(geomean_cong / (float)this->cost_vector.size());

    return util::Cost_Entry(geomean_delay, geomean_cong);
}

/* returns a cost entry that represents the medial of all recorded entries */
util::Cost_Entry util::Expansion_Cost_Entry::get_median_entry() const {
    /* find median by binning the delays of all entries and then chosing the bin
     * with the largest number of entries */

    int num_bins = 10;

    /* find entries with smallest and largest delays */
    util::Cost_Entry min_del_entry;
    util::Cost_Entry max_del_entry;
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
    std::vector<std::vector<util::Cost_Entry>> entry_bins(num_bins, std::vector<util::Cost_Entry>());
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
    util::Cost_Entry representative_entry = entry_bins[largest_bin][0];

    return representative_entry;
}

template<typename Entry>
void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                const Entry& parent_entry,
                                std::unordered_map<int, util::Search_Path>& paths,
                                std::vector<bool>& node_expanded,
                                std::priority_queue<Entry,
                                                    std::vector<Entry>,
                                                    std::greater<Entry>>& pq) {
    int parent_ind = parent_entry.rr_node_ind;

    auto& parent_node = rr_nodes[parent_ind];

    for (int iedge = 0; iedge < parent_node.num_edges(); iedge++) {
        int child_node_ind = parent_node.edge_sink_node(iedge);
        int switch_ind = parent_node.edge_switch(iedge);

        /* skip this child if it has already been expanded from */
        if (node_expanded[child_node_ind]) {
            continue;
        }

        Entry child_entry(child_node_ind, switch_ind, &parent_entry);
        VTR_ASSERT(child_entry.cost() >= 0);
        pq.push(child_entry);

        /* Create (if it doesn't exist) or update (if the new cost is lower)
         * to specified node */
        Search_Path path_entry = {child_entry.cost(), parent_ind, iedge};
        auto result = paths.insert(std::make_pair(
            child_node_ind,
            path_entry));
        if (!result.second) {
            if (child_entry.cost() < result.first->second.cost) {
                result.first->second = path_entry;
            }
        }
    }
}

template void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                         const PQ_Entry_Delay& parent_entry,
                                         std::unordered_map<int, Search_Path>& paths,
                                         std::vector<bool>& node_expanded,
                                         std::priority_queue<PQ_Entry_Delay,
                                                             std::vector<PQ_Entry_Delay>,
                                                             std::greater<PQ_Entry_Delay>>& pq);
template void expand_dijkstra_neighbours(const t_rr_graph_storage& rr_nodes,
                                         const PQ_Entry_Base_Cost& parent_entry,
                                         std::unordered_map<int, Search_Path>& paths,
                                         std::vector<bool>& node_expanded,
                                         std::priority_queue<PQ_Entry_Base_Cost,
                                                             std::vector<PQ_Entry_Base_Cost>,
                                                             std::greater<PQ_Entry_Base_Cost>>& pq);

} // namespace util
