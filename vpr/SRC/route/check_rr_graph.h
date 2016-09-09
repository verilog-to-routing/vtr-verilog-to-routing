#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H

void check_rr_graph(const t_graph_type graph_type,
		const int L_nx,
		const int L_ny,
		const int num_rr_switches,
		int *** Fc_in);

void check_node(int inode, enum e_route_type route_type);

#endif

