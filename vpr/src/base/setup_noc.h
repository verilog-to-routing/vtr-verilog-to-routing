#ifndef SETUP_NOC
#define SETUP_NOC

#include <iostream>
#include <string>
#include <vector>


#include "physical_types.h"
#include "device_grid.h"
#include "globals.h"



// a data structure to store the position information of a noc router in the FPGA device
struct t_noc_router_tile_position {

    int grid_width_position;
    int grid_height_position;

};


void setup_noc(const t_arch& arch, std::string noc_router_tile_name);

void identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid, std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles, std::string noc_router_tile_name);

void generate_noc(const t_arch& arch, NocContext& noc_ctx);













#endif