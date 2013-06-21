#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "read_place.h"
#include "read_xml_arch_file.h"
#include "ReadLine.h"

/* extern, should be a header */
char **ReadLineTokens(INOUTP FILE * InFile, INOUTP int *LineNum);

void read_place(INP const char *place_file, INP const char *arch_file,
		INP const char *net_file, INP int L_nx, INP int L_ny,
		INP int L_num_blocks, INOUTP struct s_block block_list[]) {

	FILE *infile;
	char **tokens;
	int line;
	int i;
	int error;
	struct s_block *cur_blk;

	infile = fopen(place_file, "r");

	/* Check filenames in first line match */
	tokens = ReadLineTokens(infile, &line);
	error = 0;
	if (NULL == tokens) {
		error = 1;
	}
	for (i = 0; i < 6; ++i) {
		if (!error) {
			if (NULL == tokens[i]) {
				error = 1;
			}
		}
	}
	if (!error) {
		if ((0 != strcmp(tokens[0], "Netlist"))
				|| (0 != strcmp(tokens[1], "file:"))
				|| (0 != strcmp(tokens[3], "Architecture"))
				|| (0 != strcmp(tokens[4], "file:"))) {
			error = 1;
		};
	}
	if (error) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Bad filename specification line in placement file.\n", 
				place_file);
	}
	if (0 != strcmp(tokens[2], arch_file)) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Architecture file that generated placement (%s) does not match current architecture file (%s).\n", 
				place_file, tokens[2], arch_file);
	}
	if (0 != strcmp(tokens[5], net_file)) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Netlist file that generated placement (%s) does not match current netlist file (%s).\n", 
				place_file, tokens[5], net_file);
	}
	free(*tokens);
	free(tokens);

	/* Check array size in second line matches */
	tokens = ReadLineTokens(infile, &line);
	error = 0;
	if (NULL == tokens) {
		error = 1;
	}
	for (i = 0; i < 7; ++i) {
		if (!error) {
			if (NULL == tokens[i]) {
				error = 1;
			}
		}
	}
	if (!error) {
		if ((0 != strcmp(tokens[0], "Array"))
				|| (0 != strcmp(tokens[1], "size:"))
				|| (0 != strcmp(tokens[3], "x"))
				|| (0 != strcmp(tokens[5], "logic"))
				|| (0 != strcmp(tokens[6], "blocks"))) {
			error = 1;
		};
	}
	if (error) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, "'%s' - Bad FPGA size specification line in placement file.\n",
				place_file);
	}
	if ((my_atoi(tokens[2]) != L_nx) || (my_atoi(tokens[4]) != L_ny)) {
		vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Current FPGA size (%d x %d) is different from size when placement generated (%d x %d).\n", 
				place_file, L_nx, L_ny, my_atoi(tokens[2]), my_atoi(tokens[4]));
	}
	free(*tokens);
	free(tokens);

	tokens = ReadLineTokens(infile, &line);
	while (tokens) {
		/* Linear search to match pad to netlist */
		cur_blk = NULL;
		for (i = 0; i < L_num_blocks; ++i) {
			if (0 == strcmp(block_list[i].name, tokens[0])) {
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
		cur_blk->x = my_atoi(tokens[1]);
		cur_blk->y = my_atoi(tokens[2]);
		cur_blk->z = my_atoi(tokens[3]);

		/* Get next line */
		assert(*tokens);
		free(*tokens);
		free(tokens);
		tokens = ReadLineTokens(infile, &line);
	}

	fclose(infile);
}

void read_user_pad_loc(char *pad_loc_file) {

	/* Reads in the locations of the IO pads from a file. */

	struct s_hash **hash_table, *h_ptr;
	int iblk, i, j, xtmp, ytmp, bnum, k;
	FILE *fp;
	char buf[BUFSIZE], bname[BUFSIZE], *ptr;

	vpr_printf_info("\n");
	vpr_printf_info("Reading locations of IO pads from '%s'.\n", pad_loc_file);
	file_line_number = 0;
	fp = fopen(pad_loc_file, "r");

	hash_table = alloc_hash_table();
	for (iblk = 0; iblk < num_blocks; iblk++) {
		if (block[iblk].type == IO_TYPE) {
			h_ptr = insert_in_hash_table(hash_table, block[iblk].name, iblk);
			block[iblk].x = OPEN; /* Mark as not seen yet. */
		}
	}

	for (i = 0; i <= nx + 1; i++) {
		for (j = 0; j <= ny + 1; j++) {
			if (grid[i][j].type == IO_TYPE) {
				for (k = 0; k < IO_TYPE->capacity; k++) {
					if (grid[i][j].blocks[k] != INVALID) {
						grid[i][j].blocks[k] = EMPTY; /* Flag for err. check */
					}
				}
			}
		}
	}

	ptr = my_fgets(buf, BUFSIZE, fp);

	while (ptr != NULL) {
		ptr = my_strtok(buf, TOKENS, fp, buf);
		if (ptr == NULL) {
			ptr = my_fgets(buf, BUFSIZE, fp);
			continue; /* Skip blank or comment lines. */
		}

		strcpy(bname, ptr);

		ptr = my_strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &xtmp);

		ptr = my_strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &ytmp);

		ptr = my_strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &k);

		ptr = my_strtok(NULL, TOKENS, fp, buf);
		if (ptr != NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Extra characters at end of line.\n");
		}

		h_ptr = get_hash_entry(hash_table, bname);
		if (h_ptr == NULL) {
			vpr_printf_warning(__FILE__, __LINE__, 
					"[Line %d] Block %s invalid, no such IO pad.\n", file_line_number, bname);
			ptr = my_fgets(buf, BUFSIZE, fp);
			continue;
		}
		bnum = h_ptr->index;
		i = xtmp;
		j = ytmp;

		if (block[bnum].x != OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Block %s is listed twice in pad file.\n", bname);
		}

		if (i < 0 || i > nx + 1 || j < 0 || j > ny + 1) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Block #%d (%s) location, (%d,%d) is out of range.\n", bnum, bname, i, j);
		}

		block[bnum].x = i; /* Will be reloaded by initial_placement anyway. */
		block[bnum].y = j; /* I need to set .x only as a done flag.         */
		block[bnum].is_fixed = TRUE;

		if (grid[i][j].type != IO_TYPE) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Attempt to place IO block %s at illegal location (%d, %d).\n", bname, i, j);
		}

		if (k >= IO_TYPE->capacity || k < 0) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, file_line_number, 
					"Block %s subblock number (%d) is out of range.\n", bname, k);
		}
		grid[i][j].blocks[k] = bnum;
		grid[i][j].usage++;

		ptr = my_fgets(buf, BUFSIZE, fp);
	}

	for (iblk = 0; iblk < num_blocks; iblk++) {
		if (block[iblk].type == IO_TYPE && block[iblk].x == OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"IO block %s location was not specified in the pad file.\n", block[iblk].name);
		}
	}

	fclose(fp);
	free_hash_table(hash_table);
	vpr_printf_info("Successfully read %s.\n", pad_loc_file);
	vpr_printf_info("\n");
}

void print_place(char *place_file, char *net_file, char *arch_file) {

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
