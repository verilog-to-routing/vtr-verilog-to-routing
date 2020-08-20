#include <cstdio>
#include <cstring>
#include <fstream>

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

/*This function reads the header of a placement file.
 * The header has two lines:
 * One for checking the netlist file
 * And another for checking grid dimensions
 */
void read_place_header(const char* net_file,
                       const char* place_file,
                       bool verify_file_digests,
                       const DeviceGrid& grid) {
    std::ifstream fstream(place_file);
    if (!fstream) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE_F,
                        "'%s' - Cannot open place file.\n",
                        place_file);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::string line;
    int lineno = 0;
    bool seen_netlist_id = false;
    bool read_header_lines = false;

    while (std::getline(fstream, line) && !read_header_lines) { //Parse line-by-line
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

            read_header_lines = true; //if you have read the grid dimensions you are done reading the place file header

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                      "Invalid line '%s' in placement file header",
                      line.c_str());
        }
    }
}

/*This function reads either the body of a placement file or a constraints file.
 * If it is reading a constraints file it marks blocks as locked as it reads in their locations.
 * The bool is_place_file indicates if the file should be read as a place file (is_place_file = true)
 * or a constraints file (is_place_file = false).
 */
void read_place_body(const char* place_file,
                     bool is_place_file) {
    std::ifstream fstream(place_file);
    if (!fstream) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE_F,
                        "'%s' - Cannot open place file.\n",
                        place_file);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    //Mark unseen blocks for error-checking for constraints file
    if (!is_place_file){
    	for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
    		place_ctx.block_locs[block_id].loc.x = OPEN; //Mark as not seen yet
    	}
    }

    std::string line;
    int lineno = 0;

    while (std::getline(fstream, line)) { //Parse line-by-line
        ++lineno;

        std::vector<std::string> tokens = vtr::split(line);

        if (tokens.empty()) {
            continue; //Skip blank lines

        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines

        } else if (is_place_file
                   && tokens.size() == 4
                   && tokens[0] == "Netlist_File:"
                   && tokens[2] == "Netlist_ID:") {
            continue; //Skip netlist line if place file, already checked in read_place_header

        } else if (is_place_file
                   && tokens.size() == 7
                   && tokens[0] == "Array"
                   && tokens[1] == "size:"
                   && tokens[3] == "x"
                   && tokens[5] == "logic"
                   && tokens[6] == "blocks") {
            continue; //Skip checking grid dimensions if place file, already checked in read_place_header

        } else if (tokens.size() == 4 || (tokens.size() == 5 && tokens[4][0] == '#')) {
            //Load the block location
            //
            //We should have 4 tokens of actual data, with an optional 5th (commented) token indicating VPR's
            //internal block number

            std::string block_name = tokens[0];
            int block_x = vtr::atoi(tokens[1]);
            int block_y = vtr::atoi(tokens[2]);
            int sub_tile_index = vtr::atoi(tokens[3]);

            ClusterBlockId blk_id = cluster_ctx.clb_nlist.find_block(block_name);

            //Check if block is listed twice in constraints file
            if(!is_place_file) {
            	if (place_ctx.block_locs[blk_id].loc.x != OPEN) {
            		VPR_THROW(VPR_ERROR_PLACE, "The block with ID %d is listed twice in the constraints file.\n", blk_id);
            	}
            }

            //Check if block location is out of range of grid dimensions
            if(!is_place_file) {
            	if (block_x < 0 || block_x > int(device_ctx.grid.width() - 1)
            		|| block_y < 0 || block_y > int(device_ctx.grid.height() - 1)) {

            		VPR_THROW(VPR_ERROR_PLACE, "The block with ID %d is out of range at location (%d, %d). \n", blk_id, block_x, block_y);
            	}
            }

            if (place_ctx.block_locs.size() != cluster_ctx.clb_nlist.blocks().size()) {
                //Resize if needed
                place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());
            }


            //Set the location
            place_ctx.block_locs[blk_id].loc.x = block_x;
            place_ctx.block_locs[blk_id].loc.y = block_y;
            place_ctx.block_locs[blk_id].loc.sub_tile = sub_tile_index;

            //need to lock down blocks  and mark grid block usage if it is a constraints file
            if (!is_place_file) {
                place_ctx.block_locs[blk_id].is_fixed = true;
                place_ctx.grid_blocks[block_x][block_y].blocks[sub_tile_index] = blk_id;
                place_ctx.grid_blocks[block_x][block_y].usage++;
            }

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                      "Invalid line '%s' in placement file",
                      line.c_str());
        }
    }

    if (!is_place_file) {
        VTR_LOG("Successfully read %s.\n", place_file);
        VTR_LOG("\n");
    }
    place_ctx.placement_id = vtr::secure_digest_file(place_file);
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
