/************ Defines and types shared by all route files ********************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_CHECK_H
#define ROUTER_CHECK_H
/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */


/* EXTERNAL library header files go second*/
#include "physical_types.h"


/* VPR header files go second*/
#include "router_common.h"

/* use a namespace for the functions */
/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router { 


void check_route(enum e_route_type route_type);

void recompute_occupancy_from_scratch();

} /* end of namespace router */

#endif
