#ifndef RR_GRAPH_AREA_H
#define RR_GRAPH_AREA_H

void count_routing_transistors(enum e_directionality directionality,
                               int num_switch,
                               int wire_to_ipin_switch,
                               std::vector<t_segment_inf>& segment_inf,
                               float R_minW_nmos,
                               float R_minW_pmos);

float trans_per_buf(float Rbuf, float R_minW_nmos, float R_minW_pmos);

#endif
