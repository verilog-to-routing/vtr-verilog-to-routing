

#include "router.h"


Router::Router(int id, int grid_position_x, int grid_position_y):router_id(id), router_grid_position_x(grid_position_x), router_grid_position_y(grid_position_y)
{
}

// getters
int Router::get_router_id(void)
{
    return router_id;
}

int Router::get_router_grid_position_x(void)
{
    return router_grid_position_x;
}

int Router::get_router_grid_position_y(void)
{
    return router_grid_position_y;
}

//test

