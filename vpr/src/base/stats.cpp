
#include "stats.h"

#include "stats_utils.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <string>
#include <iomanip>
#include <tuple>

#include "physical_types.h"
#include "physical_types_util.h"
#include "route_tree.h"
#include "vpr_utils.h"
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

#include "crr_common.h"

namespace fs = std::filesystem;

/************************* Subroutine definitions ****************************/

void routing_stats(const Netlist<>& net_list,
                   bool full_stats,
                   e_route_type route_type,
                   std::vector<t_segment_inf>& segment_inf,
                   float R_minW_nmos,
                   float R_minW_pmos,
                   float grid_logic_tile_area,
                   e_directionality directionality,
                   bool is_flat) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = g_vpr_ctx.placement().block_locs();

    int num_rr_switch = rr_graph.num_rr_switches();

    length_and_bends_stats(net_list, is_flat);
    print_channel_stats(is_flat);
    get_channel_occupancy_stats(net_list);

    VTR_LOG("Logic area (in minimum width transistor areas, excludes I/Os and empty grid tiles)...\n");

    float area = 0;

    for (const t_physical_tile_loc tile_loc : device_ctx.grid.all_locations()) {
        t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type(tile_loc);
        int width_offset = device_ctx.grid.get_width_offset(tile_loc);
        int height_offset = device_ctx.grid.get_height_offset(tile_loc);
        if (width_offset == 0 && height_offset == 0 && !type->is_io() && !type->is_empty()) {
            if (type->area == UNDEFINED) {
                area += grid_logic_tile_area * type->width * type->height;
            } else {
                area += type->area;
            }
        }
    }
    /* Todo: need to add pitch of routing to blocks with height > 3 */
    VTR_LOG("\tTotal logic block area (Warning, need to add pitch of routing to blocks with height > 3): %g\n", area);

    float used_area = 0;
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_pl_loc block_loc = block_locs[blk_id].loc;
        auto type = physical_tile_type(block_loc);
        if (!type->is_io()) {
            if (type->area == UNDEFINED) {
                used_area += grid_logic_tile_area * type->width * type->height;
            } else {
                used_area += type->area;
            }
        }
    }
    VTR_LOG("\tTotal used logic block area: %g\n", used_area);

    if (route_type == e_route_type::DETAILED) {
        count_routing_transistors(directionality,
                                  num_rr_switch,
                                  segment_inf,
                                  R_minW_nmos,
                                  R_minW_pmos,
                                  is_flat);
        get_segment_usage_stats(segment_inf);
    }

    if (full_stats) {
        print_wirelen_prob_dist(is_flat);
        print_tap_utilization(net_list, segment_inf);
    }
}

void write_sb_count_stats(const Netlist<>& net_list,
                          const std::string& sb_map_dir,
                          const std::string& sb_count_dir) {
    const RRGraphView& rr_graph = g_vpr_ctx.device().rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    std::unordered_map<std::string, int> sb_count;

    // Check if the sb_count_dir exists and is a director
    // If not, create it
    if (!fs::exists(sb_count_dir)) {
        fs::create_directories(sb_count_dir);
    }
    if (!fs::is_directory(sb_count_dir)) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "sb_count_dir is not a directory: %s\n", sb_count_dir.c_str());
    }

    for (ParentNetId net_id : net_list.nets()) {
        if (!net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0) {
            const vtr::optional<RouteTree>& tree = route_ctx.route_trees[net_id];
            if (!tree) {
                continue;
            }

            for (const RouteTreeNode& rt_node : tree.value().all_nodes()) {
                auto parent = rt_node.parent();
                // Skip the root node
                if (!parent) {
                    continue;
                }

                const RouteTreeNode& parent_rt_node = parent.value();

                RRNodeId src_node = parent_rt_node.inode;
                RRNodeId sink_node = rt_node.inode;
                std::vector<RREdgeId> edges = rr_graph.find_edges(src_node, sink_node);
                VTR_ASSERT(edges.size() == 1);
                RRSwitchId switch_id = RRSwitchId(rr_graph.edge_switch(edges[0]));
                const std::string& sw_template_id = rr_graph.rr_switch_inf(switch_id).template_id;
                if (sw_template_id.empty()) {
                    continue;
                }
                if (sb_count.find(sw_template_id) == sb_count.end()) {
                    sb_count[sw_template_id] = 0;
                }
                sb_count[sw_template_id]++;
            }
        }
    }

    // Write the sb_count to a file
    // First, read all CSV files from directory and trim them
    std::unordered_map<std::string, std::vector<std::vector<std::string>>> csv_data;

    for (const auto& entry : fs::directory_iterator(sb_map_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string filename = entry.path().filename().string();
            csv_data[filename] = read_and_trim_csv(entry.path().string());
        }
    }

    // Group sb_count entries by filename
    std::unordered_map<std::string, std::vector<std::tuple<int, int, int>>> file_groups;

    for (const auto& [sb_id, count] : sb_count) {
        SBKeyParts parts = parse_sb_key(sb_id);
        file_groups[parts.filename].push_back({parts.row, parts.col, count});
    }

    // Process each file
    for (auto& [filename, data] : csv_data) {
        // Check if this file has updates from sb_count
        if (file_groups.find(filename) == file_groups.end()) {
            // No updates for this file, just write trimmed version
            std::string output_path = sb_count_dir + "/" + filename;
            write_csv(output_path, data);
            VTR_LOG("Written trimmed CSV: %s\n", filename.c_str());
            continue;
        }

        const auto& entries = file_groups[filename];

        // Find maximum row and column needed
        int max_row = 0, max_col = 0;
        for (const auto& entry : entries) {
            max_row = std::max(max_row, std::get<0>(entry));
            max_col = std::max(max_col, std::get<1>(entry));
        }

        // Expand data structure if needed
        while (data.size() <= static_cast<size_t>(max_row)) {
            data.push_back(std::vector<std::string>());
        }

        for (auto& row : data) {
            while (row.size() <= static_cast<size_t>(max_col)) {
                row.push_back("");
            }
        }

        // Update values from sb_count
        for (const auto& entry : entries) {
            int row = std::get<0>(entry);
            int col = std::get<1>(entry);
            int count = std::get<2>(entry);
            data[row][col] = std::to_string(count);
        }

        // Write updated file
        std::string output_path = sb_count_dir + "/" + filename;
        write_csv(output_path, data);
        VTR_LOG("Written switchbox counts to: %s\n", filename.c_str());
    }
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

    // Normalize so total probability is 1 and print out.

    VTR_LOG("\n");
    VTR_LOG("Probability distribution of 2-pin net lengths:\n");
    VTR_LOG("\n");
    VTR_LOG("Length    p(Length)\n");

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
        if (!type->is_io()) {
            for (int ipin = 0; ipin < type->num_pins; ipin++) {
                if (get_pin_type_from_pin_physical_num(type, ipin) == e_pin_type::RECEIVER) {
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

float calculate_device_utilization(const DeviceGrid& grid, const std::map<t_logical_block_type_ptr, size_t>& instance_counts) {
    // Record the resources of the grid
    std::map<t_physical_tile_type_ptr, size_t> grid_resources;

    for (const t_physical_tile_loc tile_loc : grid.all_locations()) {
        int width_offset = grid.get_width_offset(tile_loc);
        int height_offset = grid.get_height_offset(tile_loc);
        if (width_offset == 0 && height_offset == 0) {
            const t_physical_tile_type_ptr type = grid.get_physical_type(tile_loc);
            ++grid_resources[type];
        }
    }

    // Determine the area of grid in tile units
    float grid_area = 0.;
    for (auto& kv : grid_resources) {
        t_physical_tile_type_ptr type = kv.first;
        size_t count = kv.second;

        float type_area = type->width * type->height;

        grid_area += type_area * count;
    }

    // Determine the area of instances in tile units
    float instance_area = 0.;
    for (auto& kv : instance_counts) {
        if (is_empty_type(kv.first)) {
            continue;
        }

        t_physical_tile_type_ptr type = pick_physical_type(kv.first);

        size_t count = kv.second;

        float type_area = type->width * type->height;

        //Instances of multi-capaicty blocks take up less space
        if (type->capacity != 0) {
            type_area /= type->capacity;
        }

        instance_area += type_area * count;
    }

    float utilization = instance_area / grid_area;

    return utilization;
}

void print_resource_usage() {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& clb_netlist = g_vpr_ctx.clustering().clb_nlist;
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    for (auto blk_id : clb_netlist.blocks()) {
        num_type_instances[clb_netlist.block_type(blk_id)]++;
    }

    VTR_LOG("\n");
    VTR_LOG("Resource usage...\n");
    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) continue;
        size_t num_instances = num_type_instances.count(&type) > 0 ? num_type_instances.at(&type) : 0;
        VTR_LOG("\tNetlist\n\t\t%d\tblocks of type: %s\n",
                num_instances, type.name.c_str());

        VTR_LOG("\tArchitecture\n");
        for (const auto equivalent_tile : type.equivalent_tiles) {
            //get the number of equivalent tile across all layers
            num_instances = device_ctx.grid.num_instances(equivalent_tile, -1);

            VTR_LOG("\t\t%d\tblocks of type: %s\n",
                    num_instances, equivalent_tile->name.c_str());
        }
    }
    VTR_LOG("\n");
}

void print_device_utilization(const float target_device_utilization) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& clb_netlist = g_vpr_ctx.clustering().clb_nlist;
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    for (auto blk_id : clb_netlist.blocks()) {
        num_type_instances[clb_netlist.block_type(blk_id)]++;
    }

    float device_utilization = calculate_device_utilization(device_ctx.grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    for (const auto& type : device_ctx.physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        if (device_ctx.grid.num_instances(&type, -1) != 0) {
            VTR_LOG("\tPhysical Tile %s:\n", type.name.c_str());

            auto equivalent_sites = get_equivalent_sites_set(&type);

            for (auto logical_block : equivalent_sites) {
                float util = 0.;
                size_t num_inst = device_ctx.grid.num_instances(&type, -1);
                if (num_inst != 0) {
                    size_t num_netlist_instances = num_type_instances.count(logical_block) > 0 ? num_type_instances.at(logical_block) : 0;
                    util = float(num_netlist_instances) / num_inst;
                }
                VTR_LOG("\tBlock Utilization: %.2f Logical Block: %s\n", util, logical_block->name.c_str());
            }
        }
    }
    VTR_LOG("\n");

    if (!device_ctx.grid.limiting_resources().empty()) {
        std::vector<std::string> limiting_block_names;
        for (auto blk_type : device_ctx.grid.limiting_resources()) {
            limiting_block_names.emplace_back(blk_type->name);
        }
        VTR_LOG("FPGA size limited by block type(s): %s\n", vtr::join(limiting_block_names, " ").c_str());
        VTR_LOG("\n");
    }
}
