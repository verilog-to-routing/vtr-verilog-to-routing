#include <cmath>

#include "setup_noc.h"

#include "globals.h"
#include "vpr_error.h"
#include "vtr_math.h"
#include "echo_files.h"

// a default condition that helps keep track of whether a physical router has been assigned to a logical router or not
static constexpr int PHYSICAL_ROUTER_NOT_ASSIGNED = -1;

// a default index used for initialization purposes. No router will have a negative index
static constexpr int INVALID_PHYSICAL_ROUTER_INDEX = -1;

void setup_noc(const t_arch& arch) {
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
    auto noc_router_tiles = identify_and_store_noc_router_tile_positions(device_ctx.grid, arch.noc->noc_router_tile_name);

    // check whether the noc topology information provided uses more than the number of available routers in the FPGA
    if (noc_router_tiles.size() < arch.noc->router_list.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The Provided NoC topology information in the architecture file "
                        "has more number of routers than what is available in the FPGA device.");
    } else if (noc_router_tiles.size() > arch.noc->router_list.size()) {
        // check whether the noc topology information provided is using all the routers in the FPGA
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The Provided NoC topology information in the architecture file "
                        "uses less number of routers than what is available in the FPGA device.");
    } else if (noc_router_tiles.empty()) {  // case where no physical router tiles were found
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "No physical NoC routers were found on the FPGA device. "
                        "Either the provided name for the physical router tile was incorrect or the FPGA device has no routers.");
    }

    // store the reference to device grid with
    // need to set this first before adding routers to the model
    noc_ctx.noc_model.set_device_grid_spec((int)device_ctx.grid.width(), (int)device_ctx.grid.height());

    // generate noc model
    generate_noc(arch, noc_ctx, noc_router_tiles);

    /* store the general noc properties
     * noc_ctx.noc_model.set_noc_link_bandwidth(...) is not called because all
     * link bandwidths were set when create_noc_links(...) was called.
     */
    noc_ctx.noc_model.set_noc_link_latency(arch.noc->link_latency);
    noc_ctx.noc_model.set_noc_router_latency(arch.noc->router_latency);

    // echo the noc info
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_NOC_MODEL)) {
        noc_ctx.noc_model.echo_noc(getEchoFileName(E_ECHO_NOC_MODEL));
    }
}

vtr::vector<int, t_noc_router_tile_position> identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid,
                                                                                          std::string_view noc_router_tile_name) {
    const int num_layers = device_grid.get_num_layers();
    const int grid_width = (int)device_grid.width();
    const int grid_height = (int)device_grid.height();

    vtr::vector<int, t_noc_router_tile_position> noc_router_tiles;

    // go through the device
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        for (int i = 0; i < grid_width; i++) {
            for (int j = 0; j < grid_height; j++) {
                // get some information from the current tile
                const auto& type = device_grid.get_physical_type({i, j, layer_num});
                int width_offset = device_grid.get_width_offset({i, j, layer_num});
                int height_offset = device_grid.get_height_offset({i, j, layer_num});

                std::string_view curr_tile_name = type->name;
                int curr_tile_width_offset = width_offset;
                int curr_tile_height_offset = height_offset;

                int curr_tile_height = type->height;
                int curr_tile_width = type->width;

                /*
                 * Only store the tile position if it is a noc router.
                 * Additionally, since a router tile can span multiple grid locations,
                 * we only add the tile if the height and width offset are zero (this prevents the router from being added multiple times for each grid location it spans).
                 */
                if (noc_router_tile_name == curr_tile_name && !curr_tile_width_offset && !curr_tile_height_offset) {
                    // calculating the centroid position of the current tile
                    float curr_tile_centroid_x = (curr_tile_width - 1) / 2.0f + i;
                    float curr_tile_centroid_y = (curr_tile_height - 1) / 2.0f + j;

                    noc_router_tiles.emplace_back(i, j, layer_num, curr_tile_centroid_x, curr_tile_centroid_y);
                }
            }
        }
    }

    return noc_router_tiles;
}

void generate_noc(const t_arch& arch,
                  NocContext& noc_ctx,
                  const vtr::vector<int, t_noc_router_tile_position>& noc_router_tiles) {
    // references to the noc
    NocStorage* noc_model = &noc_ctx.noc_model;
    // reference to the noc description
    const t_noc_inf& noc_info = *arch.noc;

    // initialize the noc
    noc_model->clear_noc();

    // create all the routers in the NoC
    create_noc_routers(*arch.noc, noc_model, noc_router_tiles);

    // create all the links in the NoC
    create_noc_links(noc_info, noc_model);

    // indicate that the NoC has been built, cannot be modified anymore
    noc_model->finished_building_noc();
}

void create_noc_routers(const t_noc_inf& noc_info,
                        NocStorage* noc_model,
                        const vtr::vector<int, t_noc_router_tile_position>& noc_router_tiles) {
    // keep track of the router assignments (store the user router id that was assigned to each physical router tile)
    // this is used in error checking, after determining the closest physical router for a user described router in the arch file,
    // the data structure below can be used to check if that physical router was already assigned previously
    std::vector<int> router_assignments;
    router_assignments.resize(noc_router_tiles.size(), PHYSICAL_ROUTER_NOT_ASSIGNED);

    // Below we create all the routers within the NoC //

    // go through each user described router in the arch file and assign it to a physical router on the FPGA
    for (const auto& logical_router : noc_info.router_list) {
        // keep track of the shortest distance between a user described router (noc description in the arch file) and a physical router on the FPGA
        float shortest_distance = std::numeric_limits<float>::max();

        // stores the index of a physical router within the noc_router_tiles that is closest to a given user described router
        int closest_physical_router = 0;

        // initialize the router ids that track the error case where two physical router tiles have the same distance to a user described router
        // we initialize it to an invalid index, so that it reflects the situation where we never hit this case
        int error_case_physical_router_index_1 = INVALID_PHYSICAL_ROUTER_INDEX;
        int error_case_physical_router_index_2 = INVALID_PHYSICAL_ROUTER_INDEX;

        // determine the physical router tile that is closest to the current user described router in the arch file
        for (const auto& [curr_physical_router_index, physical_router] : noc_router_tiles.pairs()) {
            // make sure that we only compute the distance between logical and physical routers on the same layer
            if (physical_router.layer_position != logical_router.device_layer_position) {
                continue;
            }

            // use Euclidean distance to calculate the length between the current user described router and the physical router
            float curr_calculated_distance = std::hypot(physical_router.tile_centroid_x - logical_router.device_x_position,
                                                        physical_router.tile_centroid_y - logical_router.device_y_position);

            // if the current distance is the same as the previous shortest distance
            if (vtr::isclose(curr_calculated_distance, shortest_distance)) {
                // store the ids of the two physical routers
                error_case_physical_router_index_1 = closest_physical_router;
                error_case_physical_router_index_2 = curr_physical_router_index;

            // case where the current logical router is closest to the physical router tile
            } else if (curr_calculated_distance < shortest_distance) {
                // update the shortest distance and then the closest router
                shortest_distance = curr_calculated_distance;
                closest_physical_router = curr_physical_router_index;
            }
        }

        // make sure that there was at least one physical router on the same layer as the logical router
        if (shortest_distance == std::numeric_limits<float>::max()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Router with ID:'%d' is located on layer %d, but no physical router was found on this layer.",
                            logical_router.id, logical_router.device_layer_position);
        }

        // check the case where two physical router tiles have the same distance to the given logical router
        if (error_case_physical_router_index_1 == closest_physical_router) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Router with ID:'%d' has the same distance to physical router tiles located at position (%d,%d) and (%d,%d). Therefore, no router assignment could be made.",
                            logical_router.id, noc_router_tiles[error_case_physical_router_index_1].grid_width_position, noc_router_tiles[error_case_physical_router_index_1].grid_height_position,
                            noc_router_tiles[error_case_physical_router_index_2].grid_width_position, noc_router_tiles[error_case_physical_router_index_2].grid_height_position);
        }

        // check if the current physical router was already assigned previously, if so then throw an error
        if (router_assignments[closest_physical_router] != PHYSICAL_ROUTER_NOT_ASSIGNED) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Routers with IDs:'%d' and '%d' are both closest to physical router tile located at (%d,%d) and the physical router could not be assigned multiple times.",
                            logical_router.id, router_assignments[closest_physical_router], noc_router_tiles[closest_physical_router].grid_width_position,
                            noc_router_tiles[closest_physical_router].grid_height_position);
        }

        auto it = noc_info.router_latency_overrides.find(logical_router.id);
        double router_latency = (it == noc_info.router_latency_overrides.end()) ? noc_info.router_latency : it->second;

        // at this point, the closest user described router to the current physical router was found
        // so add the router to the NoC
        noc_model->add_router(logical_router.id,
                              noc_router_tiles[closest_physical_router].grid_width_position,
                              noc_router_tiles[closest_physical_router].grid_height_position,
                              noc_router_tiles[closest_physical_router].layer_position,
                              router_latency);

        // add the new assignment to the tracker
        router_assignments[closest_physical_router] = logical_router.id;
    }
}

void create_noc_links(const t_noc_inf& noc_info, NocStorage* noc_model) {
    // the ids used to represent the routers in the NoC are not the same as the ones provided by the user in the arch desc file.
    // while going through the router connections, the user provided router ids are converted and then stored below before being used in the links.
    NocRouterId source_router;
    NocRouterId sink_router;

    // start of by creating enough space for the list of outgoing links for each router in the NoC
    noc_model->make_room_for_noc_router_link_list();

    // go through each router and add its outgoing links to the NoC
    for (const auto& router : noc_info.router_list) {
        // get the converted id of the current source router
        source_router = noc_model->convert_router_id(router.id);

        // go through all the routers connected to the current one and add links to the noc
        for (const auto conn_router_id : router.connection_list) {
            // get the converted id of the currently connected sink router
            sink_router = noc_model->convert_router_id(conn_router_id);

            // check if this link has an overridden latency
            auto lat_it = noc_info.link_latency_overrides.find({router.id, conn_router_id});
            // use the link-specific latency if it has an overridden latency, otherwise use the NoC-wide link latency
            double link_lat = (lat_it == noc_info.link_latency_overrides.end()) ? noc_info.link_latency : lat_it->second;

            // check if this link has an overridden bandwidth
            auto bw_it = noc_info.link_bandwidth_overrides.find({router.id, conn_router_id});
            // use the link-specific bandwidth if it has an overridden bandwidth, otherwise use the NoC-wide link bandwidth
            double link_bw = (bw_it == noc_info.link_bandwidth_overrides.end()) ? noc_info.link_bandwidth : bw_it->second;

            // add the link to the Noc
            noc_model->add_link(source_router, sink_router, link_bw, link_lat);
        }
    }
}
