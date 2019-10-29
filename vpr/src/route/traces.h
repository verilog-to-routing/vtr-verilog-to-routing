#ifndef TRACES_H_
#define TRACES_H_

#include "vtr_vector.h"
#include "vtr_memory.h"
#include "route_common.h"
#include "route_tree_type.h"
#include "route_traceback.h"

class TraceAllocator {
  public:
    ~TraceAllocator();
    t_trace* alloc();
    void free(t_trace* tptr);

    size_t num_trace_allocated() const {
        return num_trace_allocated_;
    }

    void free_chunk_memory_trace();

  private:
    /* For managing my own list of currently free trace data structures.    */
    t_trace* trace_free_head_;

    /* For keeping track of the sudo malloc memory for the trace*/
    vtr::t_chunk trace_ch_;

    /* To watch for memory leaks. */
    size_t num_trace_allocated_ = 0;
};

class Traces {
  public:
    Traces() {}

    // No copy/no assign.
    Traces(Traces const&) = delete;
    void operator=(Traces const& x) = delete;

    void resize(size_t num_nets) {
        trace_.resize(num_nets);
        trace_nodes_.resize(num_nets);
    }
    void clear() {
        trace_.clear();
        trace_nodes_.clear();
    }

    // Copies routing out of object into best_routing/saved_clb_opins_used_locally.
    void save_routing(
        vtr::vector<ClusterNetId, t_trace*>& best_routing,
        const t_clb_opins_used& clb_opins_used_locally,
        t_clb_opins_used& saved_clb_opins_used_locally);

    // Copies routing from best_routing/saved_clb_opins_used_locally to this object.
    void restore_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing,
                         t_clb_opins_used& clb_opins_used_locally,
                         const t_clb_opins_used& saved_clb_opins_used_locally);

    const t_trace* get_trace_head(ClusterNetId net_id) const {
        return trace_[net_id].head;
    }

    const t_trace* update_traceback(
        const std::vector<t_rr_node>& rr_nodes,
        const std::vector<t_rr_node_route_inf>& rr_node_route_inf,
        t_heap* hptr,
        ClusterNetId net_id);
    size_t num_trace_allocated() const {
        return traces_.num_trace_allocated();
    }

    void assert_trace_empty(ClusterNetId net_id) const;
    void free_traceback(ClusterNetId net_id);
    void free_all();
    void free_chunk_memory_trace();
    bool empty() const {
        return trace_.empty();
    }

    struct traceback_element {
        int inode;
        int switch_id;
    };

    typedef vtr::vector<ClusterNetId, std::vector<traceback_element>> saved_traces_t;

    void save_traces(saved_traces_t* routing) const;
    void restore_traces(const saved_traces_t& routing, float pres_fac);

    void set_traceback(ClusterNetId, const std::vector<traceback_element>& traceback);

    const t_trace* traceback_from_route_tree(
        const std::vector<t_rr_node>& rr_nodes,
        ClusterNetId inet,
        const t_rt_node* root,
        int num_routed_sinks);

  private:
    struct t_trace_branch {
        t_trace* head;
        t_trace* tail;
    };

    void free_traceback(t_trace* tptr);
    t_trace_branch traceback_branch(
        const std::vector<t_rr_node>& rr_nodes,
        const std::vector<t_rr_node_route_inf>& rr_node_route_inf,
        int node,
        std::unordered_set<int>& trace_nodes);
    std::pair<t_trace*, t_trace*> add_trace_non_configurable(
        const std::vector<t_rr_node>& rr_nodes,
        t_trace* head,
        t_trace* tail,
        int node,
        std::unordered_set<int>& trace_nodes);
    std::pair<t_trace*, t_trace*> add_trace_non_configurable_recurr(
        const std::vector<t_rr_node>& rr_nodes,
        int node,
        std::unordered_set<int>& trace_nodes,
        int depth);
    std::pair<t_trace*, t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node);
    bool validate_trace_nodes(t_trace* head, const std::unordered_set<int>& trace_nodes);

    /* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
    TraceAllocator traces_;
    struct t_traceback {
        t_trace* head = nullptr;
        t_trace* tail = nullptr;
    };
    vtr::vector<ClusterNetId, t_traceback> trace_;
    vtr::vector<ClusterNetId, std::unordered_set<int>> trace_nodes_;
};

#endif /* TRACES_H_ */
