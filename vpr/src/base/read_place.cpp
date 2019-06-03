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

void read_place(const char* net_file,
                const char* place_file,
                bool verify_file_digests,
                const DeviceGrid& grid) {
    std::ifstream fstream(place_file);
    if (!fstream) {
        vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__,
                  "'%s' - Cannot open place file.\n",
                  place_file);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    std::string line;
    int lineno = 0;
    bool seen_netlist_id = false;
    bool seen_grid_dimensions = false;
    while (std::getline(fstream, line)) { //Parse line-by-line
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

            if (seen_grid_dimensions) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                          "Duplicate device grid dimensions specification");
            }

            size_t place_file_width = vtr::atou(tokens[2]);
            size_t place_file_height = vtr::atou(tokens[4]);
            if (grid.width() != place_file_width || grid.height() != place_file_height) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                          "Current FPGA size (%d x %d) is different from size when placement generated (%d x %d)",
                          grid.width(), grid.height(), place_file_width, place_file_height);
            }

            seen_grid_dimensions = true;

        } else if (tokens.size() == 4 || (tokens.size() == 5 && tokens[4][0] == '#')) {
            //Load the block location
            //
            //We should have 4 tokens of actual data, with an optional 5th (commented) token indicating VPR's
            //internal block number

            //Grid dimensions are required by this point
            if (!seen_grid_dimensions) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                          "Missing device grid size specification");
            }

            std::string block_name = tokens[0];
            int block_x = vtr::atoi(tokens[1]);
            int block_y = vtr::atoi(tokens[2]);
            int block_z = vtr::atoi(tokens[3]);

            ClusterBlockId blk_id = cluster_ctx.clb_nlist.find_block(block_name);

            if (place_ctx.block_locs.size() != cluster_ctx.clb_nlist.blocks().size()) {
                //Resize if needed
                place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());
            }

            //Set the location
            place_ctx.block_locs[blk_id].loc.x = block_x;
            place_ctx.block_locs[blk_id].loc.y = block_y;
            place_ctx.block_locs[blk_id].loc.z = block_z;

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno,
                      "Invalid line '%s' in placement file",
                      line.c_str());
        }
    }

    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}

void read_user_pad_loc(const char* pad_loc_file) {
    /* Reads in the locations of the IO pads from a file. */

    t_hash **hash_table, *h_ptr;
    int xtmp, ytmp;
    FILE* fp;
    char buf[vtr::bufsize], bname[vtr::bufsize], *ptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    VTR_LOG("\n");
    VTR_LOG("Reading locations of IO pads from '%s'.\n", pad_loc_file);
    fp = fopen(pad_loc_file, "r");
    if (!fp) vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__,
                       "'%s' - Cannot find IO pads location file.\n",
                       pad_loc_file);

    hash_table = alloc_hash_table();
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (is_io_type(cluster_ctx.clb_nlist.block_type(blk_id))) {
            insert_in_hash_table(hash_table, cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id));
            place_ctx.block_locs[blk_id].loc.x = OPEN; /* Mark as not seen yet. */
        }
    }

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            auto type = device_ctx.grid[i][j].type;
            if (is_io_type(type)) {
                for (int k = 0; k < type->capacity; k++) {
                    if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                        place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID; /* Flag for err. check */
                    }
                }
            }
        }
    }

    ptr = vtr::fgets(buf, vtr::bufsize, fp);

    while (ptr != nullptr) {
        ptr = vtr::strtok(buf, TOKENS, fp, buf);
        if (ptr == nullptr) {
            ptr = vtr::fgets(buf, vtr::bufsize, fp);
            continue; /* Skip blank or comment lines. */
        }

        if (strlen(ptr) + 1 < vtr::bufsize) {
            strcpy(bname, ptr);
        } else {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Block name exceeded buffer size of %zu characters", vtr::bufsize);
        }

        ptr = vtr::strtok(nullptr, TOKENS, fp, buf);
        if (ptr == nullptr) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Incomplete.\n");
        }
        sscanf(ptr, "%d", &xtmp);

        ptr = vtr::strtok(nullptr, TOKENS, fp, buf);
        if (ptr == nullptr) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Incomplete.\n");
        }
        sscanf(ptr, "%d", &ytmp);

        ptr = vtr::strtok(nullptr, TOKENS, fp, buf);
        if (ptr == nullptr) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Incomplete.\n");
        }
        int k;
        sscanf(ptr, "%d", &k);

        ptr = vtr::strtok(nullptr, TOKENS, fp, buf);
        if (ptr != nullptr) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Extra characters at end of line.\n");
        }

        h_ptr = get_hash_entry(hash_table, bname);
        if (h_ptr == nullptr) {
            VTR_LOG_WARN("[Line %d] Block %s invalid, no such IO pad.\n",
                         vtr::get_file_line_number_of_last_opened_file(), bname);
            ptr = vtr::fgets(buf, vtr::bufsize, fp);
            continue;
        }
        ClusterBlockId bnum(h_ptr->index);
        int i = xtmp;
        int j = ytmp;

        if (place_ctx.block_locs[bnum].loc.x != OPEN) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Block %s is listed twice in pad file.\n", bname);
        }

        if (i < 0 || i > int(device_ctx.grid.width() - 1) || j < 0 || j > int(device_ctx.grid.height() - 1)) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0,
                      "Block #%zu (%s) location, (%d,%d) is out of range.\n", size_t(bnum), bname, i, j);
        }

        place_ctx.block_locs[bnum].loc.x = i; /* Will be reloaded by initial_placement anyway. */
        place_ctx.block_locs[bnum].loc.y = j; /* We need to set .x only as a done flag.         */
        place_ctx.block_locs[bnum].loc.z = k;
        place_ctx.block_locs[bnum].is_fixed = true;

        auto type = device_ctx.grid[i][j].type;
        if (!is_io_type(type)) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0,
                      "Attempt to place IO block %s at illegal location (%d, %d).\n", bname, i, j);
        }

        if (k >= type->capacity || k < 0) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(),
                      "Block %s subblock number (%d) is out of range.\n", bname, k);
        }
        place_ctx.grid_blocks[i][j].blocks[k] = bnum;
        place_ctx.grid_blocks[i][j].usage++;

        ptr = vtr::fgets(buf, vtr::bufsize, fp);
    }

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto type = cluster_ctx.clb_nlist.block_type(blk_id);
        if (is_io_type(type) && place_ctx.block_locs[blk_id].loc.x == OPEN) {
            vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0,
                      "IO block %s location was not specified in the pad file.\n", cluster_ctx.clb_nlist.block_name(blk_id).c_str());
        }
    }

    fclose(fp);
    free_hash_table(hash_table);
    VTR_LOG("Successfully read %s.\n", pad_loc_file);
    VTR_LOG("\n");
}

/* Prints out the placement of the circuit. The architecture and    *
 * netlist files used to generate this placement are recorded in the *
 * file to avoid loading a placement with the wrong support files    *
 * later.                                                            */
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

            fprintf(fp, "%d\t%d\t%d", place_ctx.block_locs[blk_id].loc.x, place_ctx.block_locs[blk_id].loc.y, place_ctx.block_locs[blk_id].loc.z);
            fprintf(fp, "\t#%zu\n", size_t(blk_id));
        }
    }
    fclose(fp);

    //Calculate the ID of the placement
    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}
