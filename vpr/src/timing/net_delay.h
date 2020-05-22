#ifndef NET_DELAY_H
#define NET_DELAY_H

#include "vtr_memory.h"
#include "vtr_vector.h"
#include "vpr_net_pins_matrix.h"

void load_net_delay_from_routing(ClbNetPinsMatrix<float>& net_delay);

#endif
