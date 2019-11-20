#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"

#define ABS_DIFF(X, Y) (((X) > (Y))? ((X) - (Y)):((Y) - (X)))
#define MAX_X 50
#define MAX_LEN MAX_X*2

typedef struct relapos_rec_s {
	int num_rp[MAX_LEN];
} relapos_rec_t;

#ifdef PRINT_REL_POS_DISTR
void
print_relative_pos_distr(void)
{

	/* Prints out the probability distribution of the relative locations of *
	 * input pins on a net -- i.e. simulates 2-point net distance probability *
	 * distribution.                                                        */
#ifdef PRINT_REL_POS_DISTR
	FILE *out_bin_file;
	relapos_rec_t rp_rec;
#endif /* PRINT_REL_POS_DISTR */

	int inet, len, rp, src_x, src_y, dst_x, dst_y, del_x, del_y, min_del,
	sink_pin, sum;
	int *total_conn;
	int **relapos;
	double **relapos_distr;

	total_conn = (int *)my_malloc((nx + ny + 1) * sizeof(int));
	relapos = (int **)my_malloc((nx + ny + 1) * sizeof(int *));
	relapos_distr = (double **)my_malloc((nx + ny + 1) * sizeof(double *));
	for (len = 0; len <= nx + ny; len++)
	{
		relapos[len] = (int *)my_calloc(len / 2 + 1, sizeof(int));
		relapos_distr[len] =
		(double *)my_calloc((len / 2 + 1), sizeof(double));
	}

	for (inet = 0; inet < num_nets; inet++)
	{
		if (clb_net[inet].is_global == FALSE)
		{

			src_x = block[clb_net[inet].node_block[0]].x;
			src_y = block[clb_net[inet].node_block[0]].y;

			for (sink_pin = 1; sink_pin <= clb_net[inet].num_sinks;
					sink_pin++)
			{
				dst_x = block[clb_net[inet].node_block[sink_pin]].x;
				dst_y = block[clb_net[inet].node_block[sink_pin]].y;

				del_x = ABS_DIFF(dst_x, src_x);
				del_y = ABS_DIFF(dst_y, src_y);

				len = del_x + del_y;

				min_del = (del_x < del_y) ? del_x : del_y;

				if (!(min_del <= (len / 2)))
				{
					vpr_printf(TIO_MESSAGE_ERROR, "Error calculating relative location min_del = %d, len = %d\n",
						min_del, len);
					exit(1);
				}
				else
				{
					relapos[len][min_del]++;
				}
			}
		}
	}

#ifdef PRINT_REL_POS_DISTR
	out_bin_file =
	fopen("/jayar/b/b5/fang/vpr_test/wirelength/relapos2.bin", "rb+");
#endif /* PRINT_REL_POS_DISTR */

	for (len = 0; len <= nx + ny; len++)
	{
		sum = 0;
		for (rp = 0; rp <= len / 2; rp++)
		{
			sum += relapos[len][rp];
		}
		if (sum != 0)
		{
#ifdef PRINT_REL_POS_DISTR
			fseek(out_bin_file, sizeof(relapos_rec_t) * len,
					SEEK_SET);
			fread(&rp_rec, sizeof(relapos_rec_t), 1, out_bin_file);
#endif /* PRINT_REL_POS_DISTR */

			for (rp = 0; rp <= len / 2; rp++)
			{

				relapos_distr[len][rp] =
				(double)relapos[len][rp] / (double)sum;

				/* updating the binary record at "len" */
#ifdef PRINT_REL_POS_DISTR
				vpr_printf(TIO_MESSAGE_ERROR, "old %d increased by %d\n", rp_rec.num_rp[rp], relapos[len][rp]);
				rp_rec.num_rp[rp] += relapos[len][rp];
				vpr_printf(TIO_MESSAGE_ERROR, "becomes %d\n", rp_rec.num_rp[rp]);
#endif /* PRINT_REL_POS_DISTR */
			}
#ifdef PRINT_REL_POS_DISTR
			/* write back the updated record at "len" */
			fseek(out_bin_file, sizeof(relapos_rec_t) * len,
					SEEK_SET);
			fwrite(&rp_rec, sizeof(relapos_rec_t), 1, out_bin_file);
#endif /* PRINT_REL_POS_DISTR */

		}
		total_conn[len] = sum;
	}

	fprintf(stdout, "Source to sink relative positions:\n");
	for (len = 1; len <= nx + ny; len++)
	{
		if (total_conn[len] != 0)
		{
			fprintf(stdout, "Of 2-pin distance %d exists %d\n\n", len,
					total_conn[len]);
			for (rp = 0; rp <= len / 2; rp++)
			{
				fprintf(stdout, "\trp%d\t%d\t\t(%.5f)\n", rp,
						relapos[len][rp], relapos_distr[len][rp]);
			}
			fprintf(stdout, "----------------\n");
		}
	}

	free((void *)total_conn);
	for (len = 0; len <= nx + ny; len++)
	{
		free((void *)relapos[len]);
		free((void *)relapos_distr[len]);
	}
	free((void *)relapos);
	free((void *)relapos_distr);

#ifdef PRINT_REL_POS_DISTR
	fclose(out_bin_file);
#endif /* PRINT_REL_POS_DISTR */
}
#endif
