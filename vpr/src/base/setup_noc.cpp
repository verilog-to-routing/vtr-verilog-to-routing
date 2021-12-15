
#include "setup_noc.h"

#include "vtr_assert.h"
#include "vpr_error.h"


void setup_noc(const t_arch& arch, std::string noc_router_tile_name)
{

    // variable to store all the noc router tiles within the FPGA device
    std::vector<t_noc_router_tile_position> list_of_noc_router_tiles;

    // get references to global variables
    auto& device_ctx = g_vpr_ctx.device();
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    // go through the FPGA grid and find the noc router tiles
    // then store the position 
    identify_and_store_noc_router_tile_positions(device_ctx.grid,list_of_noc_router_tiles, noc_router_tile_name);

    // check whether the noc topology information provided uses more than the number of available routers in the FPGA
    if (list_of_noc_router_tiles.size() > arch.noc->router_list.size())
    {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Provided NoC topology information in the architecture file has more routers that what is available in the FPGA.");
    }

    // check whether the noc topology information provided is using all the routers in the FPGA
    if (list_of_noc_router_tiles.size() < arch.noc->router_list.size())
    {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Provided NoC topology information in the architecture file has more routers that what is available in the FPGA.");
    }

    // generate noc model
    generate_noc(arch, noc_ctx);






    return;

}

void identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid, std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles, std::string noc_router_tile_name)
{
    int grid_width = device_grid.width();
    int grid_height = device_grid.height();

    int curr_tile_width_offset;
    int curr_tile_height_offset;
    std::string curr_tile_name;

    // go through the device
    for(int i = 0; i < grid_width; i++)
    {
        
        for (int j = 0; j < grid_height; j++)
        {
            // get some information from the current tile
            curr_tile_name.assign(device_grid[i][j].type->name);
            curr_tile_width_offset = device_grid[i][j].width_offset;
            curr_tile_height_offset = device_grid[i][j].height_offset;

            /* 
                Only store the tile position if it is a noc router.
                Additionally, since a router tile can span multiple grid locations, we only add the tile if the height and width offset are zero (this prevents the router from being added multiple times for each grid location it spans).
            */
            if (!((noc_router_tile_name.compare(curr_tile_name)) && curr_tile_width_offset && curr_tile_width_offset))
            {
                list_of_noc_router_tiles.push_back({i,j});
            }

        }
    }

    return;

}

void generate_noc(const t_arch& arch, NocContext& noc_ctx)
{

    return;

}


