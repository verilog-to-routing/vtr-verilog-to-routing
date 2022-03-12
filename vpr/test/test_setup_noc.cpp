#include "catch2/catch_test_macros.hpp"

#include "setup_noc.h"
#include "globals.h"
#include "physical_types.h"

// for comparing floats
#include "vtr_math.h"

namespace{
    
    TEST_CASE("test_identify_and_store_noc_router_tile_positions", "[vpr_setup_noc]"){

        // test device grid name
        std::string device_grid_name = "test";

        // creating a reference for the empty tile name and router name
        char empty_tile_name[6] = "empty";
        char router_tile_name[7] = "router"; 

        // device grid parameters
        int test_grid_width = 10;
        int test_grid_height = 10;
        
        // create the test device grid (10x10)
        auto test_grid = vtr::Matrix<t_grid_tile>({10, 10});

        // create an empty physical tile and assign its parameters
        t_physical_tile_type empty_tile;
        empty_tile.name = empty_tile_name;
        empty_tile.height = 1;
        empty_tile.width = 1;

        // create a router tile and assign its parameters
        // the router will take up 4 grid spaces and be named "router"
        t_physical_tile_type router_tile;
        router_tile.name = router_tile_name;
        router_tile.height = 2;
        router_tile.width = 2;

        // name of the router physical tile
        //std::string router_tile_name_string("router");

        // results from the test function
        std::vector<t_noc_router_tile_position> list_of_routers;

        // make sure the test result is not corrupted
        REQUIRE(list_of_routers.size() == 0);


        SECTION("All routers are seperated by one or more grid spaces"){

            // in this test, the routers will be on the 4 corners of the FPGA
            
            // bottom left corner
            test_grid[0][0].type = &router_tile;
            test_grid[0][0].height_offset = 0;
            test_grid[0][0].width_offset = 0;
            
            test_grid[1][0].type = &router_tile;
            test_grid[1][0].height_offset = 0;
            test_grid[1][0].width_offset = 1;

            test_grid[0][1].type = &router_tile;
            test_grid[0][1].height_offset = 1;
            test_grid[0][1].width_offset = 0;

            test_grid[1][1].type = &router_tile;
            test_grid[1][1].height_offset = 1;
            test_grid[1][1].width_offset = 1;

            // bottom right corner
            test_grid[8][0].type = &router_tile;
            test_grid[8][0].height_offset = 0;
            test_grid[8][0].width_offset = 0;
            
            test_grid[9][0].type = &router_tile;
            test_grid[9][0].height_offset = 0;
            test_grid[9][0].width_offset = 1;

            test_grid[8][1].type = &router_tile;
            test_grid[8][1].height_offset = 1;
            test_grid[8][1].width_offset = 0;

            test_grid[9][1].type = &router_tile;
            test_grid[9][1].height_offset = 1;
            test_grid[9][1].width_offset = 1;

            // top left corner
            test_grid[0][8].type = &router_tile;
            test_grid[0][8].height_offset = 0;
            test_grid[0][8].width_offset = 0;
            
            test_grid[1][8].type = &router_tile;
            test_grid[1][8].height_offset = 0;
            test_grid[1][8].width_offset = 1;

            test_grid[0][9].type = &router_tile;
            test_grid[0][9].height_offset = 1;
            test_grid[0][9].width_offset = 0;

            test_grid[1][9].type = &router_tile;
            test_grid[1][9].height_offset = 1;
            test_grid[1][9].width_offset = 1;

            // top right corner
            test_grid[8][8].type = &router_tile;
            test_grid[8][8].height_offset = 0;
            test_grid[8][8].width_offset = 0;
            
            test_grid[9][8].type = &router_tile;
            test_grid[9][8].height_offset = 0;
            test_grid[9][8].width_offset = 1;

            test_grid[8][9].type = &router_tile;
            test_grid[8][9].height_offset = 1;
            test_grid[8][9].width_offset = 0;

            test_grid[9][9].type = &router_tile;
            test_grid[9][9].height_offset = 1;
            test_grid[9][9].width_offset = 1;

            for(int i = 0; i < test_grid_width; i++)
            {
                for(int j = 0; j < test_grid_height; j++)
                {
                    // make sure the current tyle is not a router
                    if (test_grid[i][j].type == nullptr)
                    {
                        // assign the non-router tile as empty
                        test_grid[i][j].type = &empty_tile;
                        test_grid[i][j].width_offset = 0;
                        test_grid[i][j].height_offset = 0;
                    }
                }
            }

            // create a new device grid
            DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid);

            // call the test function
            identify_and_store_noc_router_tile_positions(test_device, list_of_routers, std::string(router_tile_name));

            // check that the physocal router tile positions were correctly determined
            // make sure that only 4 router tiles were found
            REQUIRE(list_of_routers.size() == 4);

            // check the bottom left router
            REQUIRE(list_of_routers[0].grid_width_position == 0);
            REQUIRE(list_of_routers[0].grid_height_position == 0);
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5));
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 0.5));

            // check the bottom right router
            REQUIRE(list_of_routers[1].grid_width_position == 0);
            REQUIRE(list_of_routers[1].grid_height_position == 8);
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 0.5));
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 8.5));

            // check the top left router
            REQUIRE(list_of_routers[2].grid_width_position == 8);
            REQUIRE(list_of_routers[2].grid_height_position == 0);
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 8.5));
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 0.5));

            // check the top right router
            REQUIRE(list_of_routers[3].grid_width_position == 8);
            REQUIRE(list_of_routers[3].grid_height_position == 8);
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 8.5));
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 8.5));
        }
        SECTION("All routers are horizontally connected to another router"){

            // in this test, the routers will be on the 4 corners of the FPGA
            
            // bottom left corner
            test_grid[3][0].type = &router_tile;
            test_grid[3][0].height_offset = 0;
            test_grid[3][0].width_offset = 0;
            
            test_grid[4][0].type = &router_tile;
            test_grid[4][0].height_offset = 0;
            test_grid[4][0].width_offset = 1;

            test_grid[3][1].type = &router_tile;
            test_grid[3][1].height_offset = 1;
            test_grid[3][1].width_offset = 0;

            test_grid[4][1].type = &router_tile;
            test_grid[4][1].height_offset = 1;
            test_grid[4][1].width_offset = 1;

            // bottom right corner
            test_grid[5][0].type = &router_tile;
            test_grid[5][0].height_offset = 0;
            test_grid[5][0].width_offset = 0;
            
            test_grid[6][0].type = &router_tile;
            test_grid[6][0].height_offset = 0;
            test_grid[6][0].width_offset = 1;

            test_grid[5][1].type = &router_tile;
            test_grid[5][1].height_offset = 1;
            test_grid[5][1].width_offset = 0;

            test_grid[6][1].type = &router_tile;
            test_grid[6][1].height_offset = 1;
            test_grid[6][1].width_offset = 1;

            // top left corner
            test_grid[0][5].type = &router_tile;
            test_grid[0][5].height_offset = 0;
            test_grid[0][5].width_offset = 0;
            
            test_grid[1][5].type = &router_tile;
            test_grid[1][5].height_offset = 0;
            test_grid[1][5].width_offset = 1;

            test_grid[0][6].type = &router_tile;
            test_grid[0][6].height_offset = 1;
            test_grid[0][6].width_offset = 0;

            test_grid[1][6].type = &router_tile;
            test_grid[1][6].height_offset = 1;
            test_grid[1][6].width_offset = 1;

            // top right corner
            test_grid[2][5].type = &router_tile;
            test_grid[2][5].height_offset = 0;
            test_grid[2][5].width_offset = 0;
            
            test_grid[3][5].type = &router_tile;
            test_grid[3][5].height_offset = 0;
            test_grid[3][5].width_offset = 1;

            test_grid[2][6].type = &router_tile;
            test_grid[2][6].height_offset = 1;
            test_grid[2][6].width_offset = 0;

            test_grid[3][6].type = &router_tile;
            test_grid[3][6].height_offset = 1;
            test_grid[3][6].width_offset = 1;

            for(int i = 0; i < test_grid_width; i++)
            {
                for(int j = 0; j < test_grid_height; j++)
                {
                    // make sure the current tyle is not a router
                    if (test_grid[i][j].type == nullptr)
                    {
                        // assign the non-router tile as empty
                        test_grid[i][j].type = &empty_tile;
                        test_grid[i][j].width_offset = 0;
                        test_grid[i][j].height_offset = 0;
                    }
                }
            }

            // create a new device grid
            DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid);

            // call the test function
            identify_and_store_noc_router_tile_positions(test_device, list_of_routers, std::string(router_tile_name));

            // check that the physocal router tile positions were correctly determined
            // make sure that only 4 router tiles were found
            REQUIRE(list_of_routers.size() == 4);

            // check the bottom left router
            REQUIRE(list_of_routers[0].grid_width_position == 0);
            REQUIRE(list_of_routers[0].grid_height_position == 5);
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5));
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 5.5));

            // check the bottom right router
            REQUIRE(list_of_routers[1].grid_width_position == 2);
            REQUIRE(list_of_routers[1].grid_height_position == 5);
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 2.5));
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 5.5));

            // check the top left router
            REQUIRE(list_of_routers[2].grid_width_position == 3);
            REQUIRE(list_of_routers[2].grid_height_position == 0);
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 3.5));
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 0.5));

            // check the top right router
            REQUIRE(list_of_routers[3].grid_width_position == 5);
            REQUIRE(list_of_routers[3].grid_height_position == 0);
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 5.5));
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 0.5));
        }
        SECTION("All routers are vertically connected to another router"){

            // in this test, the routers will be on the 4 corners of the FPGA
            
            // bottom left corner
            test_grid[0][2].type = &router_tile;
            test_grid[0][2].height_offset = 0;
            test_grid[0][2].width_offset = 0;
            
            test_grid[1][2].type = &router_tile;
            test_grid[1][2].height_offset = 0;
            test_grid[1][2].width_offset = 1;

            test_grid[0][3].type = &router_tile;
            test_grid[0][3].height_offset = 1;
            test_grid[0][3].width_offset = 0;

            test_grid[1][3].type = &router_tile;
            test_grid[1][3].height_offset = 1;
            test_grid[1][3].width_offset = 1;

            // bottom right corner
            test_grid[0][4].type = &router_tile;
            test_grid[0][4].height_offset = 0;
            test_grid[0][4].width_offset = 0;
            
            test_grid[1][4].type = &router_tile;
            test_grid[1][4].height_offset = 0;
            test_grid[1][4].width_offset = 1;

            test_grid[0][5].type = &router_tile;
            test_grid[0][5].height_offset = 1;
            test_grid[0][5].width_offset = 0;

            test_grid[1][5].type = &router_tile;
            test_grid[1][5].height_offset = 1;
            test_grid[1][5].width_offset = 1;

            // top left corner
            test_grid[7][6].type = &router_tile;
            test_grid[7][6].height_offset = 0;
            test_grid[7][6].width_offset = 0;
            
            test_grid[8][6].type = &router_tile;
            test_grid[8][6].height_offset = 0;
            test_grid[8][6].width_offset = 1;

            test_grid[7][7].type = &router_tile;
            test_grid[7][7].height_offset = 1;
            test_grid[7][7].width_offset = 0;

            test_grid[8][7].type = &router_tile;
            test_grid[8][7].height_offset = 1;
            test_grid[8][7].width_offset = 1;

            // top right corner
            test_grid[7][8].type = &router_tile;
            test_grid[7][8].height_offset = 0;
            test_grid[7][8].width_offset = 0;
            
            test_grid[8][8].type = &router_tile;
            test_grid[8][8].height_offset = 0;
            test_grid[8][8].width_offset = 1;

            test_grid[7][9].type = &router_tile;
            test_grid[7][9].height_offset = 1;
            test_grid[7][9].width_offset = 0;

            test_grid[8][9].type = &router_tile;
            test_grid[8][9].height_offset = 1;
            test_grid[8][9].width_offset = 1;

            for(int i = 0; i < test_grid_width; i++)
            {
                for(int j = 0; j < test_grid_height; j++)
                {
                    // make sure the current tyle is not a router
                    if (test_grid[i][j].type == nullptr)
                    {
                        // assign the non-router tile as empty
                        test_grid[i][j].type = &empty_tile;
                        test_grid[i][j].width_offset = 0;
                        test_grid[i][j].height_offset = 0;
                    }
                }
            }

            // create a new device grid
            DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid);

            // call the test function
            identify_and_store_noc_router_tile_positions(test_device, list_of_routers, std::string(router_tile_name));

            // check that the physocal router tile positions were correctly determined
            // make sure that only 4 router tiles were found
            REQUIRE(list_of_routers.size() == 4);

            // check the bottom left router
            REQUIRE(list_of_routers[0].grid_width_position == 0);
            REQUIRE(list_of_routers[0].grid_height_position == 2);
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5));
            REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 2.5));

            // check the bottom right router
            REQUIRE(list_of_routers[1].grid_width_position == 0);
            REQUIRE(list_of_routers[1].grid_height_position == 4);
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 0.5));
            REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 4.5));

            // check the top left router
            REQUIRE(list_of_routers[2].grid_width_position == 7);
            REQUIRE(list_of_routers[2].grid_height_position == 6);
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 7.5));
            REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 6.5));

            // check the top right router
            REQUIRE(list_of_routers[3].grid_width_position == 7);
            REQUIRE(list_of_routers[3].grid_height_position == 8);
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 7.5));
            REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 8.5));
        }

    }
    TEST_CASE("test_create_noc_routers", "[vpr_setup_noc]"){

        // datastructure to hold the list of physical tiles
        std::vector<t_noc_router_tile_position> list_of_routers;

        /*
            Setup:
                - The router will take over a 3x2 grid area
                - The NoC will be a 3x3 Mesh topology and located at 
                the following:
                    - router 1: (0,0)
                    - router 2: (4,0)
                    - router 3: (8,0)
                    - router 4: (0,4)
                    - router 5: (4,4)
                    - router 6: (8,4)
                    - router 7: (0,8)
                    - router 8: (4,8)
                    - router 9: (8,8)
        */
        list_of_routers.push_back({0,0,0.5,1});
        list_of_routers.push_back({4,0,4.5,1});
        list_of_routers.push_back({8,0,8.5,1});
        
        list_of_routers.push_back({0,4,0.5,5});
        list_of_routers.push_back({4,4,4.5,5});
        list_of_routers.push_back({8,4,8.5,5});

        list_of_routers.push_back({0,8,0.5,9});
        list_of_routers.push_back({4,8,4.5,9});
        list_of_routers.push_back({8,8,8.5,9});

    }

}