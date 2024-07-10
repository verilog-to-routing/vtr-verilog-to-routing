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
#include "place_util.h"

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

    VTR_LOG("Successfully read constraints file %s.\n", constraints_file);
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

    // store the current position in file
    // if an invalid line is read, we might need to return to the beginning of that line
    std::streampos file_pos = placement_file.tellg();

    while (std::getline(placement_file, line) && (!seen_netlist_id || !seen_grid_dimensions)) { //Parse line-by-line
       ++lineno;

        std::vector<std::string> tokens = vtr::split(line);

        if (tokens.empty()) {
            continue; //Skip blank lines

        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines

        } else if (tokens.size() == 4 &&
                   tokens[0] == "Netlist_File:" &&
                   tokens[2] == "Netlist_ID:") {
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

            if (place_netlist_id != cluster_ctx.clb_nlist.netlist_id()) {
                auto msg = vtr::string_fmt(
                    "The packed netlist file that generated placement (File: '%s' ID: '%s')"
                    " does not match current netlist (File: '%s' ID: '%s')",
                    place_netlist_file.c_str(), place_netlist_id.c_str(),
                    net_file, cluster_ctx.clb_nlist.netlist_id().c_str());
                if (verify_file_digests) {
                    msg += " To ignore the packed netlist mismatch, use '--verify_file_digests off' command line option.";
                    vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, msg.c_str());
                } else {
                    VTR_LOGF_WARN(place_file, lineno, "%s\n", msg.c_str());
                    VTR_LOG_WARN("The packed netlist mismatch is ignored because"
                                 "--verify_file_digests command line option is off.");
                }
            }

            seen_netlist_id = true;

        } else if (tokens.size() == 7 &&
                   tokens[0] == "Array" &&
                   tokens[1] == "size:" &&
                   tokens[3] == "x" &&
                   tokens[5] == "logic" &&
                   tokens[6] == "blocks") {
            //Load the device grid dimensions

            size_t place_file_width = vtr::atou(tokens[2]);
            size_t place_file_height = vtr::atou(tokens[4]);
            if (grid.width() != place_file_width || grid.height() != place_file_height) {
                auto msg = vtr::string_fmt(
                    "Current FPGA size (%d x %d) is different from size when placement generated (%d x %d)",
                    grid.width(), grid.height(), place_file_width, place_file_height);
                if (verify_file_digests) {
                    msg += " To ignore this size mismatch, use '--verify_file_digests off' command line option.";
                    vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, msg.c_str());
                } else {
                    VTR_LOGF_WARN(place_file, lineno, "%s\n", msg.c_str());
                    VTR_LOG_WARN("The FPGA size mismatch is ignored because"
                                 "--verify_file_digests command line option is off.");
                }
            }

            seen_grid_dimensions = true;

        } else {
            //Unrecognized
            auto msg = vtr::string_fmt(
                "Invalid line '%s' in placement file header. "
                "Expected to see netlist filename and device size first.",
                line.c_str());

            if (verify_file_digests) {
                msg += " To ignore this unexpected line, use '--verify_file_digests off' command line option.";
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, msg.c_str());
            } else {
                VTR_LOGF_WARN(place_file, lineno, "%s\n", msg.c_str());
                VTR_LOG_WARN("Unexpected line in the placement file header is ignored because"
                             "--verify_file_digests command line option is off.");
            }

            if ((tokens.size() == 4 || (tokens.size() > 4 && tokens[4][0] == '#')) ||
                (tokens.size() == 5 || (tokens.size() > 5 && tokens[5][0] == '#'))) {
                placement_file.seekg(file_pos);
                break;
            }
        }

        // store the current position in the file before reading the next line
        // we might need to return to this position
        file_pos = placement_file.tellg();
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
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& atom_ctx = g_vpr_ctx.atom();

    std::string line;
    int lineno = 0;

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

        } else if ((tokens.size() == 4 || (tokens.size() > 4 && tokens[4][0] == '#')) ||
                   (tokens.size() == 5 || (tokens.size() > 5 && tokens[5][0] == '#'))) {
            //Load the block location
            //
            // If the place file corresponds to a 3D architecture, it should contain 5 tokens of actual data, with an optional 6th (commented) token indicating VPR's internal block number.
            // If it belongs to 2D architecture file, supported for backward compatability, We should have 4 tokens of actual data, with an optional 5th (commented) token indicating VPR's
            //internal block number
            int block_name_index = 0;
            int block_x_index = 1;
            int block_y_index = 2;
            int sub_tile_index_index = 3;
            int block_layer_index;
            if (tokens.size() == 4 || (tokens.size() > 4 && tokens[4][0] == '#')) {
                //2D architecture
                block_layer_index = -1;

            } else {
                // 3D architecture
                block_layer_index = 4;
            }

            std::string block_name = tokens[block_name_index];
            int block_x = vtr::atoi(tokens[block_x_index]);
            int block_y = vtr::atoi(tokens[block_y_index]);
            int sub_tile_index = vtr::atoi(tokens[sub_tile_index_index]);
            int block_layer;
            if (block_layer_index != -1) {
                block_layer = vtr::atoi(tokens[block_layer_index]);
            } else {
                block_layer = 0;
            }

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
                if (block_x != place_ctx.block_locs[blk_id].loc.x ||
                    block_y != place_ctx.block_locs[blk_id].loc.y ||
                    sub_tile_index != place_ctx.block_locs[blk_id].loc.sub_tile ||
                    block_layer != place_ctx.block_locs[blk_id].loc.layer) {
                    std::string cluster_name = cluster_ctx.clb_nlist.block_name(blk_id);
                    VPR_THROW(VPR_ERROR_PLACE,
                              "The location of cluster %s (#%d) is specified %d times in the constraints file with conflicting locations. \n"
                              "Its location was last specified with block %s. \n",
                              cluster_name.c_str(), blk_id, seen_blocks[blk_id] + 1, c_block_name);
                }
            }

            t_pl_loc loc;
            loc.x = block_x;
            loc.y = block_y;
            loc.sub_tile = sub_tile_index;
            loc.layer = block_layer;

            if (seen_blocks[blk_id] == 0) {
                set_block_location(blk_id, loc);
            }

            //need to lock down blocks if it is a constraints file
            if (!is_place_file) {
                place_ctx.block_locs[blk_id].is_fixed = true;
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
                 const char* place_file,
                 bool is_place_file) {
    FILE* fp;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    fp = fopen(place_file, "w");

    if (is_place_file) {
        fprintf(fp, "Netlist_File: %s Netlist_ID: %s\n",
                net_file,
                net_id);
        fprintf(fp, "Array size: %zu x %zu logic blocks\n\n", device_ctx.grid.width(), device_ctx.grid.height());
        fprintf(fp, "#block name\tx\ty\tsubblk\tlayer\tblock number\n");
        fprintf(fp, "#----------\t--\t--\t------\t-----\t------------\n");
    }

    if (!place_ctx.block_locs.empty()) { //Only if placement exists
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            // if block is not placed, skip (useful for printing legalizer output)
            if (!is_place_file && (place_ctx.block_locs[blk_id].loc.x == INVALID_X)) {
                continue;
            }
            fprintf(fp, "%s\t", cluster_ctx.clb_nlist.block_pb(blk_id)->name);
            if (strlen(cluster_ctx.clb_nlist.block_pb(blk_id)->name) < 8)
                fprintf(fp, "\t");

            fprintf(fp, "%d\t%d\t%d\t%d",
                    place_ctx.block_locs[blk_id].loc.x,
                    place_ctx.block_locs[blk_id].loc.y,
                    place_ctx.block_locs[blk_id].loc.sub_tile,
                    place_ctx.block_locs[blk_id].loc.layer);
            fprintf(fp, "\t#%zu\n", size_t(blk_id));
        }
    }
    fclose(fp);

    //Calculate the ID of the placement
    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}
