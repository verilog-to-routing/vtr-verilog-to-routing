#ifndef PHYSICAL_TYPES_UTIL_H
#define PHYSICAL_TYPES_UTIL_H

#include "physical_types.h"

/********************************************************************
 *                                                                  *
 *  Physical types utility functions                                *
 *                                                                  *
 *  This source file contains several utilities that enable the     *
 *  interaction with the architecture's physical types.             *
 *  Mainly, the two classes of objects accessed by the utility      *
 *  functions in this file are the following:                       *
 *    - physical_tile_type: identifies a placeable tile within      *
 *                          the device grid.                        *
 *    - logical_block_tpye: identifies a clustered block type       *
 *                          within the clb_netlist                  *
 *                                                                  *
 *  All the following utilities are intended to ease the            *
 *  developement to access the above mentioned classes and perform  *
 *  some required operations with their data.                       *
 *                                                                  *
 *  Please classify such functions in this file                     *
 *                                                                  *
 * ******************************************************************/

/*
 * Terms definition.
 *
 * This comment helps in clarifying what some of the data types correspond to
 * and what their purpose is, for a better understanding of the utility routines.
 *
 *   - logical_pin_index: corresponds to the absolute pin index of a logical block type.
 *   - physical_pin_index: corresponds to the absolute pin index of a physical tile type.
 *   - sub tile: component of a physical tile type.
 *               For further information on sub tiles refer to the documentation at:
 *               https://docs.verilogtorouting.org/en/latest/arch/reference/#tag-%3Csub_tilename
 *   - capacity: corresponds to the total number of instances of a sub tile within the belonging
 *               physical tile.
 *
 * Given that, each physical tile type can be used to place a different number/type of logical block types
 * it is necessary to have at disposal utilities to correctly synchronize and connect a logical block
 * to a compatible physical tile.
 *
 * For instance, if there are multiple physical tile types compatible with a logical type, it must be assumed
 * that the there is no 1:1 mapping between the logical block pins and the physical tiles one.
 *
 * To clarify, imagine a situation as follows:
 *
 *   - BUFG logical block type:
 *
 *        *----------------*
 *        |                |
 *    --->| CLK        OUT |--->
 *        |                |
 *        *----------------*
 *
 *     The logical pin indices are:
 *       - CLK: 0
 *       - OUT: 1
 *
 *   - CLOCK TILE physical tile type, containing a BUFGCTRL sub tile of capacity of 2:
 *
 *        *--------------------------*
 *        |                          |
 *        |    BUFGCTRL sub tile     |
 *        |    Instance 1            |
 *        |    *----------------*    |
 *        |    |                |    |
 *  CLK_1 |--->| CLK        OUT |--->| OUT_1
 *  EN_1  |--->| EN             |    |
 *        |    |                |    |
 *        |    *----------------*    |
 *        |                          |
 *        |    BUFGCTRL sub tile     |
 *        |    Instance 2            |
 *        |    *----------------*    |
 *        |    |                |    |
 *  CLK_2 |--->| CLK        OUT |--->| OUT_2
 *  EN_2  |--->| EN             |    |
 *        |    |                |    |
 *        |    *----------------*    |
 *        |                          |
 *        *--------------------------*
 *
 *     The physical pin indices are:
 *       - CLK_1: 0
 *       - EN_1 : 1
 *       - OUT_1: 2
 *       - CLK_2: 3
 *       - EN_2 : 4
 *       - OUT_2: 5
 *
 * The BUFG logical block can be placed in a BUFGCTRL sub tile of the CLOCK TILE physical tile.
 * As visible in the diagram, the CLOCK TILE contains a total of 6 physical pins, 3 for each instance,
 * and the logical block contains only 2 pins, but still, it is compatible to be placed within
 * the BUFGCTRL sub tile.
 *
 * One of the purposes of the following utility functions is to correctly identify the relation
 * of a logical pin (e.g. CLK of the BUFG logical block type) with the corresponding physical tile index
 * (e.g. CLK_2 of the CLOCK TILE physical tile, in case the logical block is placed on the second instance
 * of the BUFGCTRL sub tile).
 *
 * With the assumption that there is no 1:1 mapping between logical block and sub tile pins
 * (e.g. EN pin in the BUFGCTRL sub tile), there is some extra computation and data structures
 * needed to correctly identify the relation between the pins.
 *
 * For instance, the following information are required:
 *   - mapping between logical and sub tile pins.
 *   - mapping between sub tile pins and absoulte physical pin
 *   - capacity instance of the sub tile
 *
 * With all the above information we can calculate correctly the connection between the CLK (logical pin)
 * and CLK_2 (physical pin) from the BUFG (logical block) and CLOCK TILE (physical tile).
 */

///@brief Returns true if the absolute physical pin index is an output of the given physical tile type
bool is_opin(int ipin, t_physical_tile_type_ptr type);

///@brief Returns true if the given physical tile type can implement a .input block type
bool is_input_type(t_physical_tile_type_ptr type);
///@brief Returns true if the given physical tile type can implement a .output block type
bool is_output_type(t_physical_tile_type_ptr type);
///@brief Returns true if the given physical tile type can implement either a .input or .output block type
bool is_io_type(t_physical_tile_type_ptr type);

/**
 * @brief Returns the corresponding physical pin based on the input parameters:
 *
 * - physical_tile
 * - relative_pin: this is the pin relative to a specific sub tile
 * - capacity location: absolute sub tile location
 *
 * Take the above CLOCK TILE example:
 *   - we want to get the absolute physical pin corresponding to the first pin
 *     of the second instance of the BUFGCTRL sub tile
 *
 *   int pin = get_physical_pin_from_capacity_location(clock_tile, 0, 1);
 *
 *   This function call returns the absolute pin index of the CLK_1 pin (assumed that it is the first pin of the sub tile).
 *   The value returned in this case is 3.
 *   Note: capacity and pin indices start from zero.
 */
int get_physical_pin_from_capacity_location(t_physical_tile_type_ptr physical_tile, int relative_pin, int capacity_location);

/**
 * @brief Returns a pair consisting of the absolute capacity location relative to the pin parameter
 *
 *
 * Take the above CLOCK TILE example:
 *   - given the CLOCK TILE and the index corresponding to the CLK_1 pin, we want the relative pin
 *     of one of its sub tiles at a particualr capacity location (i.e. sub tile instance).
 *
 * std::tie(absolute_capacity, relative_pin) = get_capacity_location_from_physical_pin(clock_tile, 3)
 *
 * The value returned is (1, 0), where:
 *   - 1 corresponds to the capacity location (sub tile instance) where the absoulte physical pin index (CLK_1) is connected
 *   - 0 corresponds to the relative pin index within the BUFGCTRL sub tile
 */
std::pair<int, int> get_capacity_location_from_physical_pin(t_physical_tile_type_ptr physical_tile, int pin);

///@brief Returns the name of the pin_index'th pin on the specified block type
std::string block_type_pin_index_to_name(t_physical_tile_type_ptr type, int pin_index);

///@brief Returns the name of the class_index'th pin class on the specified block type
std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type, int class_index);

///@brief Returns the physical tile type matching a given physical tile type name, or nullptr (if not found)
t_physical_tile_type_ptr find_tile_type_by_name(std::string name, const std::vector<t_physical_tile_type>& types);

int find_pin_class(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port, e_pin_type pin_type);

///@brief Returns the relative pin index within a sub tile that corresponds to the pin within the given port and its index in the port
int find_pin(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port);

///@brief Returns the maximum number of pins within a logical block
int get_max_num_pins(t_logical_block_type_ptr logical_block);

///@brief Verifies whether a given logical block is compatible with a given physical tile
bool is_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block);

///@brief Verifies whether a logical block and a relative placement location is compatible with a given physical tile
bool is_sub_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block, int sub_tile_loc);

/**
 * @brief Returns the first physical tile type that matches the logical block
 *
 * The order of the physical tiles suitable for the input logical block follows the order
 * with which the logical blocks appear in the architecture XML definition
 */
t_physical_tile_type_ptr pick_physical_type(t_logical_block_type_ptr logical_block);

/**
 * Returns the first logical block type that matches the physical tile
 *
 * The order of the logical blocks suitable for the input physical tile follows the order
 * with which the physical tiles appear in the architecture XML definition
 */
t_logical_block_type_ptr pick_logical_type(t_physical_tile_type_ptr physical_tile);

/**
 * @brief Returns the sub tile index (within 'physical_tile') corresponding to the
 * 'logical block'.
 *
 * This function will return the index for the first sub_tile that can accommodate
 * the logical block.
 *
 * It is typically called before/during placement,
 * when picking a sub-tile to fit a logical block
 */
int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block);
/**
 * @brief Returns the physical pin index (within 'physical_tile') corresponding to the
 * logical index ('pin' of the first instance of 'logical_block' within the physcial tile.
 *
 * This function is called before/during placement, when a sub tile index was not yet assigned.
 *
 * Throws an exception if the corresponding physical pin can't be found.
 */
int get_physical_pin(t_physical_tile_type_ptr physical_tile,
                     t_logical_block_type_ptr logical_block,
                     int pin);
/**
 * @brief Returns the physical pin index (within 'physical_tile') corresponding to the
 * logical index ('pin' of the first instance of 'logical_block' within the physcial tile.
 * This function considers if a given offset is in the range of sub tile capacity
 *
 *   (First pin index at current sub-tile)                                     (The wanted pin index)
 *
 *   |                                                               |<----- pin ------->|
 *   v                                                                                   v
 *
 *   |<----- capacity.low ----->|<----- capacity.low + 1 ----->| ... |<----- sub_tile_capacity ---->|
 *
 * Throws an exception if the corresponding physical pin can't be found.
 *
 * Take the above CLOCK TILE example:
 *   - we want to get the absolute physical pin corresponding to the CLK pin
 *     of the BUFG logical block placed at the second instance of the BUFGCTRL sub tile
 *
 *   int pin = get_physical_pin_at_sub_tile_location(clock_tile, bufg_block, 1, 0);
 *
 *   where the input params are:
 *     - clock_tile: CLOCK TILE
 *     - bufg_block: BUFG
 *     - 1: second absolute capacity instance in the CLOCK TILE
 *     - 0: logical pin index corresponding to CLK
 *
 *   This function call returns the absolute pin index of the CLK_1 pin (assumed that it is the first pin of the sub tile).
 *   The value returned in this case is 3.
 *   Note: capacity and pin indices start from zero.
 */
int get_physical_pin_at_sub_tile_location(t_physical_tile_type_ptr physical_tile,
                                          t_logical_block_type_ptr logical_block,
                                          int sub_tile_capacity,
                                          int pin);

/**
 * @brief Returns the sub tile index (within 'physical_tile') corresponding to the
 * 'logical block' by considering if a given offset is in the range of sub tile capacity
 */
int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block,
                                              int sub_tile_capacity);
/**
 * @brief Returns the physical pin index (within 'physical_tile') corresponding to the
 * logical index ('pin') of the 'logical_block' at sub-tile location 'sub_tile_index'.
 *
 * Throws an exception if the corresponding physical pin can't be found.
 */
int get_sub_tile_physical_pin(int sub_tile_index,
                              t_physical_tile_type_ptr physical_tile,
                              t_logical_block_type_ptr logical_block,
                              int pin);

/**
 * @brief Returns one of the physical ports of a tile corresponding to the port_name.
 * Given that each sub_tile's port that has exactly the same name has to be equivalent
 * one to the other, it is indifferent which port is returned.
 */
t_physical_tile_port find_tile_port_by_name(t_physical_tile_type_ptr type, const char* port_name);

/**
 * @brief Returns the physical tile port given the port name and the corresponding sub tile
 */
const t_physical_tile_port* get_port_by_name(t_sub_tile* sub_tile, const char* port_name);

/**
 * @brief Returns the logical block port given the port name and the corresponding logical block type
 */
const t_port* get_port_by_name(t_logical_block_type_ptr type, const char* port_name);

/**
 * @brief Returns the physical tile port given the pin name and the corresponding sub tile
 */
const t_physical_tile_port* get_port_by_pin(const t_sub_tile* sub_tile, int pin);

/**
 * @brief Returns the logical block port given the pin name and the corresponding logical block type
 */
const t_port* get_port_by_pin(t_logical_block_type_ptr type, int pin);

#endif
