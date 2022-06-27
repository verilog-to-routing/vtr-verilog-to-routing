
#include <climits>
#include <cmath>

#include "setup_noc.h"

#include "vtr_assert.h"
#include "vpr_error.h"
#include "vtr_math.h"
#include "echo_files.h"

void setup_noc(const t_arch& arch) {
    // variable to store all the noc router tiles within the FPGA device
    // physical routers
    std::vector<t_noc_router_tile_position> noc_router_tiles;

    // get references to global variables
    auto& device_ctx = g_vpr_ctx.device();
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    // quick error check that the noc attribute of the arch is not empty
    // basically, no noc topology information was provided by the user in the arch file
    if (arch.noc == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "No NoC topology information was provided in the architecture file.");
    }

    // go through the FPGA grid and find the noc router tiles
    // then store the position
    identify_and_store_noc_router_tile_positions(device_ctx.grid, noc_router_tiles, arch.noc->noc_router_tile_name);

    // check whether the noc topology information provided uses more than the number of available routers in the FPGA
    if (noc_router_tiles.size() < arch.noc->router_list.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The Provided NoC topology information in the architecture file has more number of routers than what is available in the FPGA device.");
    } else if (noc_router_tiles.size() > arch.noc->router_list.size()) // check whether the noc topology information provided is using all the routers in the FPGA
    {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "The Provided NoC topology information in the architecture file uses less number of routers than what is available in the FPGA device.");
    } else if (noc_router_tiles.size() == 0) // case where no physical router tiles were found
    {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "No physical NoC routers were found on the FPGA device. Either the provided name for the physical router tile was incorrect or the FPGA device has no routers.");
    }

    // generate noc model
    generate_noc(arch, noc_ctx, noc_router_tiles);

    // store the general noc properties
    noc_ctx.noc_model.set_noc_link_bandwidth(arch.noc->link_bandwidth);
    noc_ctx.noc_model.set_noc_link_latency(arch.noc->link_latency);
    noc_ctx.noc_model.set_noc_router_latency(arch.noc->router_latency);

    // echo the noc info
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_NOC_MODEL)) {
        noc_ctx.noc_model.echo_noc(getEchoFileName(E_ECHO_NOC_MODEL));
    }

    return;
}

void identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid, std::vector<t_noc_router_tile_position>& noc_router_tiles, std::string noc_router_tile_name) {
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

                noc_router_tiles.push_back({i, j, curr_tile_centroid_x, curr_tile_centroid_y});
            }
        }
    }

    return;
}

void generate_noc(const t_arch& arch, NocContext& noc_ctx, std::vector<t_noc_router_tile_position>& noc_router_tiles) {
    // refrernces to the noc
    NocStorage* noc_model = &noc_ctx.noc_model;
    // reference to the noc description
    const t_noc_inf* noc_info = arch.noc;

    // initialize the noc
    noc_model->clear_noc();

    // create all the routers in the NoC
    create_noc_routers(*arch.noc, noc_model, noc_router_tiles);

    // create all the links in the NoC
    create_noc_links(noc_info, noc_model);

    // indicate that the NoC has been built, cannot be modified anymore
    noc_model->finished_building_noc();

    return;
}

void create_noc_routers(const t_noc_inf& noc_info, NocStorage* noc_model, std::vector<t_noc_router_tile_position>& noc_router_tiles) {
    // keep track of the shortest distance between a user described router (noc description in the arch file) and a physical router on the FPGA
    double shortest_distance;
    double curr_calculated_distance;
    // stores the index of a physical router within the noc_router_tiles that is closest to a given user described router
    int closest_physical_router;

    // information regarding physical router position
    double curr_physical_router_pos_x;
    double curr_physical_router_pos_y;

    // information regarding logical router position
    double curr_logical_router_position_x;
    double curr_logical_router_position_y;

    // keep track of the index of each physical router (this helps uniqely identify them)
    int curr_physical_router_index = 0;

    // keep track of the ids of the routers that create the case where multiple routers have the same distance to a physical router tile
    int error_case_physical_router_index_1;
    int error_case_physical_router_index_2;

    // keep track of the router assignments (store the user router id that was assigned to each physical router tile)
    // this is used in error checking, after determining the closest physical router for a user described router in the arch file, the datastructure below can be used to check if that physical router was already assigned previously
    std::vector<int> router_assignments;
    router_assignments.resize(noc_router_tiles.size(), PHYSICAL_ROUTER_NOT_ASSIGNED);

    // Below we create all the routers within the NoC //

    // go through each user desctibed router in the arch file and assign it to a physical router on the FPGA
    for (auto logical_router = noc_info.router_list.begin(); logical_router != noc_info.router_list.end(); logical_router++) {
        // assign the shortest distance to a large value (this is done so that the first distance calculated and we can replace this)
        shortest_distance = LLONG_MAX;

        // get position of the current logical router
        curr_logical_router_position_x = logical_router->device_x_position;
        curr_logical_router_position_y = logical_router->device_y_position;

        closest_physical_router = 0;

        // the starting index of the physical router list
        curr_physical_router_index = 0;

        // initialze the router ids that track the error case where two physical router tiles have the same distance to a user described router
        // we initialize it to a in-valid index, so that it reflects the situation where we never hit this case
        error_case_physical_router_index_1 = INVALID_PHYSICAL_ROUTER_INDEX;
        error_case_physical_router_index_2 = INVALID_PHYSICAL_ROUTER_INDEX;

        // determine the physical router tile that is closest to the current user described router in the arch file
        for (auto physical_router = noc_router_tiles.begin(); physical_router != noc_router_tiles.end(); physical_router++) {
            // get the position of the current physical router tile on the FPGA device
            curr_physical_router_pos_x = physical_router->tile_centroid_x;
            curr_physical_router_pos_y = physical_router->tile_centroid_y;

            // use euclidean distance to calculate the length between the current user described router and the physical router
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
                            logical_router->id, noc_router_tiles[error_case_physical_router_index_1].grid_width_position, noc_router_tiles[error_case_physical_router_index_1].grid_height_position,
                            noc_router_tiles[error_case_physical_router_index_2].grid_width_position, noc_router_tiles[error_case_physical_router_index_2].grid_height_position);
        }

        // check if the current physical router was already assigned previously, if so then throw an error
        if (router_assignments[closest_physical_router] != PHYSICAL_ROUTER_NOT_ASSIGNED) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Routers with IDs:'%d' and '%d' are both closest to physical router tile located at (%d,%d) and the physical router could not be assigned multiple times.",
                            logical_router->id, router_assignments[closest_physical_router], noc_router_tiles[closest_physical_router].grid_width_position,
                            noc_router_tiles[closest_physical_router].grid_height_position);
        }

        // at this point, the closest user described router to the current physical router was found
        // so add the router to the NoC
        noc_model->add_router(logical_router->id, noc_router_tiles[closest_physical_router].grid_width_position,
                              noc_router_tiles[closest_physical_router].grid_height_position);

        // add the new assignment to the tracker
        router_assignments[closest_physical_router] = logical_router->id;
    }

    return;
}

void create_noc_links(const t_noc_inf* noc_info, NocStorage* noc_model) {
    // the ids used to represent the routers in the NoC are not the same as the ones provided by the user in the arch desc file.
    // while going through the router connections, the user provided router ids are converted and then stored below before being used in the links.
    NocRouterId source_router;
    NocRouterId sink_router;

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
        }
    }

    return;
}
