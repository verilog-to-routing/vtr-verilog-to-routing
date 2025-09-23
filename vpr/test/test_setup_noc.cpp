#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers.hpp"
#include <random>

#include "setup_noc.h"
#include "globals.h"
#include "physical_types.h"

// for comparing floats
#include "vtr_math.h"

namespace {

TEST_CASE("test_identify_and_store_noc_router_tile_positions", "[vpr_setup_noc]") {
    // test device grid name
    std::string device_grid_name = "test";

    // creating a reference for the empty tile name and router name
    char empty_tile_name[6] = "empty";
    char router_tile_name[7] = "router";

    // device grid parameters
    int test_grid_width = 10;
    int test_grid_height = 10;

    // create the test device grid (10x10)
    auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, 10, 10});

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

    // results from the test function
    vtr::vector<int, t_noc_router_tile_position> list_of_routers;

    // make sure the test result is not corrupted
    REQUIRE(list_of_routers.empty());

    SECTION("All routers are seperated by one or more grid spaces") {
        // in this test, the routers will be on the 4 corners of the FPGA

        // bottom left corner
        test_grid[0][0][0].type = &router_tile;
        test_grid[0][0][0].height_offset = 0;
        test_grid[0][0][0].width_offset = 0;

        test_grid[0][1][0].type = &router_tile;
        test_grid[0][1][0].height_offset = 0;
        test_grid[0][1][0].width_offset = 1;

        test_grid[0][0][1].type = &router_tile;
        test_grid[0][0][1].height_offset = 1;
        test_grid[0][0][1].width_offset = 0;

        test_grid[0][1][1].type = &router_tile;
        test_grid[0][1][1].height_offset = 1;
        test_grid[0][1][1].width_offset = 1;

        // bottom right corner
        test_grid[0][8][0].type = &router_tile;
        test_grid[0][8][0].height_offset = 0;
        test_grid[0][8][0].width_offset = 0;

        test_grid[0][9][0].type = &router_tile;
        test_grid[0][9][0].height_offset = 0;
        test_grid[0][9][0].width_offset = 1;

        test_grid[0][8][1].type = &router_tile;
        test_grid[0][8][1].height_offset = 1;
        test_grid[0][8][1].width_offset = 0;

        test_grid[0][9][1].type = &router_tile;
        test_grid[0][9][1].height_offset = 1;
        test_grid[0][9][1].width_offset = 1;

        // top left corner
        test_grid[0][0][8].type = &router_tile;
        test_grid[0][0][8].height_offset = 0;
        test_grid[0][0][8].width_offset = 0;

        test_grid[0][1][8].type = &router_tile;
        test_grid[0][1][8].height_offset = 0;
        test_grid[0][1][8].width_offset = 1;

        test_grid[0][0][9].type = &router_tile;
        test_grid[0][0][9].height_offset = 1;
        test_grid[0][0][9].width_offset = 0;

        test_grid[0][1][9].type = &router_tile;
        test_grid[0][1][9].height_offset = 1;
        test_grid[0][1][9].width_offset = 1;

        // top right corner
        test_grid[0][8][8].type = &router_tile;
        test_grid[0][8][8].height_offset = 0;
        test_grid[0][8][8].width_offset = 0;

        test_grid[0][9][8].type = &router_tile;
        test_grid[0][9][8].height_offset = 0;
        test_grid[0][9][8].width_offset = 1;

        test_grid[0][8][9].type = &router_tile;
        test_grid[0][8][9].height_offset = 1;
        test_grid[0][8][9].width_offset = 0;

        test_grid[0][9][9].type = &router_tile;
        test_grid[0][9][9].height_offset = 1;
        test_grid[0][9][9].width_offset = 1;

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tile is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        // create a new device grid
        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        // call the test function
        list_of_routers = identify_and_store_noc_router_tile_positions(test_device, router_tile_name);

        // check that the physical router tile positions were correctly determined
        // make sure that only 4 router tiles were found
        REQUIRE(list_of_routers.size() == 4);

        // check the bottom left router
        REQUIRE(list_of_routers[0].grid_width_position == 0);
        REQUIRE(list_of_routers[0].grid_height_position == 0);
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5f));
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 0.5f));

        // check the bottom right router
        REQUIRE(list_of_routers[1].grid_width_position == 0);
        REQUIRE(list_of_routers[1].grid_height_position == 8);
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 0.5f));
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 8.5f));

        // check the top left router
        REQUIRE(list_of_routers[2].grid_width_position == 8);
        REQUIRE(list_of_routers[2].grid_height_position == 0);
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 8.5f));
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 0.5f));

        // check the top right router
        REQUIRE(list_of_routers[3].grid_width_position == 8);
        REQUIRE(list_of_routers[3].grid_height_position == 8);
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 8.5f));
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 8.5f));
    }
    SECTION("All routers are horizontally connected to another router") {
        // in this test, the routers will be on the 4 corners of the FPGA

        // bottom left corner
        test_grid[0][3][0].type = &router_tile;
        test_grid[0][3][0].height_offset = 0;
        test_grid[0][3][0].width_offset = 0;

        test_grid[0][4][0].type = &router_tile;
        test_grid[0][4][0].height_offset = 0;
        test_grid[0][4][0].width_offset = 1;

        test_grid[0][3][1].type = &router_tile;
        test_grid[0][3][1].height_offset = 1;
        test_grid[0][3][1].width_offset = 0;

        test_grid[0][4][1].type = &router_tile;
        test_grid[0][4][1].height_offset = 1;
        test_grid[0][4][1].width_offset = 1;

        // bottom right corner
        test_grid[0][5][0].type = &router_tile;
        test_grid[0][5][0].height_offset = 0;
        test_grid[0][5][0].width_offset = 0;

        test_grid[0][6][0].type = &router_tile;
        test_grid[0][6][0].height_offset = 0;
        test_grid[0][6][0].width_offset = 1;

        test_grid[0][5][1].type = &router_tile;
        test_grid[0][5][1].height_offset = 1;
        test_grid[0][5][1].width_offset = 0;

        test_grid[0][6][1].type = &router_tile;
        test_grid[0][6][1].height_offset = 1;
        test_grid[0][6][1].width_offset = 1;

        // top left corner
        test_grid[0][0][5].type = &router_tile;
        test_grid[0][0][5].height_offset = 0;
        test_grid[0][0][5].width_offset = 0;

        test_grid[0][1][5].type = &router_tile;
        test_grid[0][1][5].height_offset = 0;
        test_grid[0][1][5].width_offset = 1;

        test_grid[0][0][6].type = &router_tile;
        test_grid[0][0][6].height_offset = 1;
        test_grid[0][0][6].width_offset = 0;

        test_grid[0][1][6].type = &router_tile;
        test_grid[0][1][6].height_offset = 1;
        test_grid[0][1][6].width_offset = 1;

        // top right corner
        test_grid[0][2][5].type = &router_tile;
        test_grid[0][2][5].height_offset = 0;
        test_grid[0][2][5].width_offset = 0;

        test_grid[0][3][5].type = &router_tile;
        test_grid[0][3][5].height_offset = 0;
        test_grid[0][3][5].width_offset = 1;

        test_grid[0][2][6].type = &router_tile;
        test_grid[0][2][6].height_offset = 1;
        test_grid[0][2][6].width_offset = 0;

        test_grid[0][3][6].type = &router_tile;
        test_grid[0][3][6].height_offset = 1;
        test_grid[0][3][6].width_offset = 1;

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        // create a new device grid
        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        // call the test function
        list_of_routers = identify_and_store_noc_router_tile_positions(test_device, router_tile_name);

        // check that the physical router tile positions were correctly determined
        // make sure that only 4 router tiles were found
        REQUIRE(list_of_routers.size() == 4);

        // check the bottom left router
        REQUIRE(list_of_routers[0].grid_width_position == 0);
        REQUIRE(list_of_routers[0].grid_height_position == 5);
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5f));
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 5.5f));

        // check the bottom right router
        REQUIRE(list_of_routers[1].grid_width_position == 2);
        REQUIRE(list_of_routers[1].grid_height_position == 5);
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 2.5f));
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 5.5f));

        // check the top left router
        REQUIRE(list_of_routers[2].grid_width_position == 3);
        REQUIRE(list_of_routers[2].grid_height_position == 0);
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 3.5f));
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 0.5f));

        // check the top right router
        REQUIRE(list_of_routers[3].grid_width_position == 5);
        REQUIRE(list_of_routers[3].grid_height_position == 0);
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 5.5f));
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 0.5f));
    }
    SECTION("All routers are vertically connected to another router") {
        // in this test, the routers will be on the 4 corners of the FPGA

        // bottom left corner
        test_grid[0][0][2].type = &router_tile;
        test_grid[0][0][2].height_offset = 0;
        test_grid[0][0][2].width_offset = 0;

        test_grid[0][1][2].type = &router_tile;
        test_grid[0][1][2].height_offset = 0;
        test_grid[0][1][2].width_offset = 1;

        test_grid[0][0][3].type = &router_tile;
        test_grid[0][0][3].height_offset = 1;
        test_grid[0][0][3].width_offset = 0;

        test_grid[0][1][3].type = &router_tile;
        test_grid[0][1][3].height_offset = 1;
        test_grid[0][1][3].width_offset = 1;

        // bottom right corner
        test_grid[0][0][4].type = &router_tile;
        test_grid[0][0][4].height_offset = 0;
        test_grid[0][0][4].width_offset = 0;

        test_grid[0][1][4].type = &router_tile;
        test_grid[0][1][4].height_offset = 0;
        test_grid[0][1][4].width_offset = 1;

        test_grid[0][0][5].type = &router_tile;
        test_grid[0][0][5].height_offset = 1;
        test_grid[0][0][5].width_offset = 0;

        test_grid[0][1][5].type = &router_tile;
        test_grid[0][1][5].height_offset = 1;
        test_grid[0][1][5].width_offset = 1;

        // top left corner
        test_grid[0][7][6].type = &router_tile;
        test_grid[0][7][6].height_offset = 0;
        test_grid[0][7][6].width_offset = 0;

        test_grid[0][8][6].type = &router_tile;
        test_grid[0][8][6].height_offset = 0;
        test_grid[0][8][6].width_offset = 1;

        test_grid[0][7][7].type = &router_tile;
        test_grid[0][7][7].height_offset = 1;
        test_grid[0][7][7].width_offset = 0;

        test_grid[0][8][7].type = &router_tile;
        test_grid[0][8][7].height_offset = 1;
        test_grid[0][8][7].width_offset = 1;

        // top right corner
        test_grid[0][7][8].type = &router_tile;
        test_grid[0][7][8].height_offset = 0;
        test_grid[0][7][8].width_offset = 0;

        test_grid[0][8][8].type = &router_tile;
        test_grid[0][8][8].height_offset = 0;
        test_grid[0][8][8].width_offset = 1;

        test_grid[0][7][9].type = &router_tile;
        test_grid[0][7][9].height_offset = 1;
        test_grid[0][7][9].width_offset = 0;

        test_grid[0][8][9].type = &router_tile;
        test_grid[0][8][9].height_offset = 1;
        test_grid[0][8][9].width_offset = 1;

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tile is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        // create a new device grid
        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        DeviceGrid test_device = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        // call the test function
        list_of_routers = identify_and_store_noc_router_tile_positions(test_device, router_tile_name);

        // check that the physical router tile positions were correctly determined
        // make sure that only 4 router tiles were found
        REQUIRE(list_of_routers.size() == 4);

        // check the bottom left router
        REQUIRE(list_of_routers[0].grid_width_position == 0);
        REQUIRE(list_of_routers[0].grid_height_position == 2);
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_x, 0.5f));
        REQUIRE(vtr::isclose(list_of_routers[0].tile_centroid_y, 2.5f));

        // check the bottom right router
        REQUIRE(list_of_routers[1].grid_width_position == 0);
        REQUIRE(list_of_routers[1].grid_height_position == 4);
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_x, 0.5f));
        REQUIRE(vtr::isclose(list_of_routers[1].tile_centroid_y, 4.5f));

        // check the top left router
        REQUIRE(list_of_routers[2].grid_width_position == 7);
        REQUIRE(list_of_routers[2].grid_height_position == 6);
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_x, 7.5f));
        REQUIRE(vtr::isclose(list_of_routers[2].tile_centroid_y, 6.5f));

        // check the top right router
        REQUIRE(list_of_routers[3].grid_width_position == 7);
        REQUIRE(list_of_routers[3].grid_height_position == 8);
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_x, 7.5f));
        REQUIRE(vtr::isclose(list_of_routers[3].tile_centroid_y, 8.5f));
    }
}
TEST_CASE("test_create_noc_routers", "[vpr_setup_noc]") {
    // data structure to hold the list of physical tiles
    vtr::vector<int, t_noc_router_tile_position> list_of_routers;

    /*
     * Setup:
     * - The router will take over a 2x3 grid area
     * - The NoC will be a 3x3 Mesh topology and located at 
     * the following positions:
     * - router 1: (0,0)
     * - router 2: (4,0)
     * - router 3: (8,0)
     * - router 4: (0,4)
     * - router 5: (4,4)
     * - router 6: (8,4)
     * - router 7: (0,8)
     * - router 8: (4,8)
     * - router 9: (8,8)
     */
    list_of_routers.emplace_back(0, 0, 0, 0.5, 1);
    list_of_routers.emplace_back(4, 0, 0, 4.5, 1);
    list_of_routers.emplace_back(8, 0, 0, 8.5, 1);

    list_of_routers.emplace_back(0, 4, 0, 0.5, 5);
    list_of_routers.emplace_back(4, 4, 0, 4.5, 5);
    list_of_routers.emplace_back(8, 4, 0, 8.5, 5);

    list_of_routers.emplace_back(0, 8, 0, 0.5, 9);
    list_of_routers.emplace_back(4, 8, 0, 4.5, 9);
    list_of_routers.emplace_back(8, 8, 0, 8.5, 9);

    // create the noc model (to store the routers)
    NocStorage noc_model;

    // create the logical router list
    t_noc_inf noc_info;

    // pointer to each logical router
    t_router* temp_router = nullptr;

    const vtr::vector<NocRouterId, NocRouter>* noc_routers = nullptr;

    SECTION("Test create routers when logical routers match to exactly one physical router. The number of routers is less than whats on the FPGA.") {
        // start by creating all the logical routers
        // this is similar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 7; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
            temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;
            temp_router->id = router_id;
            noc_info.router_list.push_back(*temp_router);
        }

        // delete the router struct
        delete temp_router;

        // call the router creation
        create_noc_routers(noc_info, &noc_model, list_of_routers);

        // get the routers from the noc
        noc_routers = &(noc_model.get_noc_routers());

        // first check that only 6 routers were created
        REQUIRE(noc_routers->size() == 6);

        // now we got through the noc model and confirm that the correct
        for (int router_id = 1; router_id < 7; router_id++) {
            // covert the router id
            noc_router_id = noc_model.convert_router_id(router_id);

            // get the router that we are testing from the NoC
            const NocRouter& test_router = noc_model.get_single_noc_router(noc_router_id);

            // now check that the proper physical router was assigned to
            REQUIRE(test_router.get_router_grid_position_x() == list_of_routers[router_id - 1].grid_width_position);
            REQUIRE(test_router.get_router_grid_position_y() == list_of_routers[router_id - 1].grid_height_position);
            REQUIRE(test_router.get_router_layer_position() == list_of_routers[router_id - 1].layer_position);
        }
    }
    SECTION("Test create routers when logical routers match to exactly one physical router. The number of routers is exactly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 10; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
            temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;

            temp_router->id = router_id;
            noc_info.router_list.push_back(*temp_router);
        }

        // delete the router struct
        delete temp_router;

        // call the router creation
        create_noc_routers(noc_info, &noc_model, list_of_routers);

        // get the routers from the noc
        noc_routers = &(noc_model.get_noc_routers());

        // first check that only 6 routers were created
        REQUIRE(noc_routers->size() == 9);

        // now we got through the noc model and confirm that the correct
        for (int router_id = 1; router_id < 10; router_id++) {
            // covert the router id
            noc_router_id = noc_model.convert_router_id(router_id);

            // get the router that we are testing now from the NoC
            const NocRouter& test_router = noc_model.get_single_noc_router(noc_router_id);

            // now check that the proper physical router was assigned to
            REQUIRE(test_router.get_router_grid_position_x() == list_of_routers[router_id - 1].grid_width_position);
            REQUIRE(test_router.get_router_grid_position_y() == list_of_routers[router_id - 1].grid_height_position);
            REQUIRE(test_router.get_router_layer_position() == list_of_routers[router_id - 1].layer_position);
        }
    }
    SECTION("Test create routers when a logical router can be matched to two physical routers. The number of routers is exactly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 9; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
            temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;
            temp_router->id = router_id;
            noc_info.router_list.push_back(*temp_router);
        }

        // the top right logical router will be between the top right and top center physical routers in the mesh
        temp_router->device_x_position = 6.5;
        temp_router->device_y_position = 9;
        temp_router->id = 9;
        noc_info.router_list.push_back(*temp_router);

        // delete the router struct
        delete temp_router;

        // call the router creation
        REQUIRE_THROWS_WITH(create_noc_routers(noc_info, &noc_model, list_of_routers),
                            "Router with ID:'9' has the same distance to physical router tiles located at position (4,8) and (8,8). Therefore, no router assignment could be made.");
    }
    SECTION("Test create routers when a physical router can be matched to two logical routers. The number of routers is exactly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 9; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
            temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;
            temp_router->id = router_id;
            noc_info.router_list.push_back(*temp_router);
        }

        // the top right logical router will be between the top right and top center physical routers in the mesh
        temp_router->device_x_position = 4.8;
        temp_router->device_y_position = 5.1;
        temp_router->id = 9;
        noc_info.router_list.push_back(*temp_router);

        // delete the router struct
        delete temp_router;

        // call the router creation
        REQUIRE_THROWS_WITH(create_noc_routers(noc_info, &noc_model, list_of_routers),
                            "Routers with IDs:'9' and '5' are both closest to physical router tile located at (4,4) and the physical router could not be assigned multiple times.");
    }
}
TEST_CASE("test_create_noc_links", "[vpr_setup_noc]") {
    // data structure to hold the list of physical tiles
    std::vector<t_noc_router_tile_position> list_of_routers;

    /*
     * Setup:
     * - The router will take over a 2x3 grid area
     * - The NoC will be a 3x3 Mesh topology and located at 
     * the following positions:
     * - router 1: (0,0)
     * - router 2: (4,0)
     * - router 3: (8,0)
     * - router 4: (0,4)
     * - router 5: (4,4)
     * - router 6: (8,4)
     * - router 7: (0,8)
     * - router 8: (4,8)
     * - router 9: (8,8)
     */
    list_of_routers.emplace_back(0, 0, 0, 0.5, 1);
    list_of_routers.emplace_back(4, 0, 0, 4.5, 1);
    list_of_routers.emplace_back(8, 0, 0, 8.5, 1);

    list_of_routers.emplace_back(0, 4, 0, 0.5, 5);
    list_of_routers.emplace_back(4, 4, 0, 4.5, 5);
    list_of_routers.emplace_back(8, 4, 0, 8.5, 5);

    list_of_routers.emplace_back(0, 8, 0, 0.5, 9);
    list_of_routers.emplace_back(4, 8, 0, 4.5, 9);
    list_of_routers.emplace_back(8, 8, 0, 8.5, 9);

    // create the noc model (to store the routers)
    NocStorage noc_model;

    // store the reference to device grid with
    // this will be set to the device grid width
    noc_model.set_device_grid_spec((int)3, 0);

    // create the logical router list
    t_noc_inf noc_info;

    // pointer to each logical router
    t_router* temp_router = nullptr;

    // start by creating all the logical routers
    // this is similar to the user provided a config file
    temp_router = new t_router;

    for (int router_id = 1; router_id < 10; router_id++) {
        // we will have 9 logical routers that will take up all physical routers
        temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
        temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
        temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;
        temp_router->id = router_id;
        noc_info.router_list.push_back(*temp_router);

        // add the router to the NoC
        noc_model.add_router(router_id,
                             list_of_routers[router_id - 1].grid_width_position,
                             list_of_routers[router_id - 1].grid_height_position,
                             list_of_routers[router_id - 1].layer_position,
                             1.0);
    }

    delete temp_router;

    // need to add the links to the noc_info
    // this will be a mesh structure
    noc_info.router_list[0].connection_list.push_back(2);
    noc_info.router_list[0].connection_list.push_back(4);

    noc_info.router_list[1].connection_list.push_back(1);
    noc_info.router_list[1].connection_list.push_back(3);
    noc_info.router_list[1].connection_list.push_back(5);

    noc_info.router_list[2].connection_list.push_back(2);
    noc_info.router_list[2].connection_list.push_back(6);

    noc_info.router_list[3].connection_list.push_back(1);
    noc_info.router_list[3].connection_list.push_back(5);
    noc_info.router_list[3].connection_list.push_back(7);

    noc_info.router_list[4].connection_list.push_back(2);
    noc_info.router_list[4].connection_list.push_back(4);
    noc_info.router_list[4].connection_list.push_back(6);
    noc_info.router_list[4].connection_list.push_back(8);

    noc_info.router_list[5].connection_list.push_back(3);
    noc_info.router_list[5].connection_list.push_back(5);
    noc_info.router_list[5].connection_list.push_back(9);

    noc_info.router_list[6].connection_list.push_back(4);
    noc_info.router_list[6].connection_list.push_back(8);

    noc_info.router_list[7].connection_list.push_back(5);
    noc_info.router_list[7].connection_list.push_back(7);
    noc_info.router_list[7].connection_list.push_back(9);

    noc_info.router_list[8].connection_list.push_back(6);
    noc_info.router_list[8].connection_list.push_back(8);

    // call the function to test
    create_noc_links(noc_info, &noc_model);

    NocRouterId current_source_router_id;
    NocRouterId current_destination_router_id;

    std::vector<int>::iterator router_connection;

    // now verify the created links
    for (int router_id = 1; router_id < 10; router_id++) {
        current_source_router_id = noc_model.convert_router_id(router_id);

        router_connection = noc_info.router_list[router_id - 1].connection_list.begin();

        for (auto noc_link = noc_model.get_noc_router_outgoing_links(current_source_router_id).begin(); noc_link != noc_model.get_noc_router_outgoing_links(current_source_router_id).end(); noc_link++) {
            // get the connecting link
            const NocLink& connecting_link = noc_model.get_single_noc_link(*noc_link);

            // get the destination router
            current_destination_router_id = connecting_link.get_sink_router();
            const NocRouter& current_destination_router = noc_model.get_single_noc_router(current_destination_router_id);

            REQUIRE((current_destination_router.get_router_user_id()) == (*router_connection));

            router_connection++;
        }
    }
}
TEST_CASE("test_setup_noc", "[vpr_setup_noc]") {
    // create the architecture object
    t_arch arch;

    // create the logical router list
    t_noc_inf noc_info;

    // pointer to each logical router
    t_router* temp_router = nullptr;

    // start by creating all the logical routers
    // this is similar to the user provided a config file
    temp_router = new t_router;

    // data structure to hold the list of physical tiles
    std::vector<t_noc_router_tile_position> list_of_routers;

    // get a mutable to the device context
    auto& device_ctx = g_vpr_ctx.mutable_device();

    // delete any previous device grid
    device_ctx.grid.clear();

    /*
     * Setup:
     * - The router will take over a 2x3 grid area
     * - The NoC will be a 3x3 Mesh topology and located at 
     * the following positions:
     * - router 1: (0,0)
     * - router 2: (4,0)
     * - router 3: (8,0)
     * - router 4: (0,4)
     * - router 5: (4,4)
     * - router 6: (8,4)
     * - router 7: (0,8)
     * - router 8: (4,8)
     * - router 9: (8,8)
     */
    list_of_routers.emplace_back(0, 0, 0, 0.5, 1);
    list_of_routers.emplace_back(4, 0, 0, 4.5, 1);
    list_of_routers.emplace_back(8, 0, 0, 8.5, 1);

    list_of_routers.emplace_back(0, 4, 0, 0.5, 5);
    list_of_routers.emplace_back(4, 4, 0, 4.5, 5);
    list_of_routers.emplace_back(8, 4, 0, 8.5, 5);

    list_of_routers.emplace_back(0, 8, 0, 0.5, 9);
    list_of_routers.emplace_back(4, 8, 0, 4.5, 9);
    list_of_routers.emplace_back(8, 8, 0, 8.5, 9);

    for (int router_id = 1; router_id < 10; router_id++) {
        // we will have 9 logical routers that will take up all physical routers
        temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
        temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
        temp_router->device_layer_position = list_of_routers[router_id - 1].layer_position;
        temp_router->id = router_id;
        noc_info.router_list.push_back(*temp_router);
    }

    delete temp_router;

    // need to add the links to the noc_info
    // this will be a mesh structure
    noc_info.router_list[0].connection_list.push_back(2);
    noc_info.router_list[0].connection_list.push_back(4);

    noc_info.router_list[1].connection_list.push_back(1);
    noc_info.router_list[1].connection_list.push_back(3);
    noc_info.router_list[1].connection_list.push_back(5);

    noc_info.router_list[2].connection_list.push_back(2);
    noc_info.router_list[2].connection_list.push_back(6);

    noc_info.router_list[3].connection_list.push_back(1);
    noc_info.router_list[3].connection_list.push_back(5);
    noc_info.router_list[3].connection_list.push_back(7);

    noc_info.router_list[4].connection_list.push_back(2);
    noc_info.router_list[4].connection_list.push_back(4);
    noc_info.router_list[4].connection_list.push_back(6);
    noc_info.router_list[4].connection_list.push_back(8);

    noc_info.router_list[5].connection_list.push_back(3);
    noc_info.router_list[5].connection_list.push_back(5);
    noc_info.router_list[5].connection_list.push_back(9);

    noc_info.router_list[6].connection_list.push_back(4);
    noc_info.router_list[6].connection_list.push_back(8);

    noc_info.router_list[7].connection_list.push_back(5);
    noc_info.router_list[7].connection_list.push_back(7);
    noc_info.router_list[7].connection_list.push_back(9);

    noc_info.router_list[8].connection_list.push_back(6);
    noc_info.router_list[8].connection_list.push_back(8);

    //assign the noc_info to the arch
    arch.noc = &noc_info;

    // the architecture file has been setup to include the noc topology, and we set the parameters below
    noc_info.link_bandwidth = 67.8;
    noc_info.link_latency = 56.7;
    noc_info.router_latency = 2.3;

    SECTION("Test setup_noc when the number of logical routers is more than the number of physical routers in the FPGA.") {
        // test device grid name
        std::string device_grid_name = "test";

        // creating a reference for the empty tile name and router name
        char empty_tile_name[6] = "empty";
        char router_tile_name[7] = "router";

        // assign the name used when describing a router tile in the FPGA architecture description file
        arch.noc->noc_router_tile_name.assign(router_tile_name);

        // device grid parameters
        int test_grid_width = 10;
        int test_grid_height = 10;

        // create the test device grid (10x10)
        auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, 10, 10});

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

        // in this test, the routers will be on the 4 corners of the FPGA

        // bottom left corner
        test_grid[0][0][0].type = &router_tile;
        test_grid[0][0][0].height_offset = 0;
        test_grid[0][0][0].width_offset = 0;

        test_grid[0][1][0].type = &router_tile;
        test_grid[0][1][0].height_offset = 0;
        test_grid[0][1][0].width_offset = 1;

        test_grid[0][0][1].type = &router_tile;
        test_grid[0][0][1].height_offset = 1;
        test_grid[0][0][1].width_offset = 0;

        test_grid[0][1][1].type = &router_tile;
        test_grid[0][1][1].height_offset = 1;
        test_grid[0][1][1].width_offset = 1;

        // bottom right corner
        test_grid[0][8][0].type = &router_tile;
        test_grid[0][8][0].height_offset = 0;
        test_grid[0][8][0].width_offset = 0;

        test_grid[0][9][0].type = &router_tile;
        test_grid[0][9][0].height_offset = 0;
        test_grid[0][9][0].width_offset = 1;

        test_grid[0][8][1].type = &router_tile;
        test_grid[0][8][1].height_offset = 1;
        test_grid[0][8][1].width_offset = 0;

        test_grid[0][9][1].type = &router_tile;
        test_grid[0][9][1].height_offset = 1;
        test_grid[0][9][1].width_offset = 1;

        // top left corner
        test_grid[0][0][8].type = &router_tile;
        test_grid[0][0][8].height_offset = 0;
        test_grid[0][0][8].width_offset = 0;

        test_grid[0][1][8].type = &router_tile;
        test_grid[0][1][8].height_offset = 0;
        test_grid[0][1][8].width_offset = 1;

        test_grid[0][0][9].type = &router_tile;
        test_grid[0][0][9].height_offset = 1;
        test_grid[0][0][9].width_offset = 0;

        test_grid[0][1][9].type = &router_tile;
        test_grid[0][1][9].height_offset = 1;
        test_grid[0][1][9].width_offset = 1;

        // top right corner
        test_grid[0][8][8].type = &router_tile;
        test_grid[0][8][8].height_offset = 0;
        test_grid[0][8][8].width_offset = 0;

        test_grid[0][9][8].type = &router_tile;
        test_grid[0][9][8].height_offset = 0;
        test_grid[0][9][8].width_offset = 1;

        test_grid[0][8][9].type = &router_tile;
        test_grid[0][8][9].height_offset = 1;
        test_grid[0][8][9].width_offset = 0;

        test_grid[0][9][9].type = &router_tile;
        test_grid[0][9][9].height_offset = 1;
        test_grid[0][9][9].width_offset = 1;

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tile is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        device_ctx.grid = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        REQUIRE_THROWS_WITH(setup_noc(arch), "The Provided NoC topology information in the architecture file has more number of routers than what is available in the FPGA device.");
    }
    SECTION("Test setup_noc when there are no physical NoC routers on the FPGA.") {
        // test device grid name
        std::string device_grid_name = "test";

        // creating a reference for the empty tile name and router name
        char empty_tile_name[6] = "empty";
        char router_tile_name[7] = "router";

        // assign the name used when describing a router tile in the FPGA architecture description file
        arch.noc->noc_router_tile_name.assign(router_tile_name);

        // device grid parameters
        int test_grid_width = 10;
        int test_grid_height = 10;

        // create the test device grid (10x10)
        auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, 10, 10});

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

        // in this test, there should be no physical router tiles ih the FPGA device and there should be no routers in the topology description

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tile is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        noc_info.router_list.clear();

        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        device_ctx.grid = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        REQUIRE_THROWS_WITH(setup_noc(arch), "No physical NoC routers were found on the FPGA device. Either the provided name for the physical router tile was incorrect or the FPGA device has no routers.");
    }
    SECTION("Test setup_noc when there are overrides for NoC-wide latency and bandwidth values.") {
        // test device grid name
        std::string device_grid_name = "test";

        // creating a reference for the empty tile name and router name
        char empty_tile_name[6] = "empty";
        char router_tile_name[7] = "router";

        // assign the name used when describing a router tile in the FPGA architecture description file
        arch.noc->noc_router_tile_name.assign(router_tile_name);

        // device grid parameters
        int test_grid_width = 10;
        int test_grid_height = 10;

        // create the test device grid (10x10)
        auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, 10, 10});

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

        // (0, 0)
        test_grid[0][0][0].type = &router_tile;
        test_grid[0][0][0].height_offset = 0;
        test_grid[0][0][0].width_offset = 0;

        test_grid[0][1][0].type = &router_tile;
        test_grid[0][1][0].height_offset = 0;
        test_grid[0][1][0].width_offset = 1;

        test_grid[0][0][1].type = &router_tile;
        test_grid[0][0][1].height_offset = 1;
        test_grid[0][0][1].width_offset = 0;

        test_grid[0][1][1].type = &router_tile;
        test_grid[0][1][1].height_offset = 1;
        test_grid[0][1][1].width_offset = 1;

        // (0, 4)
        test_grid[0][0][4].type = &router_tile;
        test_grid[0][0][4].height_offset = 0;
        test_grid[0][0][4].width_offset = 0;

        test_grid[0][1][4].type = &router_tile;
        test_grid[0][1][4].height_offset = 0;
        test_grid[0][1][4].width_offset = 1;

        test_grid[0][0][5].type = &router_tile;
        test_grid[0][0][5].height_offset = 1;
        test_grid[0][0][5].width_offset = 0;

        test_grid[0][1][5].type = &router_tile;
        test_grid[0][1][5].height_offset = 1;
        test_grid[0][1][5].width_offset = 1;

        // (0, 8)
        test_grid[0][0][8].type = &router_tile;
        test_grid[0][0][8].height_offset = 0;
        test_grid[0][0][8].width_offset = 0;

        test_grid[0][1][8].type = &router_tile;
        test_grid[0][1][8].height_offset = 0;
        test_grid[0][1][8].width_offset = 1;

        test_grid[0][0][9].type = &router_tile;
        test_grid[0][0][9].height_offset = 1;
        test_grid[0][0][9].width_offset = 0;

        test_grid[0][1][9].type = &router_tile;
        test_grid[0][1][9].height_offset = 1;
        test_grid[0][1][9].width_offset = 1;

        // (4, 0)
        test_grid[0][4][0].type = &router_tile;
        test_grid[0][4][0].height_offset = 0;
        test_grid[0][4][0].width_offset = 0;

        test_grid[0][5][0].type = &router_tile;
        test_grid[0][5][0].height_offset = 0;
        test_grid[0][5][0].width_offset = 1;

        test_grid[0][4][1].type = &router_tile;
        test_grid[0][4][1].height_offset = 1;
        test_grid[0][4][1].width_offset = 0;

        test_grid[0][5][1].type = &router_tile;
        test_grid[0][5][1].height_offset = 1;
        test_grid[0][5][1].width_offset = 1;

        // (4, 4)
        test_grid[0][4][4].type = &router_tile;
        test_grid[0][4][4].height_offset = 0;
        test_grid[0][4][4].width_offset = 0;

        test_grid[0][5][4].type = &router_tile;
        test_grid[0][5][4].height_offset = 0;
        test_grid[0][5][4].width_offset = 1;

        test_grid[0][4][5].type = &router_tile;
        test_grid[0][4][5].height_offset = 1;
        test_grid[0][4][5].width_offset = 0;

        test_grid[0][5][5].type = &router_tile;
        test_grid[0][5][5].height_offset = 1;
        test_grid[0][5][5].width_offset = 1;

        // (4, 8)
        test_grid[0][4][8].type = &router_tile;
        test_grid[0][4][8].height_offset = 0;
        test_grid[0][4][8].width_offset = 0;

        test_grid[0][5][8].type = &router_tile;
        test_grid[0][5][8].height_offset = 0;
        test_grid[0][5][8].width_offset = 1;

        test_grid[0][4][9].type = &router_tile;
        test_grid[0][4][9].height_offset = 1;
        test_grid[0][4][9].width_offset = 0;

        test_grid[0][5][9].type = &router_tile;
        test_grid[0][5][9].height_offset = 1;
        test_grid[0][5][9].width_offset = 1;

        // (8, 0)
        test_grid[0][8][0].type = &router_tile;
        test_grid[0][8][0].height_offset = 0;
        test_grid[0][8][0].width_offset = 0;

        test_grid[0][9][0].type = &router_tile;
        test_grid[0][9][0].height_offset = 0;
        test_grid[0][9][0].width_offset = 1;

        test_grid[0][8][1].type = &router_tile;
        test_grid[0][8][1].height_offset = 1;
        test_grid[0][8][1].width_offset = 0;

        test_grid[0][9][1].type = &router_tile;
        test_grid[0][9][1].height_offset = 1;
        test_grid[0][9][1].width_offset = 1;

        // (8, 4)
        test_grid[0][8][4].type = &router_tile;
        test_grid[0][8][4].height_offset = 0;
        test_grid[0][8][4].width_offset = 0;

        test_grid[0][9][4].type = &router_tile;
        test_grid[0][9][4].height_offset = 0;
        test_grid[0][9][4].width_offset = 1;

        test_grid[0][8][5].type = &router_tile;
        test_grid[0][8][5].height_offset = 1;
        test_grid[0][8][5].width_offset = 0;

        test_grid[0][9][5].type = &router_tile;
        test_grid[0][9][5].height_offset = 1;
        test_grid[0][9][5].width_offset = 1;

        // (8, 8)
        test_grid[0][8][8].type = &router_tile;
        test_grid[0][8][8].height_offset = 0;
        test_grid[0][8][8].width_offset = 0;

        test_grid[0][9][8].type = &router_tile;
        test_grid[0][9][8].height_offset = 0;
        test_grid[0][9][8].width_offset = 1;

        test_grid[0][8][9].type = &router_tile;
        test_grid[0][8][9].height_offset = 1;
        test_grid[0][8][9].width_offset = 0;

        test_grid[0][9][9].type = &router_tile;
        test_grid[0][9][9].height_offset = 1;
        test_grid[0][9][9].width_offset = 1;

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tile is not a router
                if (test_grid[0][i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[0][i][j].type = &empty_tile;
                    test_grid[0][i][j].width_offset = 0;
                    test_grid[0][i][j].height_offset = 0;
                }
            }
        }

        std::random_device device;
        std::mt19937 rand_num_gen(device());
        std::uniform_int_distribution<std::mt19937::result_type> dist(1, 9);

        constexpr double LINK_LATENCY_OVERRIDE = 142.2;
        constexpr double LINK_BANDWIDTH_OVERRIDE = 727.4;
        constexpr double ROUTER_LATENCY_OVERRIDE = 151.6;
        REQUIRE(LINK_LATENCY_OVERRIDE != noc_info.link_latency);
        REQUIRE(LINK_BANDWIDTH_OVERRIDE != noc_info.link_bandwidth);
        REQUIRE(ROUTER_LATENCY_OVERRIDE != noc_info.router_latency);

        // add router latency overrides
        for (int i = 0; i < 3; i++) {
            int noc_router_user_id = dist(rand_num_gen);
            noc_info.router_latency_overrides.insert({noc_router_user_id, ROUTER_LATENCY_OVERRIDE});
        }

        // add link latency overrides
        for (int i = 0; i < 3; i++) {
            int noc_router_user_id = dist(rand_num_gen);
            size_t n_connections = noc_info.router_list[noc_router_user_id - 1].connection_list.size();
            int selected_connection = dist(rand_num_gen) % n_connections;
            int neighbor_router_user_id = noc_info.router_list[noc_router_user_id - 1].connection_list[selected_connection];
            noc_info.link_latency_overrides.insert({{noc_router_user_id, neighbor_router_user_id}, LINK_LATENCY_OVERRIDE});
        }

        // add link bandwidth overrides
        for (int i = 0; i < 3; i++) {
            int noc_router_user_id = dist(rand_num_gen);
            size_t n_connections = noc_info.router_list[noc_router_user_id - 1].connection_list.size();
            int selected_connection = dist(rand_num_gen) % n_connections;
            int neighbor_router_user_id = noc_info.router_list[noc_router_user_id - 1].connection_list[selected_connection];
            noc_info.link_bandwidth_overrides.insert({{noc_router_user_id, neighbor_router_user_id}, LINK_BANDWIDTH_OVERRIDE});
        }

        std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
        device_ctx.grid = DeviceGrid(device_grid_name, test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

        REQUIRE_NOTHROW(setup_noc(arch));

        const auto& noc_model = g_vpr_ctx.noc().noc_model;

        // check NoC router latencies
        for (const auto& noc_router : noc_model.get_noc_routers()) {
            int router_user_id = noc_router.get_router_user_id();
            auto it = noc_info.router_latency_overrides.find(router_user_id);
            double expected_latency = (it != noc_info.router_latency_overrides.end()) ? it->second : noc_info.router_latency;
            REQUIRE(expected_latency == noc_router.get_latency());
        }

        // check NoC link latencies and bandwidth
        for (const auto& noc_link : noc_model.get_noc_links()) {
            NocRouterId src_router_id = noc_link.get_source_router();
            NocRouterId dst_router_id = noc_link.get_sink_router();
            int src_user_id = noc_model.convert_router_id(src_router_id);
            int dst_user_id = noc_model.convert_router_id(dst_router_id);

            auto lat_it = noc_info.link_latency_overrides.find({src_user_id, dst_user_id});
            double expected_latency = (lat_it != noc_info.link_latency_overrides.end()) ? lat_it->second : noc_info.link_latency;
            REQUIRE(expected_latency == noc_link.get_latency());

            auto bw_it = noc_info.link_bandwidth_overrides.find({src_user_id, dst_user_id});
            double expected_bandwidth = (bw_it != noc_info.link_bandwidth_overrides.end()) ? bw_it->second : noc_info.link_bandwidth;
            REQUIRE(expected_bandwidth == noc_link.get_bandwidth());
        }

        // remove noc storage
        g_vpr_ctx.mutable_noc().noc_model.clear_noc();
    }
}

} // namespace
