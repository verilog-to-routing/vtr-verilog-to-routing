#ifndef READ_PLACE_H
#define READ_PLACE_H

/**
 * This function is for reading a place file when placement is skipped.
 * It takes in the current netlist file and grid dimensions to check that they match those that were used when placement was generated.
 * The verify_file_hashes bool is used to decide whether to give a warning or an error if the netlist files do not match.
 */
void read_place(
    const char* net_file,
    const char* place_file,
    bool verify_file_hashes,
    const DeviceGrid& grid);

/**
 * This function is used to read a constraints file that specifies the desired locations of blocks.
 */
void read_constraints(const char* constraints_file);

void print_place(const char* net_file,
                 const char* net_id,
                 const char* place_file);

#endif
