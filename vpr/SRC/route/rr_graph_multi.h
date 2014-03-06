#ifndef RR_GRAPH_MULTI_H
#define RR_GRAPH_MULTI_H

/* 
 * Function that does num_cuts horizontal cuts to the chip,
 * percent_wires_cut% of the wires crossing these cuts are removed
 * (the edges which cross the cut are removed) and the remaining
 * wires on this section have their delay increased by delay_increase
 */
void modify_rr_graph_for_interposer_based_arch(int nodes_per_chan, enum e_directionality directionality);


#endif
