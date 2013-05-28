#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H

void check_rr_graph(INP t_graph_type graph_type,
		INP int L_nx,
		INP int L_ny,
		INP int num_switches,
		int ** Fc_in);

void check_node(int inode, enum e_route_type route_type);

#endif

