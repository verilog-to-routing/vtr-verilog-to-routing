
#include "rr_graph_intra_cluster.h"

#include <list>

#include "globals.h"
#include "vtr_time.h"
#include "vpr_utils.h"
#include "physical_types_util.h"
#include "rr_node_indices.h"
#include "rr_graph_tile_nodes.h"
#include "rr_graph_switch_utils.h"
#include "check_rr_graph.h"

static void set_clusters_pin_chains(const ClusteredNetlist& clb_nlist,
                                    vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                    bool is_flat);

static vtr::vector<ClusterBlockId, std::unordered_set<int>> get_pin_chains_flat(const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains);

static void alloc_and_load_intra_cluster_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const int delayless_switch,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& chain_pin_nums,
                                                  float R_minW_nmos,
                                                  float R_minW_pmos,
                                                  bool is_flat,
                                                  bool load_rr_graph);

/// Create the connection between intra-tile pins
static void add_intra_cluster_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                             t_rr_edge_info_set& rr_edges_to_create,
                                             const DeviceGrid& grid,
                                             const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& nodes_to_collapse,
                                             float R_minW_nmos,
                                             float R_minW_pmos,
                                             int& num_edges,
                                             bool is_flat,
                                             bool load_rr_graph);

/**
 * @brief Add the intra-cluster edges
 * @param num_collapsed_nodes Return the number of nodes that are removed due to collapsing
 * @param cluster_blk_id Cluster block id of the cluster that its edges are being added
 * @param root_loc The root location of the block whose intra-cluster edges are to be added.
 * @param cap Capacity number of the location that cluster is being mapped to
 * @param nodes_to_collapse Store the nodes in the cluster that needs to be collapsed
 */
static void build_cluster_internal_edges(RRGraphBuilder& rr_graph_builder,
                                         int& num_collapsed_nodes,
                                         ClusterBlockId cluster_blk_id,
                                         const t_physical_tile_loc& root_loc,
                                         const int cap,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         const DeviceGrid& grid,
                                         bool is_flat,
                                         bool load_rr_graph);

/// @brief Connect the pins of the given t_pb to their drivers - It doesn't add the edges going in/out of pins on a chain
static void add_pb_edges(RRGraphBuilder& rr_graph_builder,
                         t_rr_edge_info_set& rr_edges_to_create,
                         t_physical_tile_type_ptr physical_type,
                         const t_sub_tile* sub_tile,
                         t_logical_block_type_ptr logical_block,
                         const t_pb* pb,
                         const t_cluster_pin_chain& nodes_to_collapse,
                         float R_minW_nmos,
                         float R_minW_pmos,
                         int rel_cap,
                         const t_physical_tile_loc& root_loc,
                         bool switches_remapped);

/**
 * @brief Edges going in/out of collapse nodes are not added by the normal routine. This function add those edges
 * @return Number of the collapsed nodes
 */
static int add_edges_for_collapsed_nodes(RRGraphBuilder& rr_graph_builder,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         t_physical_tile_type_ptr physical_type,
                                         t_logical_block_type_ptr logical_block,
                                         const std::vector<int>& cluster_pins,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         const t_physical_tile_loc& root_loc,
                                         bool load_rr_graph);

/***
 * @brief Return a pair. The firt element indicates whether the switch is added or it was already added. The second element is the switch index.
 * @param rr_graph
 * @param arch_sw_inf
 * @param R_minW_nmos Needs to be passed to use create_rr_switch_from_arch_switch
 * @param R_minW_pmos Needs to be passed to use create_rr_switch_from_arch_switch
 * @param is_rr_sw If it is true, the function would search in the data structure that store rr switches.
 *                  Otherwise, it would search in the data structure that store switches that are not rr switches.
 * @param delay
 * @return
 */
static std::pair<bool, int> find_create_intra_cluster_sw(RRGraphBuilder& rr_graph,
                                                         std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                                         float R_minW_nmos,
                                                         float R_minW_pmos,
                                                         bool is_rr_sw,
                                                         float delay);

static std::unordered_set<int> get_chain_pins(const std::vector<t_pin_chain_node>& chain);

/// This function is used to add the fan-in edges of the given chain node to the chain's sink with the modified delay
static void add_chain_node_fan_in_edges(RRGraphBuilder& rr_graph_builder,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        int& num_collapsed_pins,
                                        t_physical_tile_type_ptr physical_type,
                                        t_logical_block_type_ptr logical_block,
                                        const t_cluster_pin_chain& nodes_to_collapse,
                                        const std::unordered_set<int>& cluster_pins,
                                        const std::unordered_set<int>& chain_pins,
                                        float R_minW_nmos,
                                        float R_minW_pmos,
                                        int chain_idx,
                                        int node_idx,
                                        const t_physical_tile_loc& root_loc,
                                        bool load_rr_graph);

static t_cluster_pin_chain get_cluster_directly_connected_nodes(const std::vector<int>& cluster_pins,
                                                                t_physical_tile_type_ptr physical_type,
                                                                t_logical_block_type_ptr logical_block,
                                                                bool is_flat);

/// Return the minimum delay to the chain's sink since a pin outside the chain may have connections to multiple pins inside the chain.
static float get_min_delay_to_chain(t_physical_tile_type_ptr physical_type,
                                    t_logical_block_type_ptr logical_block,
                                    const std::unordered_set<int>& cluster_pins,
                                    const std::unordered_set<int>& chain_pins,
                                    int pin_physical_num,
                                    int chain_sink_pin);

static float get_delay_directly_connected_pins(t_physical_tile_type_ptr physical_type,
                                               t_logical_block_type_ptr logical_block,
                                               const std::unordered_set<int>& cluster_pins,
                                               int first_pin_num,
                                               int second_pin_num);

static std::vector<int> get_src_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                t_physical_tile_type_ptr physical_type,
                                                t_logical_block_type_ptr logical_block,
                                                const int pin_physical_num);

/// Returns a chain of nodes starting from pin_physcical_num. All the pins in this chain has a fan-out of 1
static std::vector<int> get_directly_connected_nodes(t_physical_tile_type_ptr physical_type,
                                                     t_logical_block_type_ptr logical_block,
                                                     const std::unordered_set<int>& pins_in_cluster,
                                                     int pin_physical_num,
                                                     bool is_flat);

static bool is_node_chain_sorted(t_physical_tile_type_ptr physical_type,
                                 t_logical_block_type_ptr logical_block,
                                 const std::unordered_set<int>& pins_in_cluster,
                                 const std::vector<int>& pin_index_vec,
                                 const std::vector<std::vector<t_pin_chain_node>>& all_node_chain);

static std::vector<int> get_node_chain_sinks(const std::vector<std::vector<t_pin_chain_node>>& cluster_chains);

static std::vector<int> get_sink_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                 t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 const int pin_physical_num);

static int get_chain_idx(const std::vector<int>& pin_idx_vec, const std::vector<int>& pin_chain, int new_idx);

/// If pin chain is a part of a chain already added to all_chains, add the new parts to the corresponding chain.
/// Otherwise, add pin_chain as a new chain to all_chains.
static void add_pin_chain(const std::vector<int>& pin_chain,
                          int chain_idx,
                          std::vector<int>& pin_index_vec,
                          std::vector<std::vector<t_pin_chain_node>>& all_chains,
                          bool is_new_chain);

static void set_clusters_pin_chains(const ClusteredNetlist& clb_nlist,
                                    vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                    bool is_flat) {
    VTR_ASSERT(is_flat);

    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs = g_vpr_ctx.placement().block_locs();

    for (ClusterBlockId cluster_blk_id : clb_nlist.blocks()) {
        t_pl_loc block_loc = block_locs[cluster_blk_id].loc;
        int abs_cap = block_loc.sub_tile;
        const auto [physical_type, sub_tile, rel_cap, logical_block] = get_cluster_blk_physical_spec(cluster_blk_id);

        std::vector<int> cluster_pins = get_cluster_block_pins(physical_type,
                                                               cluster_blk_id,
                                                               abs_cap);
        // Get the chains of nodes - Each chain would collapse into a single node
        t_cluster_pin_chain nodes_to_collapse = get_cluster_directly_connected_nodes(cluster_pins,
                                                                                     physical_type,
                                                                                     logical_block,
                                                                                     is_flat);
        pin_chains[cluster_blk_id] = std::move(nodes_to_collapse);
    }
}

static vtr::vector<ClusterBlockId, std::unordered_set<int>> get_pin_chains_flat(const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains) {
    vtr::vector<ClusterBlockId, std::unordered_set<int>> chain_pin_nums(pin_chains.size());

    for (int cluster_id_num = 0; cluster_id_num < (int)pin_chains.size(); cluster_id_num++) {
        auto cluster_id = ClusterBlockId(cluster_id_num);
        const std::vector<int>& cluster_pin_chain_num = pin_chains[cluster_id].pin_chain_idx;
        chain_pin_nums[cluster_id].reserve(cluster_pin_chain_num.size());
        for (int pin_num = 0; pin_num < (int)cluster_pin_chain_num.size(); pin_num++) {
            if (cluster_pin_chain_num[pin_num] != UNDEFINED) {
                chain_pin_nums[cluster_id].insert(pin_num);
            }
        }
    }

    return chain_pin_nums;
}

static void alloc_and_load_intra_cluster_rr_graph(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const int delayless_switch,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& chain_pin_nums,
                                                  float R_minW_nmos,
                                                  float R_minW_pmos,
                                                  bool is_flat,
                                                  bool load_rr_graph) {
    t_rr_edge_info_set rr_edges_to_create;
    int num_edges = 0;

    for (const t_physical_tile_loc tile_loc : grid.all_locations()) {
        if (grid.is_root_location(tile_loc)) {
            t_physical_tile_type_ptr physical_tile = grid.get_physical_type(tile_loc);
            std::vector<int> class_num_vec;
            std::vector<int> pin_num_vec;
            class_num_vec = get_cluster_netlist_intra_tile_classes_at_loc(tile_loc, physical_tile);
            pin_num_vec = get_cluster_netlist_intra_tile_pins_at_loc(tile_loc,
                                                                     pin_chains,
                                                                     chain_pin_nums,
                                                                     physical_tile);
            add_classes_rr_graph(rr_graph_builder,
                                 class_num_vec,
                                 tile_loc,
                                 physical_tile);

            add_pins_rr_graph(rr_graph_builder,
                              pin_num_vec,
                              tile_loc,
                              physical_tile);

            connect_src_sink_to_pins(rr_graph_builder,
                                     class_num_vec,
                                     tile_loc,
                                     rr_edges_to_create,
                                     delayless_switch,
                                     physical_tile,
                                     load_rr_graph);

            // Create the actual SOURCE->OPIN, IPIN->SINK edges
            uniquify_edges(rr_edges_to_create);
            rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
            num_edges += rr_edges_to_create.size();
            rr_edges_to_create.clear();
        }
    }

    VTR_LOG("Internal SOURCE->OPIN and IPIN->SINK edge count:%d\n", num_edges);
    num_edges = 0;
    {
        vtr::ScopedStartFinishTimer timer("Adding Internal Edges");
        // Add intra-tile edges
        add_intra_cluster_edges_rr_graph(rr_graph_builder,
                                         rr_edges_to_create,
                                         grid,
                                         pin_chains,
                                         R_minW_nmos,
                                         R_minW_pmos,
                                         num_edges,
                                         is_flat,
                                         load_rr_graph);
    }

    VTR_LOG("Internal edge count:%d\n", num_edges);

    rr_graph_builder.init_fan_in();
}

static void add_intra_cluster_edges_rr_graph(RRGraphBuilder& rr_graph_builder,
                                             t_rr_edge_info_set& rr_edges_to_create,
                                             const DeviceGrid& grid,
                                             const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& nodes_to_collapse,
                                             float R_minW_nmos,
                                             float R_minW_pmos,
                                             int& num_edges,
                                             bool is_flat,
                                             bool load_rr_graph) {
    VTR_ASSERT(is_flat);
    // This function should be called if placement is done!

    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs = g_vpr_ctx.placement().block_locs();
    const ClusteredNetlist& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;

    int num_collapsed_nodes = 0;
    for (ClusterBlockId cluster_blk_id : cluster_net_list.blocks()) {
        t_pl_loc block_loc = block_locs[cluster_blk_id].loc;
        t_physical_tile_loc loc(block_loc.x, block_loc.y, block_loc.layer);
        int abs_cap = block_loc.sub_tile;
        build_cluster_internal_edges(rr_graph_builder,
                                     num_collapsed_nodes,
                                     cluster_blk_id,
                                     loc,
                                     abs_cap,
                                     R_minW_nmos,
                                     R_minW_pmos,
                                     rr_edges_to_create,
                                     nodes_to_collapse[cluster_blk_id],
                                     grid,
                                     is_flat,
                                     load_rr_graph);
        uniquify_edges(rr_edges_to_create);
        rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
        num_edges += rr_edges_to_create.size();
        rr_edges_to_create.clear();
    }

    VTR_LOG("Number of collapsed nodes: %d\n", num_collapsed_nodes);
}

static void build_cluster_internal_edges(RRGraphBuilder& rr_graph_builder,
                                         int& num_collapsed_nodes,
                                         ClusterBlockId cluster_blk_id,
                                         const t_physical_tile_loc& root_loc,
                                         const int abs_cap,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         const DeviceGrid& grid,
                                         bool is_flat,
                                         bool load_rr_graph) {
    VTR_ASSERT(is_flat);
    // Internal edges are added from the start tile
    VTR_ASSERT(grid.is_root_location(root_loc));

    const ClusteredNetlist& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;

    t_physical_tile_type_ptr physical_type;
    const t_sub_tile* sub_tile;
    int rel_cap;
    t_logical_block_type_ptr logical_block;
    std::tie(physical_type, sub_tile, rel_cap, logical_block) = get_cluster_blk_physical_spec(cluster_blk_id);
    VTR_ASSERT(abs_cap < physical_type->capacity);
    VTR_ASSERT(rel_cap >= 0);

    std::vector<int> cluster_pins = get_cluster_block_pins(physical_type, cluster_blk_id, abs_cap);

    const t_pb* pb = cluster_net_list.block_pb(cluster_blk_id);
    std::list<const t_pb*> pb_q;
    pb_q.push_back(pb);

    while (!pb_q.empty()) {
        pb = pb_q.front();
        pb_q.pop_front();

        add_pb_edges(rr_graph_builder,
                     rr_edges_to_create,
                     physical_type,
                     sub_tile,
                     logical_block,
                     pb,
                     nodes_to_collapse,
                     R_minW_nmos,
                     R_minW_pmos,
                     rel_cap,
                     root_loc,
                     load_rr_graph);

        add_pb_child_to_list(pb_q, pb);
    }

    // Edges going in/out of the nodes on the chain are not added by the previous functions, they are added by this function
    num_collapsed_nodes += add_edges_for_collapsed_nodes(rr_graph_builder,
                                                         rr_edges_to_create,
                                                         physical_type,
                                                         logical_block,
                                                         cluster_pins,
                                                         nodes_to_collapse,
                                                         R_minW_nmos,
                                                         R_minW_pmos,
                                                         root_loc,
                                                         load_rr_graph);
}

static void add_pb_edges(RRGraphBuilder& rr_graph_builder,
                         t_rr_edge_info_set& rr_edges_to_create,
                         t_physical_tile_type_ptr physical_type,
                         const t_sub_tile* sub_tile,
                         t_logical_block_type_ptr logical_block,
                         const t_pb* pb,
                         const t_cluster_pin_chain& nodes_to_collapse,
                         float R_minW_nmos,
                         float R_minW_pmos,
                         int rel_cap,
                         const t_physical_tile_loc& root_loc,
                         bool switches_remapped) {
    t_pin_range pin_num_range = get_pb_pins(physical_type, sub_tile, logical_block, pb, rel_cap);

    const std::vector<int>& chain_sinks = nodes_to_collapse.chain_sink;
    const std::vector<int>& pin_chain_idx = nodes_to_collapse.pin_chain_idx;

    for (int pin_physical_num = pin_num_range.low; pin_physical_num <= pin_num_range.high; pin_physical_num++) {
        // The pin belongs to a chain - outgoing edges from this pin will be added later unless it is the sink of the chain
        // If a pin is on tile or it is a primitive pin, then it is not collapsed. Hence, we need to add it's connections. The only connection
        // outgoing from these pins is the connection to the chain sink which will be added later
        int chain_num = pin_chain_idx[pin_physical_num];
        bool primitive_pin = is_primitive_pin(physical_type, pin_physical_num);
        bool pin_on_tile = is_pin_on_tile(physical_type, pin_physical_num);
        if (chain_num != UNDEFINED && chain_sinks[chain_num] != pin_physical_num && !primitive_pin && !pin_on_tile) {
            continue;
        }

        RRNodeId parent_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type, root_loc, pin_physical_num);
        VTR_ASSERT(parent_pin_node_id != RRNodeId::INVALID());

        std::vector<int> conn_pins_physical_num = get_physical_pin_sink_pins(physical_type, logical_block, pin_physical_num);

        for (int conn_pin_physical_num : conn_pins_physical_num) {
            // The pin belongs to a chain - incoming edges to this pin will be added later unless it is the sink of the chain
            int conn_pin_chain_num = pin_chain_idx[conn_pin_physical_num];
            primitive_pin = is_primitive_pin(physical_type, conn_pin_physical_num);
            pin_on_tile = is_pin_on_tile(physical_type, conn_pin_physical_num);
            if (conn_pin_chain_num != UNDEFINED && chain_sinks[conn_pin_chain_num] != conn_pin_physical_num && !primitive_pin && !pin_on_tile) {
                continue;
            }

            RRNodeId conn_pin_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type, root_loc, conn_pin_physical_num);

            // If the node_id is INVALID it means that it belongs to a pin which is not added to the RR Graph. The pin is not added
            // since it belongs to a certain mode or block which is not used in clustered netlist
            if (conn_pin_node_id == RRNodeId::INVALID()) {
                continue;
            }
            int sw_idx = get_edge_sw_arch_idx(physical_type, logical_block, pin_physical_num, conn_pin_physical_num);

            if (switches_remapped) {
                std::map<int, t_arch_switch_inf>& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;
                float delay = g_vpr_ctx.device().all_sw_inf.at(sw_idx).Tdel();
                bool is_new_sw;
                std::tie(is_new_sw, sw_idx) = find_create_intra_cluster_sw(rr_graph_builder,
                                                                           all_sw_inf,
                                                                           R_minW_nmos,
                                                                           R_minW_pmos,
                                                                           switches_remapped,
                                                                           delay);
            }
            rr_edges_to_create.emplace_back(parent_pin_node_id, conn_pin_node_id, sw_idx, switches_remapped);
        }
    }
}

static int add_edges_for_collapsed_nodes(RRGraphBuilder& rr_graph_builder,
                                         t_rr_edge_info_set& rr_edges_to_create,
                                         t_physical_tile_type_ptr physical_type,
                                         t_logical_block_type_ptr logical_block,
                                         const std::vector<int>& cluster_pins,
                                         const t_cluster_pin_chain& nodes_to_collapse,
                                         float R_minW_nmos,
                                         float R_minW_pmos,
                                         const t_physical_tile_loc& root_loc,
                                         bool load_rr_graph) {
    // Store the cluster pins in a set to make the search more run-time efficient
    std::unordered_set<int> cluster_pins_set(cluster_pins.begin(), cluster_pins.end());

    int num_collapsed_pins = 0;
    int num_chain = (int)nodes_to_collapse.chains.size();

    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const std::vector<t_pin_chain_node>& chain = nodes_to_collapse.chains[chain_idx];
        int num_nodes = (int)chain.size();
        VTR_ASSERT(num_nodes > 1);
        std::unordered_set<int> chain_pins = get_chain_pins(nodes_to_collapse.chains[chain_idx]);
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            add_chain_node_fan_in_edges(rr_graph_builder,
                                        rr_edges_to_create,
                                        num_collapsed_pins,
                                        physical_type,
                                        logical_block,
                                        nodes_to_collapse,
                                        cluster_pins_set,
                                        chain_pins,
                                        R_minW_nmos,
                                        R_minW_pmos,
                                        chain_idx,
                                        node_idx,
                                        root_loc,
                                        load_rr_graph);
        }
    }
    return num_collapsed_pins;
}

static std::pair<bool, int> find_create_intra_cluster_sw(RRGraphBuilder& rr_graph,
                                                         std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                                         float R_minW_nmos,
                                                         float R_minW_pmos,
                                                         bool is_rr_sw,
                                                         float delay) {
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_graph_switches = rr_graph.rr_switch();

    if (is_rr_sw) {
        for (int rr_switch_id = 0; rr_switch_id < (int)rr_graph_switches.size(); rr_switch_id++) {
            const t_rr_switch_inf& rr_sw = rr_graph_switches[RRSwitchId(rr_switch_id)];
            if (rr_sw.intra_tile) {
                if (rr_sw.Tdel == delay) {
                    return std::make_pair(false, rr_switch_id);
                }
            }
        }
        t_rr_switch_inf new_rr_switch_inf = create_rr_switch_from_arch_switch(create_internal_arch_sw(delay),
                                                                              R_minW_nmos,
                                                                              R_minW_pmos);
        RRSwitchId rr_sw_id = rr_graph.add_rr_switch(new_rr_switch_inf);

        return std::make_pair(true, (size_t)rr_sw_id);

    } else {
        // Check whether is there any other intra-tile edge with the same delay
        auto find_res = std::find_if(arch_sw_inf.begin(), arch_sw_inf.end(),
                                     [delay](const std::pair<int, t_arch_switch_inf>& sw_inf_pair) {
                                         const t_arch_switch_inf& sw_inf = std::get<1>(sw_inf_pair);
                                         if (sw_inf.intra_tile && sw_inf.Tdel() == delay) {
                                             return true;
                                         } else {
                                             return false;
                                         }
                                     });

        // There isn't any other intra-tile edge with the same delay - Create a new one!
        if (find_res == arch_sw_inf.end()) {
            t_arch_switch_inf arch_sw = create_internal_arch_sw(delay);
            int max_key_num = std::numeric_limits<int>::min();
            // Find the maximum edge index
            for (const auto& arch_sw_pair : arch_sw_inf) {
                if (arch_sw_pair.first > max_key_num) {
                    max_key_num = arch_sw_pair.first;
                }
            }
            int new_key_num = ++max_key_num;
            arch_sw_inf.insert(std::make_pair(new_key_num, arch_sw));
            // We assume that the delay of internal switches is not dependent on their fan-in
            // If this assumption proven to not be accurate, the implementation needs to be changed.
            VTR_ASSERT(arch_sw.fixed_Tdel());

            t_rr_switch_inf new_rr_switch_inf = create_rr_switch_from_arch_switch(create_internal_arch_sw(delay),
                                                                                  R_minW_nmos,
                                                                                  R_minW_pmos);
            RRSwitchId rr_switch_id = rr_graph.add_rr_switch(new_rr_switch_inf);

            // If the switch found inside the cluster has not seen before and RR graph is not read from a file,
            // we need to add this switch to switch_fanin_remap data structure which is used later to remap switch IDs
            // from architecture ID to RR graph switch ID. The reason why we don't this when RR graph is read from a file
            // is that in that case, the switch IDs of edges are already RR graph switch IDs.
            t_arch_switch_fanin& switch_fanin_remap = g_vpr_ctx.mutable_device().switch_fanin_remap;
            switch_fanin_remap.push_back({{UNDEFINED, rr_switch_id}});

            return std::make_pair(true, new_key_num);
        } else {
            return std::make_pair(false, find_res->first);
        }
    }
}

static std::unordered_set<int> get_chain_pins(const std::vector<t_pin_chain_node>& chain) {
    std::unordered_set<int> chain_pins;
    for (t_pin_chain_node node : chain) {
        chain_pins.insert(node.pin_physical_num);
    }
    return chain_pins;
}

static void add_chain_node_fan_in_edges(RRGraphBuilder& rr_graph_builder,
                                        t_rr_edge_info_set& rr_edges_to_create,
                                        int& num_collapsed_pins,
                                        t_physical_tile_type_ptr physical_type,
                                        t_logical_block_type_ptr logical_block,
                                        const t_cluster_pin_chain& nodes_to_collapse,
                                        const std::unordered_set<int>& cluster_pins,
                                        const std::unordered_set<int>& chain_pins,
                                        float R_minW_nmos,
                                        float R_minW_pmos,
                                        int chain_idx,
                                        int node_idx,
                                        const t_physical_tile_loc& root_loc,
                                        bool load_rr_graph) {
    // Chain node pin physical number
    int pin_physical_num = nodes_to_collapse.chains[chain_idx][node_idx].pin_physical_num;
    const std::vector<int>& pin_chain_idx = nodes_to_collapse.pin_chain_idx;
    int sink_pin_num = nodes_to_collapse.chain_sink[chain_idx];

    bool pin_on_tile = is_pin_on_tile(physical_type, pin_physical_num);
    bool primitive_pin = is_primitive_pin(physical_type, pin_physical_num);

    // The delay of the fan-in edges to the chain node is added to the delay of the chain node to the sink. Thus, the information in
    // all_sw_in needs to updated to reflect this change. In other words, if there isn't any edge with the new delay in all_sw_inf, a new member should
    // be added to all_sw_inf.
    std::map<int, t_arch_switch_inf>& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;

    std::unordered_map<RRNodeId, float> src_node_edge_pair;

    // Get the chain's sink node rr node it.
    RRNodeId sink_rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type, root_loc, sink_pin_num);
    VTR_ASSERT(sink_rr_node_id != RRNodeId::INVALID());

    // None of the incoming/outgoing edges of the chain node, except for the chain sink pins, has been added in the previous functions.
    // Incoming/outgoing edges from the chain sink pins have been added in the previous functions.
    if (pin_physical_num != sink_pin_num) {
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);

        // Since the pins on the tile are connected to channels, etc. we do not collapse them into the intra-cluster nodes.
        // Since the primitive pins are connected to SINK/SRC nodes later, we do not collapse them.

        if (primitive_pin || pin_on_tile) {
            // Based on the previous checks, we put these assertions.
            VTR_ASSERT(!primitive_pin || pin_type == e_pin_type::DRIVER);
            VTR_ASSERT(!pin_on_tile || pin_type == e_pin_type::RECEIVER);
            if (pin_on_tile && is_primitive_pin(physical_type, sink_pin_num)) {
                return;
            } else if (primitive_pin && is_pin_on_tile(physical_type, sink_pin_num)) {
                return;
            }

            float chain_delay = get_delay_directly_connected_pins(physical_type,
                                                                  logical_block,
                                                                  cluster_pins,
                                                                  pin_physical_num,
                                                                  sink_pin_num);
            RRNodeId rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(),
                                                     physical_type,
                                                     root_loc,
                                                     pin_physical_num);
            VTR_ASSERT(rr_node_id != RRNodeId::INVALID());

            src_node_edge_pair.insert(std::make_pair(rr_node_id, chain_delay));

        } else {
            num_collapsed_pins++;
            std::vector<int> src_pins = get_src_pins_in_cluster(cluster_pins, physical_type, logical_block, pin_physical_num);
            for (int src_pin : src_pins) {
                // If the source pin is located on the current chain no edge should be added since the nodes should be collapsed.
                if (pin_chain_idx[src_pin] != UNDEFINED) {
                    if ((pin_chain_idx[src_pin] == chain_idx)) {
                        continue;
                    } else {
                        // If it is located on other chain, src_pin should be the sink of that chain, otherwise the chain is not formed correctly.
                        VTR_ASSERT(src_pin == nodes_to_collapse.chain_sink[pin_chain_idx[src_pin]]);
                    }
                }
                float delay = get_min_delay_to_chain(physical_type,
                                                     logical_block,
                                                     cluster_pins,
                                                     chain_pins,
                                                     src_pin,
                                                     sink_pin_num);

                RRNodeId rr_node_id = get_pin_rr_node_id(rr_graph_builder.node_lookup(), physical_type, root_loc, src_pin);
                VTR_ASSERT(rr_node_id != RRNodeId::INVALID());

                src_node_edge_pair.insert(std::make_pair(rr_node_id, delay));
            }
        }

        for (auto src_pair : src_node_edge_pair) {
            float delay = src_pair.second;
            bool is_rr_sw_id = load_rr_graph;
            bool is_new_sw;
            int sw_id;
            std::tie(is_new_sw, sw_id) = find_create_intra_cluster_sw(rr_graph_builder,
                                                                      all_sw_inf,
                                                                      R_minW_nmos,
                                                                      R_minW_pmos,
                                                                      is_rr_sw_id,
                                                                      delay);

            rr_edges_to_create.emplace_back(src_pair.first, sink_rr_node_id, sw_id, is_rr_sw_id);
        }
    }
}

static t_cluster_pin_chain get_cluster_directly_connected_nodes(const std::vector<int>& cluster_pins,
                                                                t_physical_tile_type_ptr physical_type,
                                                                t_logical_block_type_ptr logical_block,
                                                                bool is_flat) {
    std::unordered_set<int> cluster_pins_set(cluster_pins.begin(), cluster_pins.end()); // Faster for search

    VTR_ASSERT(is_flat);
    std::vector<int> pin_index_vec(get_tile_total_num_pin(physical_type), UNDEFINED);

    std::vector<std::vector<t_pin_chain_node>> chains;
    for (int pin_physical_num : cluster_pins) {
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);
        std::vector<int> conn_sink_pins = get_sink_pins_in_cluster(cluster_pins_set, physical_type, logical_block, pin_physical_num);
        VTR_ASSERT(pin_type != e_pin_type::OPEN);
        // Continue if the fan-out or fan-in of pin_physical_num is not equal to one or pin is already assigned to a chain.
        if (pin_index_vec[pin_physical_num] >= 0 || conn_sink_pins.size() != 1) {
            continue;
        } else {
            std::vector<int> node_chain = get_directly_connected_nodes(physical_type, logical_block, cluster_pins_set, pin_physical_num, is_flat);

            // node_chain contains the pin_physical_num itself to. Thus, we store the chain if its size is greater than 1.
            if (node_chain.size() > 1) {
                int num_chain = (int)chains.size();
                int chain_idx = get_chain_idx(pin_index_vec, node_chain, num_chain);
                add_pin_chain(node_chain,
                              chain_idx,
                              pin_index_vec,
                              chains,
                              (chain_idx == num_chain));
            }
        }
    }

    std::vector<int> chain_sinks = get_node_chain_sinks(chains);

    VTR_ASSERT(is_node_chain_sorted(physical_type,
                                    logical_block,
                                    cluster_pins_set,
                                    pin_index_vec,
                                    chains));

    return {pin_index_vec, chains, chain_sinks};
}

static float get_min_delay_to_chain(t_physical_tile_type_ptr physical_type,
                                    t_logical_block_type_ptr logical_block,
                                    const std::unordered_set<int>& cluster_pins,
                                    const std::unordered_set<int>& chain_pins,
                                    int pin_physical_num,
                                    int chain_sink_pin) {
    VTR_ASSERT(std::find(chain_pins.begin(), chain_pins.end(), pin_physical_num) == chain_pins.end());
    float min_delay = std::numeric_limits<float>::max();
    std::vector<int> sink_pins = get_sink_pins_in_cluster(cluster_pins, physical_type, logical_block, pin_physical_num);
    bool sink_pin_found = false;
    for (int sink_pin : sink_pins) {
        // If the sink is not on the chain, then we do not need to consider it.
        if (!chain_pins.contains(sink_pin)) {
            continue;
        }
        sink_pin_found = true;
        // Delay to the sink is equal to the delay to chain + chain's delay
        float delay = get_delay_directly_connected_pins(physical_type, logical_block, cluster_pins, sink_pin, chain_sink_pin) + get_edge_delay(physical_type, logical_block, pin_physical_num, sink_pin);
        if (delay < min_delay) {
            min_delay = delay;
        }
    }

    // We assume that the given pin has a sink in the chain.
    VTR_ASSERT(sink_pin_found);
    return min_delay;
}

static float get_delay_directly_connected_pins(t_physical_tile_type_ptr physical_type,
                                               t_logical_block_type_ptr logical_block,
                                               const std::unordered_set<int>& cluster_pins,
                                               int first_pin_num,
                                               int second_pin_num) {
    float delay = 0.;

    int curr_pin = first_pin_num;
    while (true) {
        std::vector<int> sink_pins = get_sink_pins_in_cluster(cluster_pins, physical_type, logical_block, curr_pin);
        // We expect the pins to be located on a chain
        VTR_ASSERT(sink_pins.size() == 1);
        delay += get_edge_delay(physical_type,
                                logical_block,
                                curr_pin,
                                sink_pins[0]);
        if (sink_pins[0] == second_pin_num) {
            break;
        }
        curr_pin = sink_pins[0];
    }

    return delay;
}

static std::vector<int> get_src_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                t_physical_tile_type_ptr physical_type,
                                                t_logical_block_type_ptr logical_block,
                                                const int pin_physical_num) {
    std::vector<int> src_pins_in_cluster;
    std::vector<int> all_conn_pins = get_physical_pin_src_pins(physical_type, logical_block, pin_physical_num);
    for (int conn_pin : all_conn_pins) {
        if (pins_in_cluster.find(conn_pin) != pins_in_cluster.end()) {
            src_pins_in_cluster.push_back(conn_pin);
        }
    }

    return src_pins_in_cluster;
}

static std::vector<int> get_directly_connected_nodes(t_physical_tile_type_ptr physical_type,
                                                     t_logical_block_type_ptr logical_block,
                                                     const std::unordered_set<int>& pins_in_cluster,
                                                     int pin_physical_num,
                                                     bool is_flat) {
    VTR_ASSERT(is_flat);
    if (is_primitive_pin(physical_type, pin_physical_num)) {
        return {pin_physical_num};
    }
    std::vector<int> conn_node_chain;
    e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type, pin_physical_num);

    // Forward
    {
        conn_node_chain.push_back(pin_physical_num);
        int last_pin_num = pin_physical_num;
        std::vector<int> sink_pins = get_sink_pins_in_cluster(pins_in_cluster, physical_type, logical_block, pin_physical_num);
        while (sink_pins.size() == 1) {
            last_pin_num = sink_pins[0];

            if (is_primitive_pin(physical_type, last_pin_num) || pin_type != get_pin_type_from_pin_physical_num(physical_type, last_pin_num)) {
                break;
            }

            conn_node_chain.push_back(sink_pins[0]);

            sink_pins = get_sink_pins_in_cluster(pins_in_cluster,
                                                 physical_type,
                                                 logical_block,
                                                 last_pin_num);
        }
    }
    return conn_node_chain;
}

static bool is_node_chain_sorted(t_physical_tile_type_ptr physical_type,
                                 t_logical_block_type_ptr logical_block,
                                 const std::unordered_set<int>& pins_in_cluster,
                                 const std::vector<int>& pin_index_vec,
                                 const std::vector<std::vector<t_pin_chain_node>>& all_node_chain) {
    int num_chain = (int)all_node_chain.size();
    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const std::vector<t_pin_chain_node>& curr_chain = all_node_chain[chain_idx];
        int num_nodes = (int)curr_chain.size();
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            const t_pin_chain_node& curr_node = curr_chain[node_idx];
            int curr_pin_num = curr_node.pin_physical_num;
            int nxt_idx = curr_node.nxt_node_idx;
            if (pin_index_vec[curr_pin_num] != chain_idx) {
                return false;
            }
            if (nxt_idx == UNDEFINED) {
                continue;
            }
            std::vector<int> conn_pin_vec = get_sink_pins_in_cluster(pins_in_cluster, physical_type, logical_block, curr_node.pin_physical_num);
            if (conn_pin_vec.size() != 1 || conn_pin_vec[0] != curr_chain[nxt_idx].pin_physical_num) {
                return false;
            }
        }
    }

    return true;
}

static std::vector<int> get_node_chain_sinks(const std::vector<std::vector<t_pin_chain_node>>& cluster_chains) {
    int num_chain = (int)cluster_chains.size();
    std::vector<int> chain_sink_pin(num_chain);

    for (int chain_idx = 0; chain_idx < num_chain; chain_idx++) {
        const std::vector<t_pin_chain_node>& chain = cluster_chains[chain_idx];
        int num_nodes = (int)chain.size();

        bool sink_node_found = false;
        for (int node_idx = 0; node_idx < num_nodes; node_idx++) {
            if (chain[node_idx].nxt_node_idx == UNDEFINED) {
                // Each chain should only contain ONE sink pin
                VTR_ASSERT(!sink_node_found);
                sink_node_found = true;
                chain_sink_pin[chain_idx] = chain[node_idx].pin_physical_num;
            }
        }
        // Make sure that the chain contains a sink pin
        VTR_ASSERT(sink_node_found);
    }

    return chain_sink_pin;
}

static std::vector<int> get_sink_pins_in_cluster(const std::unordered_set<int>& pins_in_cluster,
                                                 t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 const int pin_physical_num) {
    std::vector<int> sink_pins_in_cluster;
    std::vector<int> all_conn_pins = get_physical_pin_sink_pins(physical_type, logical_block, pin_physical_num);
    sink_pins_in_cluster.reserve(all_conn_pins.size());
    for (int conn_pin : all_conn_pins) {
        if (pins_in_cluster.find(conn_pin) != pins_in_cluster.end()) {
            sink_pins_in_cluster.push_back(conn_pin);
        }
    }

    return sink_pins_in_cluster;
}

static int get_chain_idx(const std::vector<int>& pin_idx_vec, const std::vector<int>& pin_chain, int new_idx) {
    int chain_idx = UNDEFINED;
    for (int pin_num : pin_chain) {
        int pin_chain_num = pin_idx_vec[pin_num];
        if (pin_chain_num != UNDEFINED) {
            chain_idx = pin_chain_num;
            break;
        }
    }
    if (chain_idx == UNDEFINED) {
        chain_idx = new_idx;
    }
    return chain_idx;
}

static void add_pin_chain(const std::vector<int>& pin_chain,
                          int chain_idx,
                          std::vector<int>& pin_index_vec,
                          std::vector<std::vector<t_pin_chain_node>>& all_chains,
                          bool is_new_chain) {
    std::list<int> new_chain;

    // None of the pins in the chain has been added to all_chains before
    if (is_new_chain) {
        std::vector<t_pin_chain_node> new_pin_chain;
        new_pin_chain.reserve(pin_chain.size());

        // Basically, the next pin connected to the current pin is located in the next element of the array
        int nxt_node_idx = 1;
        for (int pin_num : pin_chain) {
            // Since we assume that none of the pins in the pin_chain has seen before
            VTR_ASSERT(pin_index_vec[pin_num] == UNDEFINED);
            // Assign the pin its chain index
            pin_index_vec[pin_num] = chain_idx;
            new_pin_chain.emplace_back(pin_num, nxt_node_idx);
            nxt_node_idx++;
        }

        // For the last pin in the chain, there is no next pin.
        new_pin_chain[new_pin_chain.size() - 1].nxt_node_idx = UNDEFINED;

        all_chains.push_back(new_pin_chain);

    } else {
        // If the chain_idx is not equal to the all_chains.size(), it means that some of the pins in the chain has already been added
        // to the data structure which stores all the chains.

        std::vector<t_pin_chain_node>& node_chain = all_chains[chain_idx];
        int pin_chain_init_size = (int)node_chain.size();
        // For the pins in the chain which are not added, the pin connected to each of the is located in the vector element next to them.
        // Except for the last node in the chain which has not been added. We set the correct value for that particular pin later.
        int nxt_node_idx = pin_chain_init_size + 1;
        bool is_new_pin_added = false;
        bool found_pin_in_chain = false;
        for (int pin_num : pin_chain) {
            if (pin_index_vec[pin_num] == UNDEFINED) {
                pin_index_vec[pin_num] = chain_idx;
                node_chain.emplace_back(pin_num, nxt_node_idx);
                nxt_node_idx++;
                is_new_pin_added = true;
            } else {
                found_pin_in_chain = true;
                // pin_num is the pin_number of a pin which is the first pin in the chain that is added to all_chains
                int first_pin_in_chain_idx = UNDEFINED;
                for (int pin_idx = 0; pin_idx < pin_chain_init_size; pin_idx++) {
                    if (node_chain[pin_idx].pin_physical_num == pin_num) {
                        first_pin_in_chain_idx = pin_idx;
                        break;
                    }
                }
                // We assume that this pin is already added to all_chains
                VTR_ASSERT(first_pin_in_chain_idx != UNDEFINED);
                // We assume than if this block is being run, at least one new pin should be added to the chain
                VTR_ASSERT(is_new_pin_added);

                // The last added pin is connected to a pin which is already added to the chain
                node_chain[node_chain.size() - 1].nxt_node_idx = first_pin_in_chain_idx;

                // The rest of the chain should have already been added to the chain
                break;
            }
        }
        // There should be a pin which has been added to the chain before
        VTR_ASSERT(found_pin_in_chain);
    }
}

void build_intra_cluster_rr_graph(e_graph_type graph_type,
                                  const DeviceGrid& grid,
                                  const std::vector<t_physical_tile_type>& types,
                                  const RRGraphView& rr_graph,
                                  int delayless_switch,
                                  float R_minW_nmos,
                                  float R_minW_pmos,
                                  RRGraphBuilder& rr_graph_builder,
                                  bool is_flat,
                                  bool load_rr_graph) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    DeviceContext& device_ctx = g_vpr_ctx.mutable_device();

    vtr::ScopedStartFinishTimer timer("Build intra-cluster routing resource graph");

    rr_graph_builder.reset_rr_graph_flags();
    // When we are building intra-cluster resources, the edges already built are
    // already remapped.
    rr_graph_builder.init_edge_remap(true);

    vtr::vector<ClusterBlockId, t_cluster_pin_chain> pin_chains(clb_nlist.blocks().size());
    set_clusters_pin_chains(clb_nlist, pin_chains, is_flat);
    vtr::vector<ClusterBlockId, std::unordered_set<int>> cluster_flat_chain_pins = get_pin_chains_flat(pin_chains);

    int num_rr_nodes = rr_graph.num_nodes();
    alloc_and_load_intra_cluster_rr_node_indices(rr_graph_builder,
                                                 grid,
                                                 pin_chains,
                                                 cluster_flat_chain_pins,
                                                 &num_rr_nodes);
    size_t expected_node_count = num_rr_nodes;
    rr_graph_builder.resize_nodes(num_rr_nodes);

    alloc_and_load_intra_cluster_rr_graph(rr_graph_builder,
                                          grid,
                                          delayless_switch,
                                          pin_chains,
                                          cluster_flat_chain_pins,
                                          R_minW_nmos,
                                          R_minW_pmos,
                                          is_flat,
                                          load_rr_graph);

    if (rr_graph.num_nodes() > expected_node_count) {
        VTR_LOG_ERROR("Expected no more than %zu nodes, have %zu nodes\n",
                      expected_node_count, rr_graph.num_nodes());
    }

    if (!load_rr_graph) {
        rr_graph_builder.remap_rr_node_switch_indices(g_vpr_ctx.device().switch_fanin_remap);
    } else {
        rr_graph_builder.mark_edges_as_rr_switch_ids();
    }

    rr_graph_builder.partition_edges();

    rr_graph_builder.clear_temp_storage();

    const VibDeviceGrid vib_grid;
    check_rr_graph(device_ctx.rr_graph,
                   types,
                   device_ctx.rr_indexed_data,
                   grid,
                   vib_grid,
                   device_ctx.chan_width,
                   graph_type,
                   is_flat);
}
