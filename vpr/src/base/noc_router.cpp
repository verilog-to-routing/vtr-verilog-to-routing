#include "noc_router.h"


NocRouter::NocRouter(int id, int grid_position_x, int grid_position_y):router_id(id), router_grid_position_x(grid_position_x), router_grid_position_y(grid_position_y)
{
    // initialize variables
    router_design_module_ref = "";
}

// defualt destructor
NocRouter::~NocRouter(){
    
}

// getters
int NocRouter::get_router_id(void) const{
    return router_id;
}

int NocRouter::get_router_grid_position_x(void) const{
    return router_grid_position_x;
}

int NocRouter::get_router_grid_position_y(void) const{
    return router_grid_position_y;
}

std::string NocRouter::get_router_design_module_ref(void) const{
    return router_design_module_ref;
}

// setters
void NocRouter::set_router_design_module_ref(std::string design_module_ref)
{
    router_design_module_ref.assign(design_module_ref);

    return;
}