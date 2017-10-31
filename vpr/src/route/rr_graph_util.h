#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H
struct t_linked_edge {
	int edge;
	short iswitch;
	t_linked_edge *next;
};

t_linked_edge *insert_in_edge_list(t_linked_edge * head, int edge,
		short iswitch);

void free_linked_edge_soft(t_linked_edge * edge_ptr,
		t_linked_edge ** free_list_head_ptr);

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node);

int seg_index_of_sblock(int from_node, int to_node);
#endif
