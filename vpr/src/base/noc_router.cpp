/*
 * The NocRouter defines a router in the NoC.
 * The NocRouter contains the following information:
 * - The router id. This represents the unique ID given by the
 * user in the architecture description file when describing a
 * router. THe purpose of this is to help the user identify the
 * router when loggin information or displaying errors.
 * - The grid position of the physical router tile this object
 * represents. Each router in the NoC represents a physical router
 * tile in the FPGA device. By storing the grid positions, we can quickly
 * get the corresponding physical router tile information by seaching
 * the DeviceGrid in the device context.
 * - The design module currently occupying this tile. Within the user
 * design there will be a number of instations of NoC routers. The user
 * will also provide information about which routers modules will be
 * communication with each other. During placement, it is possible for
 * the router modules to move between the physical router tiles, so 
 * by storing the module reference, we can determine which physical router tiles are communicating between each other and find a router
 * between them.   
 * There are also a number of functions defined here that can be used
 * to access the above information or modify them.
 */

#include "noc_router.h"

// constructor
NocRouter::NocRouter(int id, int grid_position_x, int grid_position_y)
    : router_id(id)
    , router_grid_position_x(grid_position_x)
    , router_grid_position_y(grid_position_y) {
    // initialize variables
    router_design_module_ref = "";
}

// getters
int NocRouter::get_router_id(void) const {
    return router_id;
}

int NocRouter::get_router_grid_position_x(void) const {
    return router_grid_position_x;
}

int NocRouter::get_router_grid_position_y(void) const {
    return router_grid_position_y;
}

std::string NocRouter::get_router_design_module_ref(void) const {
    return router_design_module_ref;
}

// setters
void NocRouter::set_router_design_module_ref(std::string design_module_ref) {
    router_design_module_ref.assign(design_module_ref);

    return;
}