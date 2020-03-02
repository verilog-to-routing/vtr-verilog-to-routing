#ifndef _STATIC_HEAP_H
#define _STATIC_HEAP_H

#include "heap_type.h"
#include "device_grid.h"

t_heap* alloc_heap_data();
void free_heap_data(t_heap*);

void init_heap(const DeviceGrid& grid);
t_heap* get_heap_head();
void empty_heap();
void add_to_heap(t_heap* hptr);
bool is_empty_heap();

namespace heap_ {
bool is_valid();
void build_heap();
void push_back_node(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream);
void add_node_to_heap(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream);
} // namespace heap_

void node_to_heap(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream);

void invalidate_heap_entries(int sink_node, int ipin_node);
void heap_free_all_memory();

#endif /* _STATIC_HEAP_H */
