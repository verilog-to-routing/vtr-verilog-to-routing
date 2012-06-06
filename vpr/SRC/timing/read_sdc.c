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

/****************** Variables local to this module **************************/

static FILE *sdc;
t_sdc_clock * sdc_clocks = NULL; /* List of clock periods and offsets, read *
								  * in directly from SDC file				*/

/***************** Subroutines local to this module *************************/

void get_sdc_tok(char * buffer, int num_lines);
void check_if_number(char * ptr, int num_lines);
void calculate_constraints(void);

/********************* Subroutine definitions *******************************/

void read_sdc(char * sdc_file, int num_clocks) {
/*This function reads the constraints from the SDC file *
* specified on the command line into float ** timing_   *
* constraints.  If no file is specified, it leaves      *
* timing_constraints pointing to NULL.                  */

	char buf[BUFSIZE];
	int num_lines = 1, i, j; /* Line counter for SDC file, used to report errors */
		
	/* Allocate matrix of timing constraints [0..num_clocks-1][0..num_clocks-1] */
	timing_constraints = (float **) alloc_matrix(0, num_clocks-1, 0, num_clocks-1, sizeof(float));

	/* If no SDC file is included or specified, use default behaviour of cutting paths between domains and optimizing each clock separately */
	if ((sdc = fopen(sdc_file, "r")) == NULL) {
		printf("\nSDC file %s not found.\n", sdc_file);
		printf("All clocks will be optimized to run as fast as possible.\n");
		printf("Paths between clock domains will be cut.\n\n");
		for(i=0;i<num_netlist_clocks;i++) {
			for(j=0;j<num_netlist_clocks;j++) {
				if(i==j) timing_constraints[i][j] = 0.;
				else timing_constraints[i][j] = DO_NOT_ANALYSE;
			}
		}
		return;
	}
	
	/* If an SDC file exists, parse the file line-by-line */
	while (my_fgets(buf, BUFSIZE, sdc) != NULL) { 
		get_sdc_tok(buf, num_lines);
		num_lines++;
	}
	
	fclose(sdc);

	/* Based on the information from sdc_clocks, calculate constraints for all paths not excluded with set_false_path or set_clock_groups -exclusive */
	calculate_constraints();
}

void get_sdc_tok(char * buf, int num_lines) {
	/* Figures out which, if any, token is at the start of this line and *
	 * takes the appropriate action.                                     */

#define SDC_TOKENS " \t\n{}" 
	/*We're using so little of the SDC syntax that we can ignore braces */

	char * ptr;
	float temp_clock_period, temp_rising_edge, temp_falling_edge;
	int i,j;
	boolean found;

	/* my_strtok splits the string into tokens - little character arrays separated by the SDC_TOKENS defined above.          *
	 * Throughout this code, ptr refers to the tokens we fetch, one at a time.  The token changes at each call of my_strtok. */
	ptr = my_strtok(buf, SDC_TOKENS, sdc, buf);

	if (ptr == NULL) { /* blank line */
		return;
	}

	if(ptr[0] == '#') { /* line starts with a comment */
		return; 
	}

	if (strcmp(ptr, "create_clock") == 0) {
		/* recall: we need the syntax to be create_clock -period <float> <name or *> */

		/* make sure clock has -period specified */
		/* Note: we call my_strtok with NULL as the first argument every time AFTER the first, since this picks up tokenizing where we left off */
		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);

		if(strcmp(ptr, "-period") != 0) {
			fprintf(stderr, "create_clock must be followed by '-period' on line %d of SDC file", num_lines);
			exit(1);
		}
		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);

		/* check if the token following -period is actually a number*/
		check_if_number(ptr, num_lines);
		
		/* for now, store the clock period in a temp variable */
		temp_clock_period = (float) strtod(ptr, NULL);

		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(!ptr) {
			fprintf(stderr, "missing clock nets at end of line %d", num_lines);
			exit(1);
		}
		
		if(strcmp(ptr, "-waveform") == 0) {
			/* Get the first float, which is the rising edge, and the second, which is the falling edge */
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			check_if_number(ptr, num_lines);
			temp_rising_edge = (float) strtod(ptr, NULL);
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			check_if_number(ptr, num_lines);
			temp_falling_edge = (float) strtod(ptr, NULL);
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf); /* We need this extra one to advance the ptr to the right spot */
		} else {
			/* The clock's rising edge is by default at 0, and the falling edge is at the half-period */
			temp_rising_edge = 0.;
			temp_falling_edge = temp_clock_period / 2;
		}

		/* If the next token is a wildcard, we're done!  Set up the timing constraint matrix so that every entry *
		 *  has T_crit equal to the specified clock period, then return     */
		if(strcmp(ptr, "*") == 0) {
			for(i=0;i<num_netlist_clocks;i++) {
				for(j=0;j<num_netlist_clocks;j++) {
					timing_constraints[i][j] = temp_clock_period;
				}
			}
			return;
		} else {
			/* Parse through to the end of the line.  All that's left are the clock nets to be associated with this clock period */
			for(;;) {
				ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
				if(ptr == NULL) { /* blank line */
					return; 
				}

				/* ptr contains the name of a single clock net.  Search for its index in the array clock_list */
				found = FALSE;
				for(i=0;i<num_netlist_clocks;i++) {
					if(strcmp(ptr, clock_list[i].name) == 0) {
						found = TRUE;
						break; /* exits the num_netlist_clocks for loop */
					}
				}
				if(found == FALSE) {
					fprintf(stderr, "clock name %s does not correspond to a net on line %d of SDC file", ptr, num_lines);
					exit(1);
				}
				
				/* Store the clock's period and offset in the local array sdc_clocks.  If it doesn't exist yet, malloc it. */
				if(!sdc_clocks) {
					sdc_clocks = (t_sdc_clock *) my_malloc(num_netlist_clocks * sizeof(t_sdc_clock));
				}
				
				/* The value of i is currently the index of the clock in num_netlist_clocks, which is how we'll be thinking of the clock from now on. */ 
				sdc_clocks[i].period = temp_clock_period;
				sdc_clocks[i].offset = temp_rising_edge; /* Note: temp_falling_edge is irrelevant unless you care about duty cycles */
			}	
		}
	} else if (strcmp(ptr, "set_clock_groups") == 0) {
		
		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(strcmp(ptr, "-exclusive") != 0) {
			fprintf(stderr, "set_clock_groups must be followed by '-exclusive' on line %d of SDC file", num_lines);
			exit(1);
		}
		
		/* now all that should be left on this line are a bunch of -group commands, followed by lists of clocks in that group */
		/*for(;;) {
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			if(strcmp(ptr, "-group"
		}*/
	} else if (strcmp(ptr, "set_false_paths") == 0) {

	} else {
		fprintf(stderr, "Incorrect or unsupported syntax near start of line %d of SDC file", num_lines);
		exit(1);
	}
}

void check_if_number(char * ptr, int num_lines) {
/* Checks if the character array ptr represents a number.					    *
 * To return without throwing an error, all characters must be digits, although *
 * there can also be no more than one decimal point.                            */
	int i, num_decimal_points = 0, len=strlen(ptr);
	for (i=0;i<len;i++) {
		if((ptr[i]<'0'||ptr[i]>'9')) {
			if(ptr[i] != '.') {
				fprintf(stderr, "'-period' on line %d of SDC file is not followed by a number", num_lines);
				exit(1);
			}
			num_decimal_points++;
			if(num_decimal_points > 1) {
				fprintf(stderr, "'-period' on line %d of SDC file is not followed by a number", num_lines);
				exit(1);
			}
		}
	}
	return;
}

void calculate_constraints(void) {
	return;
}