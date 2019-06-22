#ifndef NET_DELAY_H
#define NET_DELAY_H

#include "vtr_memory.h"
#include "vtr_vector.h"

vtr::vector<ClusterNetId, float*> alloc_net_delay(vtr::t_chunk* chunk_list_ptr);

void free_net_delay(vtr::vector<ClusterNetId, float*>& net_delay,
                    vtr::t_chunk* chunk_list_ptr);

void load_net_delay_from_routing(vtr::vector<ClusterNetId, float*>& net_delay);

#endif
