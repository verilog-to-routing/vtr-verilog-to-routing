/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_UTILS_H
#define ROUTER_UTILS_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */

#include "vpr_types.h"

namespace router { /* We put utility functions in an independent namespace than routers*/

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type);
vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type);
float routing_util(float used, float avail);

} /* end of router namespace */

#endif
