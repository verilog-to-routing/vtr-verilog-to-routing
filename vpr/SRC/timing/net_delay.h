#ifndef NET_DELAY_H
#define NET_DELAY_H

#include "vtr_memory.h"

float **alloc_net_delay(vtr::t_chunk *chunk_list_ptr, unsigned int n_nets);

void free_net_delay(float **net_delay,
		vtr::t_chunk *chunk_list_ptr);

void load_net_delay_from_routing(float **net_delay, unsigned int n_nets);

#endif
