#ifndef PATH_DELAY2_H
#define PATH_DELAY2_H
/***************** Subroutines exported by this module ***********************/

int alloc_and_load_timing_graph_levels();

void check_timing_graph();

float print_critical_path_node(FILE * fp, vtr::t_linked_int * critical_path_node, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping);

void detect_and_fix_timing_graph_combinational_loops();
#endif
