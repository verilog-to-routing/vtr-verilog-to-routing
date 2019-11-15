#ifndef CREATE_RR_GRAPH_H
#define CREATE_RR_GRAPH_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
#include "rr_graph_obj.h"

/* IMPORTANT: to build clock tree,
 * vpr added segments to the original arch segments  
 * This is why to use vpr_segments as an inputs!!!
 */
void convert_rr_graph(std::vector<t_segment_inf>& vpr_segments);

#endif
