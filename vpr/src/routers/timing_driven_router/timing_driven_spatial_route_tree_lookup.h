/************ Defines and types shared by all route files ********************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef TIMING_DRIVEN_SPATIAL_ROUTE_TREE_LOOKUP_H
#define TIMING_DRIVEN_SPATIAL_ROUTE_TREE_LOOKUP_H
/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */

#include <vector>

/* EXTERNAL library header files go second*/
#include "vtr_ndmatrix.h"

/* VPR header files go second*/
#include "clustered_netlist_fwd.h"
#include "timing_driven_route_tree_type.h"

/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router { 

namespace timing_driven { 


typedef vtr::Matrix<std::vector<t_rt_node*>> SpatialRouteTreeLookup;

SpatialRouteTreeLookup build_route_tree_spatial_lookup(ClusterNetId net, t_rt_node* rt_root);

void update_route_tree_spatial_lookup_recur(t_rt_node* rt_node, SpatialRouteTreeLookup& spatial_lookup);

size_t grid_to_bin_x(size_t grid_x, const SpatialRouteTreeLookup& spatial_lookup);
size_t grid_to_bin_y(size_t grid_y, const SpatialRouteTreeLookup& spatial_lookup);

bool validate_route_tree_spatial_lookup(t_rt_node* rt_root, const SpatialRouteTreeLookup& spatial_lookup);

} /* end of namespace timing_driven */

} /* end of namespace router */


#endif
