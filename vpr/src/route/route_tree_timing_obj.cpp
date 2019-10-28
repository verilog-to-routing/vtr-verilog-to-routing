#include "route_tree_timing_obj.h"
#include "vtr_memory.h"

RtNodeAllocator::RtNodeAllocator()
    : rt_node_free_list_(nullptr) {}
RtNodeAllocator::~RtNodeAllocator() {
    empty_free_list();
}

void RtNodeAllocator::empty_free_list() {
    t_rt_node* rt_node = rt_node_free_list_;

    while (rt_node != nullptr) {
        t_rt_node* next_node = rt_node->u.next;
        free(rt_node);
        rt_node = next_node;
    }

    rt_node_free_list_ = nullptr;
}

t_rt_node* RtNodeAllocator::alloc() {
    /* Allocates a new rt_node, from the free list if possible, from the free
     * store otherwise.                                                         */

    t_rt_node* rt_node;

    rt_node = rt_node_free_list_;

    if (rt_node != nullptr) {
        rt_node_free_list_ = rt_node->u.next;
    } else {
        rt_node = (t_rt_node*)vtr::malloc(sizeof(t_rt_node));
    }

    return (rt_node);
}

void RtNodeAllocator::free(t_rt_node* rt_node) {
    /* Adds rt_node to the proper free list.          */
    rt_node->u.next = rt_node_free_list_;
    rt_node_free_list_ = rt_node;
}

LinkedRtEdgeAllocator::LinkedRtEdgeAllocator()
    : rt_edge_free_list_(nullptr) {}
LinkedRtEdgeAllocator::~LinkedRtEdgeAllocator() {
    empty_free_list();
}

t_linked_rt_edge* LinkedRtEdgeAllocator::alloc() {
    /* Allocates a new linked_rt_edge, from the free list if possible, from the
     * free store otherwise.                                                     */

    t_linked_rt_edge* linked_rt_edge;

    linked_rt_edge = rt_edge_free_list_;

    if (linked_rt_edge != nullptr) {
        rt_edge_free_list_ = linked_rt_edge->next;
    } else {
        linked_rt_edge = (t_linked_rt_edge*)vtr::malloc(sizeof(t_linked_rt_edge));
    }

    VTR_ASSERT(linked_rt_edge != nullptr);
    return (linked_rt_edge);
}

void LinkedRtEdgeAllocator::free(t_linked_rt_edge* rt_edge) {
    rt_edge->next = rt_edge_free_list_;
    rt_edge_free_list_ = rt_edge;
}

void LinkedRtEdgeAllocator::empty_free_list() {
    t_linked_rt_edge* rt_edge = rt_edge_free_list_;

    while (rt_edge != nullptr) {
        t_linked_rt_edge* next_edge = rt_edge->next;
        free(rt_edge);
        rt_edge = next_edge;
    }

    rt_edge_free_list_ = nullptr;
}
