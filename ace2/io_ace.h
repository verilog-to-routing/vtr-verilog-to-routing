#ifndef __ACE_IO_H__
#define __ACE_IO_H__

#include <stdio.h>

#include "ace.h"

#define BLIF_FILE_NAME_LEN 512

/*
 int          ace_io_read_activity (network_t *, FILE *,
 ace_pi_format_t, double, double);
 void         ace_io_print_activity (network_t *, FILE *);
 */

void ace_io_print_usage();
int ace_io_parse_argv(int argc, char ** argv, FILE ** BLIF, FILE ** IN_ACT,
		FILE ** OUT_ACT, char * blif_file_name, char * new_blif_file_name,
		ace_pi_format_t * pi_format, double *p, double * d, int * seed,
        char** clk_name);
void ace_io_print_activity(Abc_Ntk_t * ntk, FILE * fp);
int ace_io_read_activity(Abc_Ntk_t * ntk, FILE * in_act_file_desc,
		ace_pi_format_t pi_format, double p, double d, const char * clk_name);

#endif
