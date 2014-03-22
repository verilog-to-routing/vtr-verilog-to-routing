#ifndef RR_GRAPH_MULTI_H
#define RR_GRAPH_MULTI_H


#ifdef INTERPOSER_BASED_ARCHITECTURE
/* 
 * Function that does num_cuts horizontal cuts to the chip,
 * percent_wires_cut% of the wires crossing these cuts are removed
 * (the edges which cross the cut are removed) and the remaining
 * wires on this section have their delay increased by delay_increase
 */
void modify_rr_graph_for_interposer_based_arch(int nodes_per_chan, enum e_directionality directionality);

typedef enum t_CutPatternType
{
	UNIFORM_CUT_WITH_ROTATION,
	UNIFORM_CUT_WITHOUT_ROTATION,
	CHUNK_CUT_WITH_ROTATION,
	CHUNK_CUT_WITHOUT_ROTATION
} CutPatternType;


void modify_rr_graph_using_switch_modification_methodology(int nodes_per_chan, enum e_directionality directionality);
void modify_rr_graph_using_interposer_node_addition_methodology(int nodes_per_chan, enum e_directionality directionality);
void find_all_CHANY_wires_that_cross_the_interposer(int nodes_per_chan, int** rr_nodes_that_cross, int* num_rr_nodes_that_cross);
void expand_rr_graph(int* rr_nodes_that_cross, int num_rr_nodes_that_cross, int nodes_per_chan);
void delete_rr_connection(int src_node, int dst_node);
bool create_rr_connection(int src_node, int dst_node, int connection_switch_index);

void cut_chanx_interposer_node_connections(int nodes_per_chan);
int get_connection_switch_index(int src, int dst);
int get_num_interposer_loads(int inode, int nodes_per_chan);
int get_num_interposer_drivers(int inode, int nodes_per_chan);

#endif //INTERPOSER_BASED_ARCHITECTURE



#endif
