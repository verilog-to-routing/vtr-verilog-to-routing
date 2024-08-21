#ifndef READ_PLACE_H
#define READ_PLACE_H

#include "vtr_vector_map.h"

#include <string>


class PlacerState;
class ClusterBlockId;
struct t_block_loc;

/**
 * This function is for reading a place file when placement is skipped.
 * It takes in the current netlist file and grid dimensions to check that they match those that were used when placement was generated.
 * The verify_file_hashes bool is used to decide whether to give a warning or an error if the netlist files do not match.
 */
std::string read_place(const char* net_file,
                       const char* place_file,
                       BlkLocRegistry& blk_loc_registry,
                       bool verify_file_hashes,
                       const DeviceGrid& grid);

/**
 * This function is used to read a constraints file that specifies the desired locations of blocks.
 */
void read_constraints(const char* constraints_file,
                      BlkLocRegistry& blk_loc_registry);

/**
 * This function prints out a place file.
 * @param is_place_file: defaults to true. If false, does not print file header; this is useful if
 *                       the output will be used as a constraints file. If is_place_file is false,
 *                       net_file and net_id parameters are not used and can be set to nullptr.
 *                       Note: if false, only placed clusters are printed - clusters without
 *                       placement coordinates (e.g. orphan clusters created during legalization
 *                       will not be included; this file is used as a placement constraints
 *                       file when running placement in order to place orphan clusters.
 */
std::string print_place(const char* net_file,
                        const char* net_id,
                        const char* place_file,
                        const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                        bool is_place_file = true);

#endif
