#include <string.h>
#include <stdio.h>
#include "assert.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "read_sdc.h"
#include "read_blif.h"
#include "path_delay.h"

/* read_sdc.c is work in progress */

#define DO_NOT_ANALYSE -1

static FILE *sdc;

/***************** Subroutines local to this module *************************/

void get_sdc_tok(char * buffer, int num_lines);
boolean check_if_number(char * ptr);

/********************* Subroutine definitions *******************************/

void read_sdc(char * sdc_file, int num_clocks) {
/*This function reads the constraints from the SDC file *
* specified on the command line into float ** timing_   *
* constraints.  If no file is specified, it leaves      *
* timing_constraints pointing to NULL.                  */

	char buf[BUFSIZE];
	int num_lines = 0;
		
	if ((sdc = fopen(sdc_file, "r")) == NULL) {
		printf("SDC file %s not found.\n", sdc_file);
		printf("All clocks will be analysed together during timing analysis.\n\n");
		return;
	}
	
	/* Allocate matrix of timing constraints [0..num_clocks-1][0..num_clocks-1] */
	timing_constraints = (float **) alloc_matrix(0, num_clocks-1, 0, num_clocks-1, sizeof(float));
	
	/* Parse the file line-by-line */
	while (my_fgets(buf, BUFSIZE, sdc) != NULL) { 
		get_sdc_tok(buf, num_lines);
		num_lines++;
	}

	fclose(sdc);
}

void get_sdc_tok(char * buf, int num_lines) {
	/* Figures out which, if any token is at the start of this line and *
	 * takes the appropriate action.                                    */

#define SDC_TOKENS " \t\n{}" 
	/*We're using so little of the SDC syntax that we can ignore braces*/

	char * ptr;
	float clock_period_temp;
	int i,j;
	/*char clockname[BUFSIZE];*/

	ptr = my_strtok(buf, TOKENS, sdc, buf);
	if (ptr == NULL)
		return;

	if (strcmp(ptr, "create_clock") == 0) {
		/* recall: we need the syntax to be create_clock -period <float> <name or *> */

		/* make sure clock has -period specified */
		ptr = my_strtok(NULL, TOKENS, sdc, buf);
		if(strcmp(ptr, "-period") != 0) {
			fprintf(stderr, "create_clock requires a '-period' on line %d of SDC file", num_lines);
			exit(1);
		}
		ptr = my_strtok(NULL, TOKENS, sdc, buf);

		/* check if the token following -period is actually a number*/
		if(!check_if_number(ptr)) {
			fprintf(stderr, "'-period' on line %d of SDC file is not followed by a number", num_lines);
			exit(1);
		}
		
		/* for now, store the clock period in a temp variable */
		clock_period_temp = (float) strtod(ptr, NULL);

		ptr = my_strtok(NULL, TOKENS, sdc, buf);
		if(!ptr) {
			fprintf(stderr, "missing clock nets at end of line %d", num_lines);
			exit(1);
		}
		
		/* If the next token is a wildcard, we're done!  Set up the timing constraint matrix so that every diagonal entry *
		 *  has T_crit equal to the temporary clock period, and every off-diagonal entry is not analysed, then return     */
		if(strcmp(ptr, "*") == 0) {
			for(i=0;i<num_netlist_clocks;i++) {
				for(j=0;j<num_netlist_clocks;j++) {
					if(i==j) timing_constraints[i][j] = clock_period_temp;
					else timing_constraints[i][j] = DO_NOT_ANALYSE;
				}
			}
			return;
		}		
	}
	else {
		fprintf(stderr, "Incorrect or unsupported syntax in line %d of SDC file", num_lines);
		exit(1);
	}
}

boolean check_if_number(char * ptr) {
/* Checks if the character array ptr represents a number.  *
 * To return TRUE, all characters must be digits, although *
 * there can also be no more than one decimal point.       */
	int i, num_decimal_points = 0, len=strlen(ptr);
		for (i=0;i<len;i++) {
		if((ptr[i]<'0'||ptr[i]>'9')) {
			if(ptr[i] != '.') 
				return FALSE;
			num_decimal_points++;
			if(num_decimal_points > 1) 
					return FALSE;
			}
	}
	return TRUE;
}