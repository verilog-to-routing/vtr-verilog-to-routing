#include <cmath>
#include <set>

#include "physical_types_util.h"
#include "route_tree.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "rr_graph_area.h"
#include "segment_stats.h"
#include "channel_stats.h"
#include "stats.h"

/********************** Subroutines local to this module *********************/

static void load_channel_occupancies(const Netlist<>& net_list,
                                     vtr::Matrix<int>& chanx_occ,
                                     vtr::Matrix<int>& chany_occ);

static void length_and_bends_stats(const Netlist<>& net_list, bool is_flat);

static void get_channel_occupancy_stats(const Netlist<>& net_list, bool /***/);

/************************* Subroutine definitions ****************************/

/**
 * @brief Prints out various statistics about the current routing.
 *
 * Both a routing and an rr_graph must exist when you call this routine.
 */
void routing_stats(const Netlist<>& net_list,
                   bool full_stats,
                   enum e_route_type route_type,
                   std::vector<t_segment_inf>& segment_inf,
                   float R_minW_nmos,
                   float R_minW_pmos,
                   float grid_logic_tile_area,
                   enum e_directionality directionality,
                   int wire_to_ipin_switch,
                   bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = g_vpr_ctx.placement().block_locs();

    int num_rr_switch = rr_graph.num_rr_switches();

    length_and_bends_stats(net_list, is_flat);
    print_channel_stats(is_flat);
    get_channel_occupancy_stats(net_list, is_flat);

    VTR_LOG("Logic area (in minimum width transistor areas, excludes I/Os and empty grid tiles)...\n");

    float area = 0;
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                auto type = device_ctx.grid.get_physical_type({i, j, layer_num});
                int width_offset = device_ctx.grid.get_width_offset({i, j, layer_num});
                int height_offset = device_ctx.grid.get_height_offset({i, j, layer_num});
                if (width_offset == 0
                    && height_offset == 0
                    && !is_io_type(type)
                    && type != device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                    if (type->area == UNDEFINED) {
                        area += grid_logic_tile_area * type->width * type->height;
                    } else {
                        area += type->area;
                    }
                }
            }
        }
    }
    /* Todo: need to add pitch of routing to blocks with height > 3 */
    VTR_LOG("\tTotal logic block area (Warning, need to add pitch of routing to blocks with height > 3): %g\n", area);

    float used_area = 0;
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_pl_loc block_loc = block_locs[blk_id].loc;
        auto type = physical_tile_type(block_loc);
        if (!is_io_type(type)) {
            if (type->area == UNDEFINED) {
                used_area += grid_logic_tile_area * type->width * type->height;
            } else {
                used_area += type->area;
            }
        }
    }
    VTR_LOG("\tTotal used logic block area: %g\n", used_area);

    if (route_type == DETAILED) {
        count_routing_transistors(directionality, num_rr_switch, wire_to_ipin_switch,
                                  segment_inf, R_minW_nmos, R_minW_pmos, is_flat);
        get_segment_usage_stats(segment_inf);
    }

    if (full_stats) {
        print_wirelen_prob_dist(is_flat);
    }
}

/**
 * @brief Figures out maximum, minimum and average number of bends
 *        and net length in the routing.
 */
void length_and_bends_stats(const Netlist<>& net_list, bool is_flat) {
    int max_bends = 0;
    int total_bends = 0;
    int max_length = 0;
    int total_length = 0;
    int max_segments = 0;
    int total_segments = 0;
    int num_global_nets = 0;
    int num_clb_opins_reserved = 0;
    int num_absorbed_nets = 0;

    for (auto net_id : net_list.nets()) {
        if (!net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0) { /* Globals don't count. */
            int bends, length, segments;
            bool is_absorbed;
            get_num_bends_and_length(net_id, &bends, &length, &segments, &is_absorbed);

            total_bends += bends;
            max_bends = std::max(bends, max_bends);

            total_length += length;
            max_length = std::max(length, max_length);

            total_segments += segments;
            max_segments = std::max(segments, max_segments);

            if (is_absorbed) {
                num_absorbed_nets++;
            }
        } else if (net_list.net_is_ignored(net_id)) {
            num_global_nets++;
        } else if (!is_flat) {
            /* If flat_routing is enabled, we don't need to count the number of reserved opins*/
            num_clb_opins_reserved++;
        }
    }

    float av_bends = (float)total_bends / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("\n");
    VTR_LOG("Average number of bends per net: %#g  Maximum # of bends: %d\n", av_bends, max_bends);
    VTR_LOG("\n");

    float av_length = (float)total_length / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Number of global nets: %d\n", num_global_nets);
    VTR_LOG("Number of routed nets (nonglobal): %d\n", (int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Wire length results (in units of 1 clb segments)...\n");
    VTR_LOG("\tTotal wirelength: %d, average net length: %#g\n", total_length, av_length);
    VTR_LOG("\tMaximum net length: %d\n", max_length);
    VTR_LOG("\n");

    float av_segments = (float)total_segments / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Wire length results in terms of physical segments...\n");
    VTR_LOG("\tTotal wiring segments used: %d, average wire segments per net: %#g\n", total_segments, av_segments);
    VTR_LOG("\tMaximum segments used by a net: %d\n", max_segments);
    VTR_LOG("\tTotal local nets with reserved CLB opins: %d\n", num_clb_opins_reserved);

    VTR_LOG("Total number of nets absorbed: %d\n", num_absorbed_nets);
}

///@brief Determines how many tracks are used in each channel.
static void get_channel_occupancy_stats(const Netlist<>& net_list, bool /***/) {
    auto& device_ctx = g_vpr_ctx.device();

    auto chanx_occ = vtr::Matrix<int>({{
                                          device_ctx.grid.width(),     //[0 .. device_ctx.grid.width() - 1] (length of x channel)
                                          device_ctx.grid.height() - 1 //[0 .. device_ctx.grid.height() - 2] (# x channels)
                                      }},
                                      0);

    auto chany_occ = vtr::Matrix<int>({{
                                          device_ctx.grid.width() - 1, //[0 .. device_ctx.grid.width() - 2] (# y channels)
                                          device_ctx.grid.height()     //[0 .. device_ctx.grid.height() - 1] (length of y channel)
                                      }},
                                      0);
    load_channel_occupancies(net_list, chanx_occ, chany_occ);

    VTR_LOG("\n");
    VTR_LOG("X - Directed channels:   j max occ ave occ capacity\n");
    VTR_LOG("                      ---- ------- ------- --------\n");

    int total_x = 0;
    for (size_t j = 0; j < device_ctx.grid.height() - 1; ++j) {
        total_x += device_ctx.chan_width.x_list[j];
        float ave_occ = 0.0;
        int max_occ = -1;

        for (size_t i = 1; i < device_ctx.grid.width(); ++i) {
            max_occ = std::max(chanx_occ[i][j], max_occ);
            ave_occ += chanx_occ[i][j];
        }
        ave_occ /= device_ctx.grid.width();
        VTR_LOG("                      %4d %7d %7.3f %8d\n", j, max_occ, ave_occ, device_ctx.chan_width.x_list[j]);
    }

    VTR_LOG("Y - Directed channels:   i max occ ave occ capacity\n");
    VTR_LOG("                      ---- ------- ------- --------\n");

    int total_y = 0;
    for (size_t i = 0; i < device_ctx.grid.width() - 1; ++i) {
        total_y += device_ctx.chan_width.y_list[i];
        float ave_occ = 0.0;
        int max_occ = -1;

        for (size_t j = 1; j < device_ctx.grid.height(); ++j) {
            max_occ = std::max(chany_occ[i][j], max_occ);
            ave_occ += chany_occ[i][j];
        }
        ave_occ /= device_ctx.grid.height();
        VTR_LOG("                      %4d %7d %7.3f %8d\n", i, max_occ, ave_occ, device_ctx.chan_width.y_list[i]);
    }

    VTR_LOG("\n");
    VTR_LOG("Total tracks in x-direction: %d, in y-direction: %d\n", total_x, total_y);
    VTR_LOG("\n");
}

/**
 * @brief Loads the two arrays passed in with the total occupancy at each of the
 *        channel segments in the FPGA.
 */
static void load_channel_occupancies(const Netlist<>& net_list,
                                     vtr::Matrix<int>& chanx_occ,
                                     vtr::Matrix<int>& chany_occ) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    /* First set the occupancy of everything to zero. */
    chanx_occ.fill(0);
    chany_occ.fill(0);

    /* Now go through each net and count the tracks and pins used everywhere */
    for (auto net_id : net_list.nets()) {
        /* Skip global and empty nets. */
        if (net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0)
            continue;

        auto& tree = route_ctx.route_trees[net_id];
        if (!tree)
            continue;

        for (auto& rt_node : tree.value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            t_rr_type rr_type = rr_graph.node_type(inode);

            if (rr_type == CHANX) {
                int j = rr_graph.node_ylow(inode);
                for (int i = rr_graph.node_xlow(inode); i <= rr_graph.node_xhigh(inode); i++)
                    chanx_occ[i][j]++;
            } else if (rr_type == CHANY) {
                int i = rr_graph.node_xlow(inode);
                for (int j = rr_graph.node_ylow(inode); j <= rr_graph.node_yhigh(inode); j++)
                    chany_occ[i][j]++;
            }
        }
    }
}

/**
 * @brief Counts and returns the number of bends, wirelength, and number of routing
 *        resource segments in net inet's routing.
 */
void get_num_bends_and_length(ParentNetId inet, int* bends_ptr, int* len_ptr, int* segments_ptr, bool* is_absorbed_ptr) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int bends, length, segments;

    bends = 0;
    length = 0;
    segments = 0;

    const vtr::optional<RouteTree>& tree = route_ctx.route_trees[inet];
    if (!tree) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "in get_num_bends_and_length: net #%lu has no routing.\n", size_t(inet));
    }

    t_rr_type prev_type = rr_graph.node_type(tree->root().inode);
    RouteTree::iterator it = tree->all_nodes().begin();
    RouteTree::iterator end = tree->all_nodes().end();
    ++it; /* start from the next node after source */

    for (; it != end; ++it) {
        const RouteTreeNode& rt_node = *it;
        RRNodeId inode = rt_node.inode;
        t_rr_type curr_type = rr_graph.node_type(inode);

        if (curr_type == CHANX || curr_type == CHANY) {
            segments++;
            length += rr_graph.node_length(inode);

            if (curr_type != prev_type && (prev_type == CHANX || prev_type == CHANY))
                bends++;
        }

        /* The all_nodes iterator walks all nodes in the tree. If we are at a leaf and going back to the top, prev_type is invalid: just set it to SINK */
        prev_type = rt_node.is_leaf() ? SINK : curr_type;
    }

    *bends_ptr = bends;
    *len_ptr = length;
    *segments_ptr = segments;
    *is_absorbed_ptr = (segments == 0);
}

/**
 * @brief Prints out the probability distribution of the wirelength / number
 *        input pins on a net -- i.e. simulates 2-point net length probability
 *        distribution.
 */
void print_wirelen_prob_dist(bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    float norm_fac, two_point_length;
    int bends, length, segments, index;
    float av_length;
    int prob_dist_size, incr;
    bool is_absorbed;

    prob_dist_size = device_ctx.grid.width() + device_ctx.grid.height() + 10;
    std::vector<float> prob_dist(prob_dist_size, 0.0);
    norm_fac = 0.;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto par_net_id = get_cluster_net_parent_id(g_vpr_ctx.atom().lookup(), net_id, is_flat);
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id) && cluster_ctx.clb_nlist.net_sinks(net_id).size() != 0) {
            get_num_bends_and_length(par_net_id, &bends, &length, &segments, &is_absorbed);

            /*  Assign probability to two integer lengths proportionately -- i.e.  *
             *  if two_point_length = 1.9, add 0.9 of the pins to prob_dist[2] and *
             *  only 0.1 to prob_dist[1].                                          */

            int num_sinks = cluster_ctx.clb_nlist.net_sinks(net_id).size();
            VTR_ASSERT(num_sinks > 0);

            two_point_length = (float)length / (float)(num_sinks);
            index = (int)two_point_length;
            if (index >= prob_dist_size) {
                VTR_LOG_WARN("Index (%d) to prob_dist exceeds its allocated size (%d).\n",
                             index, prob_dist_size);
                VTR_LOG("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");

                /*  Resized to prob_dist + incr. Elements after old prob_dist_size set
                 *   to 0.0.  */
                incr = index - prob_dist_size + 2;
                prob_dist_size += incr;
                prob_dist.resize(prob_dist_size);
            }
            prob_dist[index] += (num_sinks) * (1 - two_point_length + index);

            index++;
            if (index >= prob_dist_size) {
                VTR_LOG_WARN("Index (%d) to prob_dist exceeds its allocated size (%d).\n",
                             index, prob_dist_size);
                VTR_LOG("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");
                incr = index - prob_dist_size + 2;
                prob_dist_size += incr;

                prob_dist.resize(prob_dist_size);
            }
            prob_dist[index] += (num_sinks) * (1 - index + two_point_length);

            norm_fac += num_sinks;
        }
    }

    /* Normalize so total probability is 1 and print out. */

    VTR_LOG("\n");
    VTR_LOG("Probability distribution of 2-pin net lengths:\n");
    VTR_LOG("\n");
    VTR_LOG("Length    p(Lenth)\n");

    av_length = 0;

    for (index = 0; index < prob_dist_size; index++) {
        prob_dist[index] /= norm_fac;
        VTR_LOG("%6d  %10.6f\n", index, prob_dist[index]);
        av_length += prob_dist[index] * index;
    }

    VTR_LOG("\n");
    VTR_LOG("Number of 2-pin nets: ;%g;\n", norm_fac);
    VTR_LOG("Expected value of 2-pin net length (R): ;%g;\n", av_length);
    VTR_LOG("Total wirelength: ;%g;\n", norm_fac * av_length);
}

/**
 * @brief  Finds the average number of input pins used per clb.
 *
 * Does not count inputs which are hooked to global nets
 * (i.e. the clock when it is marked global).
 */
void print_lambda() {
    int num_inputs_used = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_pl_loc block_loc = block_locs[blk_id].loc;
        auto type = physical_tile_type(block_loc);
        VTR_ASSERT(type != nullptr);
        if (!is_io_type(type)) {
            for (int ipin = 0; ipin < type->num_pins; ipin++) {
                if (get_pin_type_from_pin_physical_num(type, ipin) == RECEIVER) {
                    ClusterNetId net_id = cluster_ctx.clb_nlist.block_net(blk_id, ipin);
                    if (net_id != ClusterNetId::INVALID())                 /* Pin is connected? */
                        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) /* Not a global clock */
                            num_inputs_used++;
                }
            }
        }
    }

    float lambda = (float)num_inputs_used / (float)cluster_ctx.clb_nlist.blocks().size();
    VTR_LOG("Average lambda (input pins used per clb) is: %g\n", lambda);
}

///@brief Count how many clocks are in the netlist.
int count_netlist_clocks() {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::set<std::string> clock_names;

    //Loop through each clock pin and record the names in clock_names
    for (auto blk_id : atom_ctx.netlist().blocks()) {
        for (auto pin_id : atom_ctx.netlist().block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.netlist().pin_net(pin_id);
            clock_names.insert(atom_ctx.netlist().net_name(net_id));
        }
    }

    //Since std::set does not include duplicates, the number of clocks is the size of the set
    return static_cast<int>(clock_names.size());
}
