#ifndef NOC_ROUTER_H
#define NOC_ROUTER_H

/**
 * @file
 * @brief This file defines the NocRouter class.
 *
 * Overview
 * ========
 * The NocRouter represents a physical router in the NoC.
 * The NocRouter acts as nodes in the NoC and is used as
 * entry points to the NoC. The NocRouters are created based
 * on the topology information provided by the user in the arch file.
 *
 * The NocRouter contains the following information:
 * - The router id. This represents the unique ID given by the
 * user in the architecture description file when describing a
 * router. The purpose of this is to help the user identify the
 * router when logging information or displaying errors.
 * - The grid position of the physical router tile this object
 * represents. Each router in the NoC represents a physical router
 * tile in the FPGA device. By storing the grid positions, we can quickly
 * get the corresponding physical router tile information by searching
 * the DeviceGrid in the device context.
 * - The design module (router cluster blocks) currently occupying this tile. Within the user
 * design there will be a number of instantiations of NoC routers. The user
 * will also provide information about which router blocks will be
 * communication with each other. During placement, it is possible for
 * the router blocks to move between the physical router tiles, so
 * by storing the module reference, we can determine which physical router tiles are communicating between each other and find a route
 * between them.
 */

#include <iostream>
#include <string>

#include "clustered_netlist.h"

class NocRouter {
  private:
    /** This represents a unique id provided by the user when describing the NoC topology in the arch file. The intended
     * use is to report errors with router ids the user understands*/
    int router_user_id;

    // device position of the physical router tile
    /** Represents the horizontal grid position on the device 
     * the physical router tile is located*/
    int router_grid_position_x;
    /** Represents the vertical grid position on the device 
     * the physical router is located*/
    int router_grid_position_y;
    /** Represents the layer number of the die 
     * that the physical router is located*/
    int router_layer_position;

    /** A unique identifier that represents a router block in the 
     * clustered netlist that is placed on the physical router*/
    ClusterBlockId router_block_ref;

  public:
    NocRouter(int id, int grid_position_x, int grid_position_y, int layer_position);

    // getters

    /**
     * @brief Gets the unique id assigned by the user for the physical router
     * @return A numerical value (integer) that represents the physical router id
     */
    int get_router_user_id(void) const;

    /**
     * @brief Gets the horizontal position on the FPGA device that the physical router is located
     * @return A numerical value (integer) that represents horizontal position of the physical router
     */
    int get_router_grid_position_x(void) const;

    /**
     * @brief Gets the vertical position on the FPGA device that the physical router is located
     * @return A numerical value (integer) that represents vertical position of the physical router
     */
    int get_router_grid_position_y(void) const;

    /**
     * @brief Gets the layer number of the die the the physical router is located
     * @return A numerical value (integer) that represents layer position of the physical router
     */
    int get_router_layer_position(void) const;

    /**
     * @brief Gets the unique id of the router block that is current placed on the physical router
     * @return A ClusterBlockId that identifies a router block in the clustered netlist
     */
    ClusterBlockId get_router_block_ref(void) const;

    // setters
    /**
     * @brief Sets the router block that is placed on the physical router
     * @param router_block_ref_id A ClusterBlockId that represents a router block
     */
    void set_router_block_ref(ClusterBlockId router_block_ref_id);
};

#endif