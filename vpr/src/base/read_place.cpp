#include <cstdio>
#include <cstring>
#include <fstream>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_digest.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "hash.h"
#include "read_place.h"
#include "read_xml_arch_file.h"

void read_place_header(
    std::ifstream& placement_file,
    const char* net_file,
    const char* place_file,
    bool verify_file_hashes,
    const DeviceGrid& grid);

void read_place_body(
    std::ifstream& placement_file,
    const char* place_file,
    bool is_place_file);

void read_place(
    const char* net_file,
    const char* place_file,
    bool verify_file_digests,
    const DeviceGrid& grid) {
    std::ifstream fstream(place_file);
    if (!fstream) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE_F,
                        "'%s' - Cannot open place file.\n",
                        place_file);
    }

    bool is_place_file = true;

    VTR_LOG("Reading %s.\n", place_file);
    VTR_LOG("\n");

    read_place_header(fstream, net_file, place_file, verify_file_digests, grid);
    read_place_body(fstream, place_file, is_place_file);

    VTR_LOG("Successfully read %s.\n", place_file);
    VTR_LOG("\n");
}

void read_constraints(const char* constraints_file) {
    std::ifstream fstream(constraints_file);
    if (!fstream) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE_F,
                        "'%s' - Cannot open constraints file.\n",
                        constraints_file);
    }

    bool is_place_file = false;

    VTR_LOG("Reading %s.\n", constraints_file);
    VTR_LOG("\n");

    read_place_body(fstream, constraints_file, is_place_file);

    VTR_LOG("Successfully read %s.\n", constraints_file);
    VTR_LOG("\n");
}

/**
 * This function reads the header (first two lines) of a placement file.
 * The header consists of two lines that specify the netlist file and grid size that were used when generating placement.
 * It checks whether the packed netlist file that generated the placement matches the current netlist file.
 * It also checks whether the FPGA grid size has stayed the same from when the placement was generated.
 * The verify_file_digests bool is used to decide whether to give a warning or an error if the netlist files do not match.
 * An error is given if the grid size has changed.
 */
void read_place_header(std::ifstream& placement_file,
                       const char* net_file,
                       const char* place_file,
                       bool verify_file_digests,
                       const DeviceGrid& grid) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::string line;
    int lineno = 0;
    bool seen_netlist_id = false;
    bool seen_grid_dimensions = false;

    while (std::getline(placement_file, line) && (!seen_netlist_id || !seen_grid_dimensions)) { //Parse line-by-line
        ++lineno;

        std::vector<std::string> tokens = vtr::split(line);

        if (tokens.empty()) {
            continue; //Skip blank lines

        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines

        } else if (tokens.size() == 4
                   && tokens[0] == "Netlist_File:"
                   && tokens[2] == "Netlist_ID:") {
            //Check that the netlist used to generate this placement matches the one loaded
            //
            //NOTE: this is an optional check which causes no errors if this line is missing.
            //      This ensures other tools can still generate placement files which can be loaded
            //      by VPR.

            if (seen_netlist_id) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                          "Duplicate Netlist_File/Netlist_ID specification");
            }

            std::string place_netlist_id = tokens[3];
            std::string place_netlist_file = tokens[1];

            if (place_netlist_id != cluster_ctx.clb_nlist.netlist_id().c_str()) {
                auto msg = vtr::string_fmt(
                    "The packed netlist file that generated placement (File: '%s' ID: '%s')"
                    " does not match current netlist (File: '%s' ID: '%s')",
                    place_netlist_file.c_str(), place_netlist_id.c_str(),
                    net_file, cluster_ctx.clb_nlist.netlist_id().c_str());
                if (verify_file_digests) {
                    vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, msg.c_str());
                } else {
                    VTR_LOGF_WARN(place_file, lineno, "%s\n", msg.c_str());
                }
            }

            seen_netlist_id = true;

        } else if (tokens.size() == 7
                   && tokens[0] == "Array"
                   && tokens[1] == "size:"
                   && tokens[3] == "x"
                   && tokens[5] == "logic"
                   && tokens[6] == "blocks") {
            //Load the device grid dimensions

            size_t place_file_width = vtr::atou(tokens[2]);
            size_t place_file_height = vtr::atou(tokens[4]);
            if (grid.width() != place_file_width || grid.height() != place_file_height) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                          "Current FPGA size (%d x %d) is different from size when placement generated (%d x %d)",
                          grid.width(), grid.height(), place_file_width, place_file_height);
            }

            seen_grid_dimensions = true;

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                      "Invalid line '%s' in placement file header",
                      line.c_str());
        }
    }
}

/**
 * This function reads either the body of a placement file or a constraints file.
 * A constraints file is in the same format as a placement file, but without the first two header lines.
 * If it is reading a place file it sets the x, y, and subtile locations of the blocks in the placement context.
 * If it is reading a constraints file it does the same and also marks the blocks as locked and marks the grid usage.
 * The bool is_place_file indicates if the file should be read as a place file (is_place_file = true)
 * or a constraints file (is_place_file = false).
 */
void read_place_body(std::ifstream& placement_file,
                     const char* place_file,
                     bool is_place_file) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::string line;
    int lineno = 0;

    if (place_ctx.block_locs.size() != cluster_ctx.clb_nlist.blocks().size()) {
        //Resize if needed
        place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());
    }

    //used to count how many times a block has been seen in the place/constraints file so duplicate blocks can be detected
    vtr::vector_map<ClusterBlockId, int> seen_blocks;

    //initialize seen_blocks
    for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
        int seen_count = 0;
        seen_blocks.insert(block_id, seen_count);
    }

    while (std::getline(placement_file, line)) { //Parse line-by-line
        ++lineno;

        std::vector<std::string> tokens = vtr::split(line);

        if (tokens.empty()) {
            continue; //Skip blank lines

        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines

        } else if (tokens.size() == 4 || (tokens.size() > 4 && tokens[4][0] == '#')) {
            //Load the block location
            //
            //We should have 4 tokens of actual data, with an optional 5th (commented) token indicating VPR's
            //internal block number

            std::string block_name = tokens[0];
            int block_x = vtr::atoi(tokens[1]);
            int block_y = vtr::atoi(tokens[2]);
            int sub_tile_index = vtr::atoi(tokens[3]);

            //c-style block name needed for printing block name in error messages
            char const* c_block_name = block_name.c_str();

            ClusterBlockId blk_id = cluster_ctx.clb_nlist.find_block(block_name);

            //If block name is not found in cluster netlist check if it is in atom netlist
            if (blk_id == ClusterBlockId::INVALID()) {
                AtomBlockId atom_blk_id = atom_ctx.nlist.find_block(block_name);

                if (atom_blk_id == AtomBlockId::INVALID()) {
                    VTR_LOG_WARN("Block %s has an invalid name and it is going to be skipped.\n", c_block_name);
                    continue;
                } else {
                    blk_id = atom_ctx.lookup.atom_clb(atom_blk_id); //getting the ClusterBlockId of the cluster that the atom is in
                }
            }

            //Check if block is listed multiple times with conflicting locations in constraints file
            if (seen_blocks[blk_id] > 0) {
                if (block_x != place_ctx.block_locs[blk_id].loc.x || block_y != place_ctx.block_locs[blk_id].loc.y || sub_tile_index != place_ctx.block_locs[blk_id].loc.sub_tile) {
                    VPR_THROW(VPR_ERROR_PLACE,
                              "The location of cluster %d is specified %d times in the constraints file with conflicting locations. \n"
                              "Its location was last specified with block %s. \n",
                              blk_id, seen_blocks[blk_id] + 1, c_block_name);
                }
            }

            //Check if block location is out of range of grid dimensions
            if (block_x < 0 || block_x > int(device_ctx.grid.width() - 1)
                || block_y < 0 || block_y > int(device_ctx.grid.height() - 1)) {
                VPR_THROW(VPR_ERROR_PLACE, "Block %s with ID %d is out of range at location (%d, %d). \n", c_block_name, blk_id, block_x, block_y);
            }

            //Set the location
            place_ctx.block_locs[blk_id].loc.x = block_x;
            place_ctx.block_locs[blk_id].loc.y = block_y;
            place_ctx.block_locs[blk_id].loc.sub_tile = sub_tile_index;

            //Check if block is at an illegal location

            auto physical_tile = device_ctx.grid[block_x][block_y].type;
            auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

            if (sub_tile_index >= physical_tile->capacity || sub_tile_index < 0) {
                VPR_THROW(VPR_ERROR_PLACE, "Block %s subtile number (%d) is out of range. \n", c_block_name, sub_tile_index);
            }

            if (!is_sub_tile_compatible(physical_tile, logical_block, place_ctx.block_locs[blk_id].loc.sub_tile)) {
                VPR_THROW(VPR_ERROR_PLACE, "Attempt to place block %s with ID %d at illegal location (%d, %d). \n", c_block_name, blk_id, block_x, block_y);
            }

            //need to lock down blocks  and mark grid block usage if it is a constraints file
            //for a place file, grid usage is marked during initial placement instead
            if (!is_place_file) {
                place_ctx.block_locs[blk_id].is_fixed = true;
                place_ctx.grid_blocks[block_x][block_y].blocks[sub_tile_index] = blk_id;
                if (seen_blocks[blk_id] == 0) {
                    place_ctx.grid_blocks[block_x][block_y].usage++;
                }
            }

            //mark the block as seen
            seen_blocks[blk_id]++;

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                      "Invalid line '%s' in file",
                      line.c_str());
        }
    }

    //For place files, check that all blocks have been read
    //For constraints files, not all blocks need to be read
    if (is_place_file) {
        for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
            if (seen_blocks[block_id] == 0) {
                VPR_THROW(VPR_ERROR_PLACE, "Block %d has not been read from the place file. \n", block_id);
            }
        }
    }

    //Want to make a hash for place file to be used during routing for error checking
    if (is_place_file) {
        place_ctx.placement_id = vtr::secure_digest_file(place_file);
    }
}

/**
 * @brief Prints out the placement of the circuit.
 *
 * The architecture and netlist files used to generate this placement are recorded
 * in the file to avoid loading a placement with the wrong support file later.
 */
void print_place(const char* net_file,
                 const char* net_id,
                 const char* place_file) {
    FILE* fp;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    fp = fopen(place_file, "w");

    fprintf(fp, "Netlist_File: %s Netlist_ID: %s\n",
            net_file,
            net_id);
    fprintf(fp, "Array size: %zu x %zu logic blocks\n\n", device_ctx.grid.width(), device_ctx.grid.height());
    fprintf(fp, "#block name\tx\ty\tsubblk\tblock number\n");
    fprintf(fp, "#----------\t--\t--\t------\t------------\n");

    if (!place_ctx.block_locs.empty()) { //Only if placement exists
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            fprintf(fp, "%s\t", cluster_ctx.clb_nlist.block_name(blk_id).c_str());
            if (strlen(cluster_ctx.clb_nlist.block_name(blk_id).c_str()) < 8)
                fprintf(fp, "\t");

            fprintf(fp, "%d\t%d\t%d", place_ctx.block_locs[blk_id].loc.x, place_ctx.block_locs[blk_id].loc.y, place_ctx.block_locs[blk_id].loc.sub_tile);
            fprintf(fp, "\t#%zu\n", size_t(blk_id));
        }
    }
    fclose(fp);

    //Calculate the ID of the placement
    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}
