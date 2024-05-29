#ifndef SETUP_NOC
#define SETUP_NOC

/**
 * @file 
 * @brief This is the setup_noc header file. The main purpose of 
 * this file is to create a model of the NoC within FPGA using
 * the description provided by the user in the architecture
 * description file. The NoC can only be created here and modifying the
 * NoC elsewhere will cause an error after calling setup_noc().
 * 
 * Overview
 * ========
 * To create a NoC model, the setup_noc() function defined below
 * should be used. It performs the following steps:
 * 
 * Router Creation
 * ---------------
 * Each router described in the architecture file is created and
 * added to the NoC. Since the routers represents physical tiles on
 * the FPGA, when the router is created, it is also assigned to a
 * corresponding physical router tile. 
 * 
 * Link Creation
 * -------------
 * The user describes a "connection list", which represents an intended
 * connection between two routers in the NoC. Each link connects two
 * routers together. For each router, a number
 * of Links are created to connect it to another router in its "connection
 * list".
 * 
 */

#include <string_view>
#include <vector>

#include "device_grid.h"
#include "vpr_context.h"

// a data structure to store the position information of a noc router in the FPGA device
struct t_noc_router_tile_position {
    t_noc_router_tile_position(int x, int y, int layer_num, double centroid_x, double centroid_y)
        : grid_width_position(x)
        , grid_height_position(y)
        , layer_position(layer_num)
        , tile_centroid_x(centroid_x)
        , tile_centroid_y(centroid_y) {}

    int grid_width_position;
    int grid_height_position;
    int layer_position;

    double tile_centroid_x;
    double tile_centroid_y;
};

/**
 * @brief Based on the NoC information provided by the user in the architecture
 *        description file, a NoC model is created. The model defines the
 *        constraints of the NoC as well as its layout on the FPGA device. 
 *        The datastructure used to define the model is  "NocStorage" and that
 *        is created here and stored within the noc_ctx. 
 * 
 * @param arch Contains the parsed information from the architecture
 *             description file.
 */
void setup_noc(const t_arch& arch);

/**
 * @brief Goes through the FPGA device and identifies tiles that
 *        are NoC routers based on the name used to describe
 *        the router. Every identified routers grid position is
 *        stored in a list.
 * 
 * @param device_grid The FPGA device description.
 * @param noc_router_tile_name The name used when describing the NoC router
 *                             tile in the FPGA architecture description
 *                             file.
 *
 * @return The grid position information for all NoC router tiles in the FPGA.
 */
std::vector<t_noc_router_tile_position> identify_and_store_noc_router_tile_positions(const DeviceGrid& device_grid,
                                                                                     std::string_view noc_router_tile_name);

/**
 * @brief Creates NoC routers and adds them to the NoC model based
 *        on the routers provided by the user. Then the NoC links are
 *        created based on the topology. This completes the NoC
 *        model creation.
 * 
 * @param arch Contains the parsed information from the architecture
 *             description file.
 * @param noc_ctx A global variable that contains the NoC Model and other
 *                NoC related information.
 * @param list_of_noc_router_tiles Stores the grid position information
 *                                 for all NoC router tiles in the FPGA.
 */
void generate_noc(const t_arch& arch,
                  NocContext& noc_ctx,
                  const std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles);

/**
 * @brief Go through the routers described by the user
 *        in the architecture description file and assigns it a corresponding
 *        physical router tile in the FPGA. Each logical router has a grid
 *        location, so the closest physical router to the grid location is then
 *        assigned to it. Once a physical router is assigned, a NoC router
 *        is created to represent it and this is added to the NoC model.
 * 
 * @param noc_info Contains the parsed NoC topology information from the
 *                 architecture description file.
 * @param noc_model An internal model that describes the NoC. Contains a list of
 *                  routers and links that connect the routers together.
 * @param list_of_noc_router_tiles Stores the grid position information
 *                                 for all NoC router tiles in the FPGA.
 */
void create_noc_routers(const t_noc_inf& noc_info,
                        NocStorage* noc_model,
                        const std::vector<t_noc_router_tile_position>& list_of_noc_router_tiles);

/**
 * @brief Goes through the topology information as described in the FPGA
 *        architecture description file and creates NoC links that are stored
 *        into the NoC Model. All the created NoC links describe how the routers
 *        are connected to each other.
 * 
 * @param noc_info Contains the parsed NoC topology information from the
 *                 architecture description file.
 * @param noc_model An internal model that describes the NoC. Contains a list of
 *                  routers and links that connect the routers together.
 */
void create_noc_links(const t_noc_inf& noc_info, NocStorage* noc_model);

#endif