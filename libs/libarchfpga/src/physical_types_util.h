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
 *    - physical_tile_type                                          *
 *    - logical_block_tpye                                          *
 *                                                                  *
 *  All the following utilities are intended to ease the            *
 *  developement to access the above mentioned classes and perform  *
 *  some required operations with their data.                       *
 *                                                                  *
 *  Please classify such functions in this file                     *
 *                                                                  *
 * ******************************************************************/

bool is_opin(int ipin, t_physical_tile_type_ptr type);
bool is_input_type(t_physical_tile_type_ptr type);
bool is_output_type(t_physical_tile_type_ptr type);
bool is_io_type(t_physical_tile_type_ptr type);

//Returns the corresponding physical pin based on the input parameters:
// - physical_tile
// - relative_pin: this is the pin relative to a specific sub tile
// - capacity location: absolute sub tile location
int get_physical_pin_from_capacity_location(t_physical_tile_type_ptr physical_tile, int relative_pin, int capacity_location);

//Returns a pair consisting of the absolute capacity location relative to the pin parameter
//and the relative pin within the sub tile
std::pair<int, int> get_capacity_location_from_physical_pin(t_physical_tile_type_ptr physical_tile, int pin);

//Returns the name of the pin_index'th pin on the specified block type
std::string block_type_pin_index_to_name(t_physical_tile_type_ptr type, int pin_index);

//Returns the name of the class_index'th pin class on the specified block type
std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type, int class_index);

//Returns the physical tile type matching a given physical tile type name, or nullptr (if not found)
t_physical_tile_type_ptr find_tile_type_by_name(std::string name, const std::vector<t_physical_tile_type>& types);

int find_pin_class(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port, e_pin_type pin_type);

int find_pin(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port);

//Returns the maximum number of pins within a logical block
int get_max_num_pins(t_logical_block_type_ptr logical_block);

//Verifies whether a given logical block is compatible with a given physical tile
bool is_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block);

//Verifies whether a logical block and a relative placement location is compatible with a given physical tile
bool is_sub_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block, int sub_tile_loc);

//Returns the first physical tile type that matches the logical block
//
//The order of the physical tiles suitable for the input logical block follows the order
//with which the logical blocks appear in the architecture XML definition
t_physical_tile_type_ptr pick_physical_type(t_logical_block_type_ptr logical_block);

//Returns the first logical block type that matches the physical tile
//
//The order of the logical blocks suitable for the input physical tile follows the order
//with which the physical tiles appear in the architecture XML definition
t_logical_block_type_ptr pick_logical_type(t_physical_tile_type_ptr physical_tile);

//Returns the sub tile index (within 'physical_tile') corresponding to the
//'logical block'.
//
//This function will return the index for the first sub_tile that can accommodate
//the logical block.
//
//It is typically called before/during placement,
//when picking a sub-tile to fit a logical block
int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block);

//Returns the physical pin index (within 'physical_tile') corresponding to the
//logical index ('pin' of the first instance of 'logical_block' within the physcial tile.
//
//This function is called before/during placement, when a sub tile index was not yet assigned.
//
//Throws an exception if the corresponding physical pin can't be found.
int get_physical_pin(t_physical_tile_type_ptr physical_tile,
                     t_logical_block_type_ptr logical_block,
                     int pin);

//Returns the physical pin index (within 'physical_tile') corresponding to the
//logical index ('pin' of the first instance of 'logical_block' within the physcial tile.
//This function considers if a given offset is in the range of sub tile capacity
//
//  (First pin index at current sub-tile)                                     (The wanted pin index)
//
//  |                                                               |<----- pin ------->|
//  v                                                                                   v
//
//  |<----- capacity.low ----->|<----- capacity.low + 1 ----->| ... |<----- sub_tile_capacity ---->|
//
//Throws an exception if the corresponding physical pin can't be found.
int get_physical_pin_at_sub_tile_location(t_physical_tile_type_ptr physical_tile,
                                          t_logical_block_type_ptr logical_block,
                                          int sub_tile_capacity,
                                          int pin);

//Returns the sub tile index (within 'physical_tile') corresponding to the
//'logical block' by considering if a given offset is in the range of sub tile capacity
int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block,
                                              int sub_tile_capacity);

//Returns the physical pin index (within 'physical_tile') corresponding to the
//logical index ('pin') of the 'logical_block' at sub-tile location 'sub_tile_index'.
//
//Throws an exception if the corresponding physical pin can't be found.
int get_sub_tile_physical_pin(int sub_tile_index,
                              t_physical_tile_type_ptr physical_tile,
                              t_logical_block_type_ptr logical_block,
                              int pin);

// Returns one of the physical ports of a tile corresponding to the port_name.
// Given that each sub_tile's port that has exactly the same name has to be equivalent
// one to the other, it is indifferent which port is returned.
t_physical_tile_port find_tile_port_by_name(t_physical_tile_type_ptr type, const char* port_name);

#endif
