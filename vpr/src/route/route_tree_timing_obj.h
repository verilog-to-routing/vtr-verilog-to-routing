#pragma once
#include <vector>
#include "route_tree_type.h"
#include "clustered_netlist_fwd.h"
#include "spatial_route_tree_lookup.h"
#include "vtr_vector.h"
#include "vpr_types.h"
#include "route_common.h"
#include "connection_based_routing.h"

class RtNodeAllocator {
  public:
    RtNodeAllocator();
    ~RtNodeAllocator();
    t_rt_node* alloc();
    void free(t_rt_node* rt_node);
    void empty_free_list();

  private:
    t_rt_node* rt_node_free_list_;
};

class LinkedRtEdgeAllocator {
  public:
    LinkedRtEdgeAllocator();
    ~LinkedRtEdgeAllocator();
    t_linked_rt_edge* alloc();
    void free(t_linked_rt_edge* rt_edge);
    void empty_free_list();

  private:
    /* Frees lists for fast addition and deletion of nodes and edges. */
    t_linked_rt_edge* rt_edge_free_list_;
};

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
class RouteTreeTiming {
  public:
    RouteTreeTiming(
        const RrNodeInf& node_inf,
        const RrNodeSetInf& node_set_inf,
        const SwitchInf& switch_inf,
        const vtr::vector<ClusterNetId, std::vector<int>>& net_rr_terminals)
        : node_inf_(node_inf)
        , node_set_inf_(node_set_inf)
        , switch_inf_(switch_inf)
        , net_rr_terminals_(net_rr_terminals) {
        rr_node_to_rt_node_.resize(node_inf_.size(), nullptr);
    }

    t_rt_node* init_route_tree_to_source(ClusterNetId inet);

    void free_route_tree(t_rt_node* rt_node);

    template<typename RouteInf>
    void print_route_tree(const RouteInf& route_inf, const t_rt_node* rt_node) const;

    template<typename RouteInf>
    void print_route_tree(const RouteInf& route_inf, const t_rt_node* rt_node, int depth) const;

    template<typename RouteInf>
    t_rt_node* update_route_tree(const RouteInf& route_inf, t_heap* hptr, SpatialRouteTreeLookup* spatial_rt_lookup);

    void load_route_tree_Tdel(t_rt_node* rt_root, float Tarrival) const;

    template<typename RouteInf>
    void load_route_tree_rr_route_inf(RouteInf* route_inf_ptr, t_rt_node* root) const;

    t_rt_node* init_route_tree_to_source_no_net(int inode);

    void add_route_tree_to_rr_node_lookup(t_rt_node* node);

    t_rt_node* find_sink_rt_node(t_rt_node* rt_root, ClusterNetId net_id, ClusterPinId sink_pin);
    t_rt_node* find_sink_rt_node_recurr(t_rt_node* node, int sink_inode);

    /********** Incremental reroute ***********/
    // instead of ripping up a net that has some congestion, cut the branches
    // that don't legally lead to a sink and start routing with that partial route tree

    void print_edge(const t_linked_rt_edge* edge);
    void print_route_tree_node(const t_rt_node* rt_root);
    void print_route_tree_inf(const t_rt_node* rt_root);
    void print_route_tree_congestion(const t_rt_node* rt_root);

    t_rt_node* traceback_to_route_tree(ClusterNetId inet);
    t_rt_node* traceback_to_route_tree(ClusterNetId inet, std::vector<int>* non_config_node_set_usage);
    t_rt_node* traceback_to_route_tree(t_trace* head);
    t_rt_node* traceback_to_route_tree(t_trace* head, std::vector<int>* non_config_node_set_usage);
    t_trace* traceback_from_route_tree(ClusterNetId inet, const t_rt_node* root, int num_remaining_sinks);

    // Prune route tree
    //
    //  Note that non-configurable node will not be pruned unless the node is
    //  being totally ripped up, or the node is congested.
    template<typename RouteInf>
    t_rt_node* prune_route_tree(const RouteInf& route_inf, t_rt_node* rt_root, CBRR& connections_inf);

    // Prune route tree
    //
    //  Note that non-configurable nodes will be pruned if
    //  non_config_node_set_usage is provided.  prune_route_tree will update
    //  non_config_node_set_usage after pruning.
    template<typename RouteInf>
    t_rt_node* prune_route_tree(const RouteInf& route_inf, t_rt_node* rt_root, CBRR& connections_inf, std::vector<int>* non_config_node_set_usage);

    void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac);

    bool is_equivalent_route_tree(const t_rt_node* rt_root, const t_rt_node* cloned_rt_root);
    bool is_valid_skeleton_tree(const t_rt_node* rt_root);
    bool is_valid_route_tree(const t_rt_node* rt_root);
    bool is_uncongested_route_tree(const t_rt_node* root);
    float load_new_subtree_C_downstream(t_rt_node* rt_node) const;
    void load_new_subtree_R_upstream(t_rt_node* rt_node) const;

  private:
    template<typename RouteInf>
    t_rt_node* add_subtree_to_route_tree(const RouteInf& route_inf, t_heap* hptr, t_rt_node** sink_rt_node_ptr);

    t_rt_node*
    update_unbuffered_ancestors_C_downstream(t_rt_node* start_of_new_path_rt_node) const;

    bool verify_route_tree_recurr(t_rt_node* node, std::set<int>* seen_nodes) const;

    t_trace* traceback_to_route_tree_branch(t_trace* trace,
                                            std::map<int, t_rt_node*>* rr_node_to_rt,
                                            std::vector<int>* non_config_node_set_usage);

    template<typename RouteInf>
    t_rt_node* prune_route_tree_recurr(const RouteInf& route_inf, t_rt_node* node, CBRR& connections_inf, bool force_prune, std::vector<int>* non_config_node_set_usage);

    t_rt_node* add_non_configurable_to_route_tree(const int rr_node, const bool reached_by_non_configurable_edge, std::unordered_set<int>& visited);

    /* Array below allows mapping from any rr_node to any rt_node currently in
     * the rt_tree.                                                              */
    std::vector<t_rt_node*> rr_node_to_rt_node_; /* [0..device_ctx.rr_nodes.size()-1] */

    const RrNodeInf& node_inf_;
    const RrNodeSetInf& node_set_inf_;
    const SwitchInf& switch_inf_;
    RtNodeAllocator rt_nodes_;
    LinkedRtEdgeAllocator rt_edges_;
    const vtr::vector<ClusterNetId, std::vector<int>>& net_rr_terminals_;
};

#include "route_tree_timing_obj.hpp"
