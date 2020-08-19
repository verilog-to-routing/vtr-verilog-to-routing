#ifndef READ_PLACE_H
#define READ_PLACE_H

void read_place_header(
    const char* net_file,
    const char* place_file,
    bool verify_file_hashes,
    const DeviceGrid& grid);

void read_place_body(
    const char* place_file,
    bool is_place_file);

void print_place(const char* net_file,
                 const char* net_id,
                 const char* place_file);

#endif
