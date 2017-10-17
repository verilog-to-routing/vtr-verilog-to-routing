/* 
 * Read a .route file and load the route tree and other associated data structure
 * with the correct values. This is used to perform --analysis only
 */

#ifndef READ_ROUTE_H
#define READ_ROUTE_H



void read_route(const char* placement_file, const char* route_file, const t_router_opts& RouterOpts, const t_segment_inf* segment_inf);


#endif /* READ_ROUTE_H */

