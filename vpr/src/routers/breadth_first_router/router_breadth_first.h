#ifndef ROUTER_BREADTH_FIRST_H
#define ROUTER_BREADTH_FIRST_H

#include "router_common.h"

/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router {

namespace breadth_first {

bool try_breadth_first_route(t_router_opts router_opts);
bool try_breadth_first_route_net(ClusterNetId net_id, float pres_fac,
                                 t_router_opts router_opts);


} /* end of namespace breadth_first */

} /* end of namespace router */

#endif
