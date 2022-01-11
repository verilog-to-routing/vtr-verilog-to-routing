#include "noc_router.h"


NocRouter::NocRouter(int id, int grid_position_x, int grid_position_y):router_id(id), router_grid_position_x(grid_position_x), router_grid_position_y(grid_position_y)
{
}

// getters
int NocRouter::get_router_id(void)
{
    return router_id;
}

int NocRouter::get_router_grid_position_x(void)
{
    return router_grid_position_x;
}

int NocRouter::get_router_grid_position_y(void)
{
    return router_grid_position_y;
}

std::string NocRouter::get_router_design_module_name(void)
{
    return router_design_module_name;
}

// setters
void NocRouter::set_router_design_module_name(std::string design_module_name)
{
    router_design_module_name.assign(design_module_name);

    return;
}