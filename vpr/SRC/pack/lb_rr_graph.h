/* 
  Functions to creates, manipulate, and free the lb_rr_node graph that represents interconnect within a logic block.

  Author: Jason Luu
  Date: July 22, 2013
 */

#ifndef LB_RR_GRAPH_H
#define LB_RR_GRAPH_H

vector <t_lb_type_rr_node> *alloc_and_load_all_lb_type_rr_graph();

void free_all_lb_type_rr_graph(INOUTP vector<t_lb_type_rr_node> **lb_type_rr_graphs);

/* Accessor functions */
int get_lb_type_rr_graph_ext_source_index(t_type_ptr lb_type);
int get_lb_type_rr_graph_ext_sink_index(t_type_ptr lb_type);

#endif


