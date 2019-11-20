#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "read_place.h"


/* extern, should be a header */
char **ReadLineTokens(INOUT FILE * InFile,
		      INOUT int *LineNum);


void
read_place(IN const char *place_file,
	   IN const char *arch_file,
	   IN const char *net_file,
	   IN int nx,
	   IN int ny,
	   IN int num_blocks,
	   INOUT struct s_block block_list[])
{

    FILE *infile;
    char **tokens;
    int line;
    int i;
    int error;
    struct s_block *cur_blk;

    infile = my_fopen(place_file, "r");

    /* Check filenames in first line match */
    tokens = ReadLineTokens(infile, &line);
    error = 0;
    if(NULL == tokens)
	{
	    error = 1;
	}
    for(i = 0; i < 6; ++i)
	{
	    if(!error)
		{
		    if(NULL == tokens[i])
			{
			    error = 1;
			}
		}
	}
    if(!error)
	{
	    if((0 != strcmp(tokens[0], "Netlist")) ||
	       (0 != strcmp(tokens[1], "file:")) ||
	       (0 != strcmp(tokens[3], "Architecture")) ||
	       (0 != strcmp(tokens[4], "file:")))
		{
		    error = 1;
		};
	}
    if(error)
	{
	    printf(ERRTAG
		   "'%s' - Bad filename specification line in placement file\n",
		   place_file);
	    exit(1);
	}
    if(0 != strcmp(tokens[2], arch_file))
	{
	    printf(ERRTAG
		   "'%s' - Architecture file that generated placement (%s) does "
		   "not match current architecture file (%s)\n", place_file,
		   tokens[2], arch_file);
	    exit(1);
	}
    if(0 != strcmp(tokens[5], net_file))
	{
	    printf(ERRTAG
		   "'%s' - Netlist file that generated placement (%s) does "
		   "not match current netlist file (%s)\n", place_file,
		   tokens[5], net_file);
	    exit(1);
	}

    /* Check array size in second line matches */
    tokens = ReadLineTokens(infile, &line);
    error = 0;
    if(NULL == tokens)
	{
	    error = 1;
	}
    for(i = 0; i < 7; ++i)
	{
	    if(!error)
		{
		    if(NULL == tokens[i])
			{
			    error = 1;
			}
		}
	}
    if(!error)
	{
	    if((0 != strcmp(tokens[0], "Array")) ||
	       (0 != strcmp(tokens[1], "size:")) ||
	       (0 != strcmp(tokens[3], "x")) ||
	       (0 != strcmp(tokens[5], "logic")) ||
	       (0 != strcmp(tokens[6], "blocks")))
		{
		    error = 1;
		};
	}
    if(error)
	{
	    printf(ERRTAG
		   "'%s' - Bad fpga size specification line in placement file\n",
		   place_file);
	    exit(1);
	}
    if((my_atoi(tokens[2]) != nx) || (my_atoi(tokens[4]) != ny))
	{
	    printf(ERRTAG
		   "'%s' - Current FPGA size (%d x %d) is different from "
		   "size when placement generated (%d x %d)\n", place_file,
		   nx, ny, my_atoi(tokens[2]), my_atoi(tokens[4]));
	    exit(1);
	}

    tokens = ReadLineTokens(infile, &line);
    while(tokens)
	{
	    /* Linear search to match pad to netlist */
	    cur_blk = NULL;
	    for(i = 0; i < num_blocks; ++i)
		{
		    if(0 == strcmp(block_list[i].name, tokens[0]))
			{
			    cur_blk = (block_list + i);
			    break;
			}
		}

	    /* Error if invalid block */
	    if(NULL == cur_blk)
		{
		    printf(ERRTAG "'%s':%d - Block in placement file does "
			   "not exist in netlist\n", place_file, line);
		    exit(1);
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

void
read_user_pad_loc(char *pad_loc_file)
{

/* Reads in the locations of the IO pads from a file. */

    struct s_hash **hash_table, *h_ptr;
    int iblk, i, j, xtmp, ytmp, bnum, k;
    FILE *fp;
    char buf[BUFSIZE], bname[BUFSIZE], *ptr;

    printf("\nReading locations of IO pads from %s.\n", pad_loc_file);
    linenum = 0;
    fp = my_fopen(pad_loc_file, "r");

    hash_table = alloc_hash_table();
    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    if(block[iblk].type == IO_TYPE)
		{
		    h_ptr =
			insert_in_hash_table(hash_table, block[iblk].name,
					     iblk);
		    block[iblk].x = OPEN;	/* Mark as not seen yet. */
		}
	}

    for(i = 0; i <= nx + 1; i++)
	{
	    for(j = 0; j <= ny + 1; j++)
		{
		    if(grid[i][j].type == IO_TYPE)
			{
			    for(k = 0; k < IO_TYPE->capacity; k++)
				grid[i][j].blocks[k] = OPEN;	/* Flag for err. check */
			}
		}
	}

    ptr = my_fgets(buf, BUFSIZE, fp);

    while(ptr != NULL)
	{
	    ptr = my_strtok(buf, TOKENS, fp, buf);
	    if(ptr == NULL)
		{
		    ptr = my_fgets(buf, BUFSIZE, fp);
		    continue;	/* Skip blank or comment lines. */
		}

	    strcpy(bname, ptr);

	    ptr = my_strtok(NULL, TOKENS, fp, buf);
	    if(ptr == NULL)
		{
		    printf("Error:  line %d is incomplete.\n", linenum);
		    exit(1);
		}
	    sscanf(ptr, "%d", &xtmp);

	    ptr = my_strtok(NULL, TOKENS, fp, buf);
	    if(ptr == NULL)
		{
		    printf("Error:  line %d is incomplete.\n", linenum);
		    exit(1);
		}
	    sscanf(ptr, "%d", &ytmp);

	    ptr = my_strtok(NULL, TOKENS, fp, buf);
	    if(ptr == NULL)
		{
		    printf("Error:  line %d is incomplete.\n", linenum);
		    exit(1);
		}
	    sscanf(ptr, "%d", &k);

	    ptr = my_strtok(NULL, TOKENS, fp, buf);
	    if(ptr != NULL)
		{
		    printf("Error:  extra characters at end of line %d.\n",
			   linenum);
		    exit(1);
		}

	    h_ptr = get_hash_entry(hash_table, bname);
	    if(h_ptr == NULL)
		{
		    printf("Error:  block %s on line %d: no such IO pad.\n",
			   bname, linenum);
		    exit(1);
		}
	    bnum = h_ptr->index;
	    i = xtmp;
	    j = ytmp;

	    if(block[bnum].x != OPEN)
		{
		    printf
			("Error:  line %d.  Block %s listed twice in pad file.\n",
			 linenum, bname);
		    exit(1);
		}

	    if(i < 0 || i > nx + 1 || j < 0 || j > ny + 1)
		{
		    printf("Error:  block #%d (%s) location\n", bnum, bname);
		    printf("(%d,%d) is out of range.\n", i, j);
		    exit(1);
		}

	    block[bnum].x = i;	/* Will be reloaded by initial_placement anyway. */
	    block[bnum].y = j;	/* I need to set .x only as a done flag.         */

	    if(grid[i][j].type != IO_TYPE)
		{
		    printf("Error:  attempt to place IO block %s in \n",
			   bname);
		    printf("an illegal location (%d, %d).\n", i, j);
		    exit(1);
		}

	    if(k >= IO_TYPE->capacity || k < 0)
		{
		    printf
			("Error:  Block %s subblock number (%d) on line %d is out of "
			 "range.\n", bname, k, linenum);
		    exit(1);
		}
	    grid[i][j].blocks[k] = bnum;
	    grid[i][j].usage++;

	    ptr = my_fgets(buf, BUFSIZE, fp);
	}

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    if(block[iblk].type == IO_TYPE && block[iblk].x == OPEN)
		{
		    printf
			("Error:  IO block %s location was not specified in "
			 "the pad file.\n", block[iblk].name);
		    exit(1);
		}
	}

    fclose(fp);
    free_hash_table(hash_table);
    printf("Successfully read %s.\n\n", pad_loc_file);
}


void
print_place(char *place_file,
	    char *net_file,
	    char *arch_file)
{

/* Prints out the placement of the circuit.  The architecture and    *
 * netlist files used to generate this placement are recorded in the *
 * file to avoid loading a placement with the wrong support files    *
 * later.                                                            */

    FILE *fp;
    int i;

    fp = my_fopen(place_file, "w");

    fprintf(fp, "Netlist file: %s   Architecture file: %s\n", net_file,
	    arch_file);
    fprintf(fp, "Array size: %d x %d logic blocks\n\n", nx, ny);
    fprintf(fp, "#block name\tx\ty\tsubblk\tblock number\n");
    fprintf(fp, "#----------\t--\t--\t------\t------------\n");

    for(i = 0; i < num_blocks; i++)
	{
	    fprintf(fp, "%s\t", block[i].name);
	    if(strlen(block[i].name) < 8)
		fprintf(fp, "\t");

	    fprintf(fp, "%d\t%d\t%d", block[i].x, block[i].y, block[i].z);
	    fprintf(fp, "\t#%d\n", i);
	}

    fclose(fp);
}
