#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "setup_noc.h"
#include "globals.h"
#include "physical_types.h"

// for comparing floats
#include "vtr_math.h"

/*Re-defining static functions in setup_noc.cpp that are tested here*/

static void identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid, std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles, std::string noc_router_tile_name) {
    int grid_width = device_grid.width();
    int grid_height = device_grid.height();

    int curr_tile_width;
    int curr_tile_height;
    int curr_tile_width_offset;
    int curr_tile_height_offset;
    std::string curr_tile_name;

    double curr_tile_centroid_x;
    double curr_tile_centroid_y;

    // go through the device
    for (int i = 0; i < grid_width; i++) {
        for (int j = 0; j < grid_height; j++) {
            // get some information from the current tile
            curr_tile_name.assign(device_grid[i][j].type->name);
            curr_tile_width_offset = device_grid[i][j].width_offset;
            curr_tile_height_offset = device_grid[i][j].height_offset;

            curr_tile_height = device_grid[i][j].type->height;
            curr_tile_width = device_grid[i][j].type->width;

            /* 
             * Only store the tile position if it is a noc router.
             * Additionally, since a router tile can span multiple grid locations, we only add the tile if the height and width offset are zero (this prevents the router from being added multiple times for each grid location it spans).
             */
            if (!(noc_router_tile_name.compare(curr_tile_name)) && !curr_tile_width_offset && !curr_tile_height_offset) {
                // calculating the centroid position of the current tile
                curr_tile_centroid_x = (curr_tile_width - 1) / (double)2 + i;
                curr_tile_centroid_y = (curr_tile_height - 1) / (double)2 + j;

                list_of_noc_router_tiles.push_back({i, j, curr_tile_centroid_x, curr_tile_centroid_y});
            }
        }
    }

    return;
}

static void create_noc_routers(const t_noc_inf& noc_info, NocStorage* noc_model, std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles) {
    // keep track of the shortest distance between a logical router and the curren physical router tile
    // also keep track of the corresponding physical router tile index (within the list)
    double shortest_distance;
    double curr_calculated_distance;
    int closest_physical_router;

    // information regarding physical router position
    double curr_physical_router_pos_x;
    double curr_physical_router_pos_y;

    // information regarding logical router position
    double curr_logical_router_position_x;
    double curr_logical_router_position_y;

    // keep track of the index of each physical router (this helps uniqely identify them)
    int curr_physical_router_index = 0;

    // keep track of the ids of the routers that ceate the case where multiple routers
    // have the same distance to a physical router tile
    int error_case_physical_router_index_1;
    int error_case_physical_router_index_2;

    // keep track of all the logical router and physical router assignments (their pairings)
    std::vector<int> router_assignments;
    router_assignments.resize(list_of_noc_router_tiles.size(), PHYSICAL_ROUTER_NOT_ASSIGNED);

    // Below we create all the routers within the NoC //

    // go through each logical router tile and assign it to a physical router on the FPGA
    for (auto logical_router = noc_info.router_list.begin(); logical_router != noc_info.router_list.end(); logical_router++) {
        // assign the shortest distance to a large value (this is done so that the first distance calculated and we can replace this)
        shortest_distance = LLONG_MAX;

        // get position of the current logical router
        curr_logical_router_position_x = logical_router->device_x_position;
        curr_logical_router_position_y = logical_router->device_y_position;

        closest_physical_router = 0;

        // the starting index of the physical router list
        curr_physical_router_index = 0;

        // initialze the router ids that track the error case where two physical router tiles have the same distance to a logical router
        // we initialize it to a in-valid index, so that it reflects the situation where we never hit this case
        error_case_physical_router_index_1 = INVALID_PHYSICAL_ROUTER_INDEX;
        error_case_physical_router_index_2 = INVALID_PHYSICAL_ROUTER_INDEX;

        // determine the physical router tile that is closest to the current logical router
        for (auto physical_router = list_of_noc_router_tiles.begin(); physical_router != list_of_noc_router_tiles.end(); physical_router++) {
            // get the position of the current physical router tile on the FPGA device
            curr_physical_router_pos_x = physical_router->tile_centroid_x;
            curr_physical_router_pos_y = physical_router->tile_centroid_y;

            // use euclidean distance to calculate the length between the current logical and physical routers
            curr_calculated_distance = sqrt(pow(abs(curr_physical_router_pos_x - curr_logical_router_position_x), 2.0) + pow(abs(curr_physical_router_pos_y - curr_logical_router_position_y), 2.0));

            // if the current distance is the same as the previous shortest distance
            if (vtr::isclose(curr_calculated_distance, shortest_distance)) {
                // store the ids of the two physical routers
                error_case_physical_router_index_1 = closest_physical_router;
                error_case_physical_router_index_2 = curr_physical_router_index;

            } else if (curr_calculated_distance < shortest_distance) // case where the current logical router is closest to the physical router tile
            {
                // update the shortest distance and then the closest router
                shortest_distance = curr_calculated_distance;
                closest_physical_router = curr_physical_router_index;
            }

            // update the index for the next physical router
            curr_physical_router_index++;
        }

        // check the case where two physical router tiles have the same distance to the given logical router
        if (error_case_physical_router_index_1 == closest_physical_router) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Router with ID:'%d' has the same distance to physical router tiles located at position (%d,%d) and (%d,%d). Therefore, no router assignment could be made.",
                            logical_router->id, list_of_noc_router_tiles[error_case_physical_router_index_1].grid_width_position, list_of_noc_router_tiles[error_case_physical_router_index_1].grid_height_position,
                            list_of_noc_router_tiles[error_case_physical_router_index_2].grid_width_position, list_of_noc_router_tiles[error_case_physical_router_index_2].grid_height_position);
        }

        // check if the current physical router was already assigned previously, if so then throw an error
        if (router_assignments[closest_physical_router] != PHYSICAL_ROUTER_NOT_ASSIGNED) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Routers with IDs:'%d' and '%d' are both closest to physical router tile located at (%d,%d) and the physical router could not be assigned multiple times.",
                            logical_router->id, router_assignments[closest_physical_router], list_of_noc_router_tiles[closest_physical_router].grid_width_position,
                            list_of_noc_router_tiles[closest_physical_router].grid_height_position);
        }

        // at this point, the closest logical router to the current physical router was found
        // so add the router to the NoC
        noc_model->add_router(logical_router->id, list_of_noc_router_tiles[closest_physical_router].grid_width_position,
                              list_of_noc_router_tiles[closest_physical_router].grid_height_position);

        // add the new assignment to the tracker
        router_assignments[closest_physical_router] = logical_router->id;
    }

    return;
}

static void create_noc_links(const t_noc_inf* noc_info, NocStorage* noc_model) {
    // the ids used to represent the routers in the NoC are not the same as the ones provided by the user in the arch desc file.
    // while going through the router connections, the user provided router ids are converted and then stored below before being used
    // in the links.
    NocRouterId source_router;
    NocRouterId sink_router;

    // store the id of each new link we create
    NocLinkId created_link_id;

    // start of by creating enough space for the list of outgoing links for each router in the NoC
    noc_model->make_room_for_noc_router_link_list();

    // go through each router and add its outgoing links to the NoC
    for (auto router = noc_info->router_list.begin(); router != noc_info->router_list.end(); router++) {
        // get the converted id of the current source router
        source_router = noc_model->convert_router_id(router->id);

        // go through all the routers connected to the current one and add links to the noc
        for (auto conn_router_id = router->connection_list.begin(); conn_router_id != router->connection_list.end(); conn_router_id++) {
            // get the converted id of the currently connected sink router
            sink_router = noc_model->convert_router_id(*conn_router_id);

            // add the link to the Noc
            noc_model->add_link(source_router, sink_router);
            ;
        }
    }

    return;
}

/*End of static function re-definition in setup_noc.cpp*/

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

    SECTION("All routers are seperated by one or more grid spaces") {
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

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[i][j].type == nullptr) {
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
    SECTION("All routers are horizontally connected to another router") {
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

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[i][j].type == nullptr) {
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
    SECTION("All routers are vertically connected to another router") {
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

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[i][j].type == nullptr) {
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
TEST_CASE("test_create_noc_routers", "[vpr_setup_noc]") {
    // datastructure to hold the list of physical tiles
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
    list_of_routers.push_back({0, 0, 0.5, 1});
    list_of_routers.push_back({4, 0, 4.5, 1});
    list_of_routers.push_back({8, 0, 8.5, 1});

    list_of_routers.push_back({0, 4, 0.5, 5});
    list_of_routers.push_back({4, 4, 4.5, 5});
    list_of_routers.push_back({8, 4, 8.5, 5});

    list_of_routers.push_back({0, 8, 0.5, 9});
    list_of_routers.push_back({4, 8, 4.5, 9});
    list_of_routers.push_back({8, 8, 8.5, 9});

    // create the noc model (to store the routers)
    NocStorage noc_model;

    // create the logical router list
    t_noc_inf noc_info;

    // pointer to each logical router
    t_router* temp_router = NULL;

    const vtr::vector<NocRouterId, NocRouter>* noc_routers = NULL;

    SECTION("Test create routers when logical routers match to exactly one physical router. The number of routers is less than whats on the FPGA.") {
        // start by creating all the logical routers
        // this is similiar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 7; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
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
            NocRouter test_router = noc_model.get_single_noc_router(noc_router_id);

            // now check that the proper physical router was assigned to
            REQUIRE(test_router.get_router_grid_position_x() == list_of_routers[router_id - 1].grid_width_position);
            REQUIRE(test_router.get_router_grid_position_y() == list_of_routers[router_id - 1].grid_height_position);
        }
    }
    SECTION("Test create routers when logical routers match to exactly one physical router. The number of routers is exacrly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similiar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 10; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
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
            NocRouter test_router = noc_model.get_single_noc_router(noc_router_id);

            // now check that the proper physical router was assigned to
            REQUIRE(test_router.get_router_grid_position_x() == list_of_routers[router_id - 1].grid_width_position);
            REQUIRE(test_router.get_router_grid_position_y() == list_of_routers[router_id - 1].grid_height_position);
        }
    }
    SECTION("Test create routers when a logical router can be matched to two physical routers. The number of routers is exactly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similiar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 9; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
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
        REQUIRE_THROWS_WITH(create_noc_routers(noc_info, &noc_model, list_of_routers), "Router with ID:'9' has the same distance to physical router tiles located at position (4,8) and (8,8). Therefore, no router assignment could be made.");
    }
    SECTION("Test create routers when a physical router can be matched to two logical routers. The number of routers is exactly the same as on the FPGA.") {
        // start by creating all the logical routers
        // this is similiar to the user provided a config file
        temp_router = new t_router;

        NocRouterId noc_router_id;

        for (int router_id = 1; router_id < 9; router_id++) {
            // we will have 6 logical routers that will take up the bottom and middle physical routers of the mesh
            temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
            temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
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
        REQUIRE_THROWS_WITH(create_noc_routers(noc_info, &noc_model, list_of_routers), "Routers with IDs:'9' and '5' are both closest to physical router tile located at (4,4) and the physical router could not be assigned multiple times.");
    }
}
TEST_CASE("test_create_noc_links", "[vpr_setup_noc]") {
    // datastructure to hold the list of physical tiles
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
    list_of_routers.push_back({0, 0, 0.5, 1});
    list_of_routers.push_back({4, 0, 4.5, 1});
    list_of_routers.push_back({8, 0, 8.5, 1});

    list_of_routers.push_back({0, 4, 0.5, 5});
    list_of_routers.push_back({4, 4, 4.5, 5});
    list_of_routers.push_back({8, 4, 8.5, 5});

    list_of_routers.push_back({0, 8, 0.5, 9});
    list_of_routers.push_back({4, 8, 4.5, 9});
    list_of_routers.push_back({8, 8, 8.5, 9});

    // create the noc model (to store the routers)
    NocStorage noc_model;

    // create the logical router list
    t_noc_inf noc_info;

    // pointer to each logical router
    t_router* temp_router = NULL;

    // start by creating all the logical routers
    // this is similiar to the user provided a config file
    temp_router = new t_router;

    for (int router_id = 1; router_id < 10; router_id++) {
        // we will have 9 logical routers that will take up all physical routers
        temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
        temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
        temp_router->id = router_id;
        noc_info.router_list.push_back(*temp_router);

        // add the router to the NoC
        noc_model.add_router(router_id, list_of_routers[router_id - 1].grid_width_position, list_of_routers[router_id - 1].grid_height_position);
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
    create_noc_links(&noc_info, &noc_model);

    NocRouterId current_source_router_id;
    NocRouterId current_destination_router_id;

    std::vector<int>::iterator router_connection;

    // now verify the created links
    for (int router_id = 1; router_id < 10; router_id++) {
        current_source_router_id = noc_model.convert_router_id(router_id);

        router_connection = noc_info.router_list[router_id - 1].connection_list.begin();

        for (auto noc_link = noc_model.get_noc_router_connections(current_source_router_id).begin(); noc_link != noc_model.get_noc_router_connections(current_source_router_id).begin(); noc_link++) {
            
            // get the connecting link
            NocLink connecting_link = noc_model.get_single_noc_link(*noc_link);
            
            // get the destination router
            current_destination_router_id = connecting_link.get_sink_router();
            NocRouter current_destination_router = noc_model.get_single_noc_router(current_destination_router_id);



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
    t_router* temp_router = NULL;

    // start by creating all the logical routers
    // this is similiar to the user provided a config file
    temp_router = new t_router;

    // datastructure to hold the list of physical tiles
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
    list_of_routers.push_back({0, 0, 0.5, 1});
    list_of_routers.push_back({4, 0, 4.5, 1});
    list_of_routers.push_back({8, 0, 8.5, 1});

    list_of_routers.push_back({0, 4, 0.5, 5});
    list_of_routers.push_back({4, 4, 4.5, 5});
    list_of_routers.push_back({8, 4, 8.5, 5});

    list_of_routers.push_back({0, 8, 0.5, 9});
    list_of_routers.push_back({4, 8, 4.5, 9});
    list_of_routers.push_back({8, 8, 8.5, 9});

    for (int router_id = 1; router_id < 10; router_id++) {
        // we will have 9 logical routers that will take up all physical routers
        temp_router->device_x_position = list_of_routers[router_id - 1].tile_centroid_x;
        temp_router->device_y_position = list_of_routers[router_id - 1].tile_centroid_y;
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

    // the architecture file has been setup to include the noc topology and we set the parameters below
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

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[i][j].type = &empty_tile;
                    test_grid[i][j].width_offset = 0;
                    test_grid[i][j].height_offset = 0;
                }
            }
        }

        device_ctx.grid = DeviceGrid(device_grid_name, test_grid);

        REQUIRE_THROWS_WITH(setup_noc(arch), "The Provided NoC topology information in the architecture file has more number of routers than what is available in the FPGA device.");
    }
    SECTION("Test setup_noc when there are no  physical NoC routers on the FPGA.") {
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

        // in this test, there should be no physical router tiles ih the FPGA device and there should be no routers in the topology description

        for (int i = 0; i < test_grid_width; i++) {
            for (int j = 0; j < test_grid_height; j++) {
                // make sure the current tyle is not a router
                if (test_grid[i][j].type == nullptr) {
                    // assign the non-router tile as empty
                    test_grid[i][j].type = &empty_tile;
                    test_grid[i][j].width_offset = 0;
                    test_grid[i][j].height_offset = 0;
                }
            }
        }

        noc_info.router_list.clear();

        device_ctx.grid = DeviceGrid(device_grid_name, test_grid);

        REQUIRE_THROWS_WITH(setup_noc(arch), "No physical NoC routers were found on the FPGA device. Either the provided name for the physical router tile was incorrect or the FPGA device has no routers.");
    }
}

} // namespace