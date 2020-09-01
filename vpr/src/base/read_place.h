#ifndef READ_PLACE_H
#define READ_PLACE_H

/**
 * This function is used to read a placement file.
 */
void read_place(
    const char* net_file,
    const char* place_file,
    bool verify_file_hashes,
    const DeviceGrid& grid,
    bool is_place_file);

/**
 * This function is used to read a constraints file that specifies the desired locations of blocks.
 */
void read_constraints(
    const char* constraints_file,
    bool is_place_file);

void print_place(const char* net_file,
                 const char* net_id,
                 const char* place_file);

#endif
