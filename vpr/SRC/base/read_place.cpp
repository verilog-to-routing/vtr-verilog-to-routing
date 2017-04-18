#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "hash.h"
#include "read_place.h"
#include "read_xml_arch_file.h"

void read_place(const char *place_file, const char *arch_file,
		const char *net_file, const int L_nx, const int L_ny,
		const int L_num_blocks, struct s_block block_list[]) {

	FILE *infile;
    std::vector<std::string> tokens;
	int line = 0;
	int i;
	int error;
	struct s_block *cur_blk;

	infile = fopen(place_file, "r");
	if (!infile) vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Cannot find place file.\n", 
				place_file);
		
	/* Check filenames in first line match */
	tokens = vtr::ReadLineTokens(infile, &line);
	error = 0;
	if (tokens.size() < 6) {
		error = 1;
	}
	for (i = 0; i < 6; ++i) {
		if (!error) {
			if (tokens[i].empty()) {
				error = 1;
			}
		}
	}
	if (!error) {
		if (tokens[0] != "Netlist"
				|| tokens[1] != "file:"
				|| tokens[3] != "Architecture"
				|| tokens[4] != "file:") {
			error = 1;
		};
	}
	if (error) {
		vpr_throw(VPR_ERROR_PLACE_F, place_file, line, 
				"Bad filename specification line in placement file.");
	}

    //TODO: a more robust approach would be to save the hash (e.g. SHA256) of the net/arch files and compare those, rather than
    //the filepaths
	if (tokens[2] != net_file) {
		vpr_throw(VPR_ERROR_PLACE_F, place_file, line, 
				"Netlist file that generated placement (%s) does not match current netlist file (%s).\n", 
				tokens[2].c_str(), net_file);
	}
	if (tokens[5] != arch_file) {
		vpr_throw(VPR_ERROR_PLACE_F, place_file, line, 
				"Architecture file that generated placement (%s) does not match current architecture file (%s).\n", 
				tokens[5].c_str(), arch_file);
	}

	/* Check array size in second line matches */
	tokens = vtr::ReadLineTokens(infile, &line);
	error = 0;
	if (tokens.size() < 7) {
		error = 1;
	}
	for (i = 0; i < 7; ++i) {
		if (!error) {
			if (tokens[i].empty()) {
				error = 1;
			}
		}
	}
	if (!error) {
		if (tokens[0] != "Array"
				|| tokens[1] != "size:"
				|| tokens[3] != "x"
				|| tokens[5] != "logic"
				|| tokens[6] != "blocks") {
			error = 1;
		};
	}
	if (error) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, "'%s' - Bad FPGA size specification line in placement file.\n",
				place_file);
	}
	if ((vtr::atoi(tokens[2].c_str()) != L_nx) || (vtr::atoi(tokens[4].c_str()) != L_ny)) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Current FPGA size (%d x %d) is different from size when placement generated (%d x %d).\n", 
				place_file, L_nx, L_ny, vtr::atoi(tokens[2].c_str()), vtr::atoi(tokens[4].c_str()));
	}

    do {
        tokens = vtr::ReadLineTokens(infile, &line);

        if(tokens.empty()) {
            continue;
        }

		/* Linear search to match pad to netlist */
		cur_blk = NULL;
		for (i = 0; i < L_num_blocks; ++i) {
			if (block_list[i].name == tokens[0]) {
				cur_blk = (block_list + i);
				break;
			}
		}

		/* Error if invalid block */
		if (NULL == cur_blk) {
			vpr_throw(VPR_ERROR_PLACE_F, place_file, line, 
					"Block in placement file does not exist in netlist.\n");
		}

		/* Set pad coords */
		cur_blk->x = vtr::atoi(tokens[1].c_str());
		cur_blk->y = vtr::atoi(tokens[2].c_str());
		cur_blk->z = vtr::atoi(tokens[3].c_str());

	} while (!std::feof(infile));

	fclose(infile);
}

void read_user_pad_loc(const char *pad_loc_file) {

	/* Reads in the locations of the IO pads from a file. */

	struct s_hash **hash_table, *h_ptr;
	int iblk, i, j, xtmp, ytmp, bnum, k;
	FILE *fp;
	char buf[vtr::BUFSIZE], bname[vtr::BUFSIZE], *ptr;

	vtr::printf_info("\n");
	vtr::printf_info("Reading locations of IO pads from '%s'.\n", pad_loc_file);
	fp = fopen(pad_loc_file, "r");
	if (!fp) vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Cannot find IO pads location file.\n", 
				pad_loc_file);
		
	hash_table = alloc_hash_table();
	for (iblk = 0; iblk < num_blocks; iblk++) {
		if (block[iblk].type == IO_TYPE) {
			insert_in_hash_table(hash_table, block[iblk].name, iblk);
			block[iblk].x = OPEN; /* Mark as not seen yet. */
		}
	}

	for (i = 0; i <= nx + 1; i++) {
		for (j = 0; j <= ny + 1; j++) {
			if (grid[i][j].type == IO_TYPE) {
				for (k = 0; k < IO_TYPE->capacity; k++) {
					if (grid[i][j].blocks[k] != INVALID_BLOCK) {
						grid[i][j].blocks[k] = EMPTY_BLOCK; /* Flag for err. check */
					}
				}
			}
		}
	}

	ptr = vtr::fgets(buf, vtr::BUFSIZE, fp);

	while (ptr != NULL) {
		ptr = vtr::strtok(buf, TOKENS, fp, buf);
		if (ptr == NULL) {
			ptr = vtr::fgets(buf, vtr::BUFSIZE, fp);
			continue; /* Skip blank or comment lines. */
		}

        if(strlen(ptr) + 1 < vtr::BUFSIZE) {
            strcpy(bname, ptr);
        } else {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block name exceeded buffer size of %zu characters", vtr::BUFSIZE);
        }

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &xtmp);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &ytmp);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &k);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr != NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Extra characters at end of line.\n");
		}

		h_ptr = get_hash_entry(hash_table, bname);
		if (h_ptr == NULL) {
			vtr::printf_warning(__FILE__, __LINE__, 
					"[Line %d] Block %s invalid, no such IO pad.\n", vtr::get_file_line_number_of_last_opened_file(), bname);
			ptr = vtr::fgets(buf, vtr::BUFSIZE, fp);
			continue;
		}
		bnum = h_ptr->index;
		i = xtmp;
		j = ytmp;

		if (block[bnum].x != OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block %s is listed twice in pad file.\n", bname);
		}

		if (i < 0 || i > nx + 1 || j < 0 || j > ny + 1) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Block #%d (%s) location, (%d,%d) is out of range.\n", bnum, bname, i, j);
		}

		block[bnum].x = i; /* Will be reloaded by initial_placement anyway. */
		block[bnum].y = j; /* I need to set .x only as a done flag.         */
		block[bnum].z = k;
		block[bnum].is_fixed = true;

		if (grid[i][j].type != IO_TYPE) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Attempt to place IO block %s at illegal location (%d, %d).\n", bname, i, j);
		}

		if (k >= IO_TYPE->capacity || k < 0) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block %s subblock number (%d) is out of range.\n", bname, k);
		}
		grid[i][j].blocks[k] = bnum;
		grid[i][j].usage++;

		ptr = vtr::fgets(buf, vtr::BUFSIZE, fp);
	}

	for (iblk = 0; iblk < num_blocks; iblk++) {
		if (block[iblk].type == IO_TYPE && block[iblk].x == OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"IO block %s location was not specified in the pad file.\n", block[iblk].name);
		}
	}

	fclose(fp);
	free_hash_table(hash_table);
	vtr::printf_info("Successfully read %s.\n", pad_loc_file);
	vtr::printf_info("\n");
}

void print_place(const char *place_file, const char *net_file, const char *arch_file) {

	/* Prints out the placement of the circuit.  The architecture and    *
	 * netlist files used to generate this placement are recorded in the *
	 * file to avoid loading a placement with the wrong support files    *
	 * later.                                                            */

	FILE *fp;
	int i;

	fp = fopen(place_file, "w");

	fprintf(fp, "Netlist file: %s   Architecture file: %s\n", net_file,
			arch_file);
	fprintf(fp, "Array size: %d x %d logic blocks\n\n", nx, ny);
	fprintf(fp, "#block name\tx\ty\tsubblk\tblock number\n");
	fprintf(fp, "#----------\t--\t--\t------\t------------\n");

	for (i = 0; i < num_blocks; i++) {
		fprintf(fp, "%s\t", block[i].name);
		if (strlen(block[i].name) < 8)
			fprintf(fp, "\t");

		fprintf(fp, "%d\t%d\t%d", block[i].x, block[i].y, block[i].z);
		fprintf(fp, "\t#%d\n", i);
	}
	fclose(fp);
}
