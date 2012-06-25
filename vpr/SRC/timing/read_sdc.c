#include <string.h>
#include <stdio.h>
#include <limits.h>
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
int * exclusive_clock_groups = NULL; /* Temporary structure used to interpret   *
									  * the command set_clock_groups -exclusive */

/***************** Subroutines local to this module *************************/

static boolean get_sdc_tok(char * buf, int num_lines);
static boolean check_if_number(char * ptr);
static int find_clock(char * ptr, int num_lines);
static float calculate_constraint(t_sdc_clock source_domain, t_sdc_clock sink_domain);

/********************* Subroutine definitions *******************************/

void read_sdc(char * sdc_file) {
/*This function reads the constraints from the SDC file *
* specified on the command line into float ** timing_   *
* constraints.  If no file is specified, it leaves      *
* timing_constraints pointing to NULL.                  */

	char buf[BUFSIZE];
	int num_lines = 1; /* Line counter for SDC file, used to report errors */
	int i, j;
		
	/* Allocate matrix of timing constraints [0..num_netlist_clocks-1][0..num_netlist_clocks-1] and initialize to 0 */
	timing_constraints = (float **) alloc_matrix(0, num_netlist_clocks-1, 0, num_netlist_clocks-1, sizeof(float));
	for(i=0;i<num_netlist_clocks;i++) {
		for(j=0;j<num_netlist_clocks;j++) {
				timing_constraints[i][j] = 0.;
		}
	}

	/* If no SDC file is included or specified, use default behaviour of cutting paths between domains and optimizing each clock separately */
	if (!sdc_file || (sdc = fopen(sdc_file, "r")) == NULL) {
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
		if(get_sdc_tok(buf, num_lines)) return; /* return only if get_sdc_tok finds a special wildcard case (see below) */
		num_lines++;
	}
	
	fclose(sdc);

	/* Based on the information from sdc_clocks, calculate constraints for all paths not excluded by set_false_path or set_clock_groups -exclusive */
		for(i=0;i<num_netlist_clocks;i++) {
			for(j=0;j<num_netlist_clocks;j++) {
				if(timing_constraints[i][j] != DO_NOT_ANALYSE)
					timing_constraints[i][j] = calculate_constraint(sdc_clocks[i], sdc_clocks[j]);
			}
		}

	/* Since all the information we need is stored in timing_constraints, free other data structures used in this routine */
	free(sdc_clocks);
	return;
}

static boolean get_sdc_tok(char * buf, int num_lines) {
	/* Figures out which, if any, token is at the start of this line and *
	 * takes the appropriate action.  Returns FALSE unless a special wildcard case is found, 
	 * in which case the calling function immediately returns. */

#define SDC_TOKENS " \t\n{}" 
	/* We're using so little of the SDC syntax that we can ignore braces */

	char * ptr;
	float temp_clock_period, temp_rising_edge, temp_falling_edge;
	int i,j, from, to, num_exclusive_groups = 0;

	/* my_strtok splits the string into tokens - little character arrays separated by the SDC_TOKENS defined above.          *
	 * Throughout this code, ptr refers to the tokens we fetch, one at a time.  The token changes at each call of my_strtok. */
	ptr = my_strtok(buf, SDC_TOKENS, sdc, buf);

	if (ptr == NULL) { /* blank line */
		return FALSE;
	}

	if(ptr[0] == '#') { /* line starts with a comment */
		return FALSE; 
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
		if(!check_if_number(ptr)) {
			fprintf(stderr, "token following '-period' is not a number on line %d of SDC file", num_lines);
			exit(1);
		}
		
		/* for now, store the clock period in a temp variable */
		temp_clock_period = (float) strtod(ptr, NULL);

		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(!ptr) {
			fprintf(stderr, "clock net(s) not specified at end of line %d of SDC file", num_lines);
			exit(1);
		}
		
		if(strcmp(ptr, "-waveform") == 0) {
			/* Get the first float, which is the rising edge, and the second, which is the falling edge */
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			if(!check_if_number(ptr)) {
				fprintf(stderr, "first token following '-waveform' is not a number on line %d of SDC file", num_lines);
				exit(1);
			}
			temp_rising_edge = (float) strtod(ptr, NULL);
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			if(!check_if_number(ptr)) {
				fprintf(stderr, "second token following '-waveform' is not a number on line %d of SDC file", num_lines);
				exit(1);
			}
			temp_falling_edge = (float) strtod(ptr, NULL);
			/* Check that the falling edge is one half period away from the rising edge, excluding rounding error */
			if(abs(temp_rising_edge - temp_falling_edge) - temp_clock_period/2.0 > EQUAL_DEF) {
				fprintf(stderr, "clock does not have 50%% duty cycle on line %d of SDC file", num_lines);
				exit(1);
			}
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf); /* We need this extra one to advance the ptr to the right spot */
		} else {
			/* The clock's rising edge is by default at 0, and the falling edge is at the half-period */
			temp_rising_edge = 0.;
			temp_falling_edge = temp_clock_period / 2.0;
		}

		/* If the next token is a wildcard, we're done!  Set up the timing constraint matrix so that every entry *
		 *  has T_crit equal to the specified clock period, then return TRUE so we can exit read_sdc() as well */
		if(strcmp(ptr, "*") == 0) {
			for(i=0;i<num_netlist_clocks;i++) {
				for(j=0;j<num_netlist_clocks;j++) {
					timing_constraints[i][j] = temp_clock_period;
				}
			}
			return TRUE;
		} else {
			/* Parse through to the end of the line.  All that should be left on this line are a bunch of 
			 * clock nets to be associated with this clock period.  An array sdc_clocks will
			 * store the period and offset of each clock at the same index which that clock has in clock_list.
			 * After everything has been parsed, we take the information from this array to calculate the actual timing constraints
			 * which these periods and offsets imply, and put them in the matrix timing_constraints. */

			for(;;) {
				if(ptr == NULL) { /* end of line */
					return FALSE; 
				}

				/* Store the clock's period and offset in the local array sdc_clocks.  If it doesn't exist yet, malloc it. */
				if(!sdc_clocks) {
					sdc_clocks = (t_sdc_clock *) my_malloc(num_netlist_clocks * sizeof(t_sdc_clock));
				}

				i = find_clock(ptr, num_lines);
				
				/* The value of i is currently the index of the clock in clock_list */ 
				sdc_clocks[i].period = temp_clock_period;
				sdc_clocks[i].offset = temp_rising_edge; 
				
				/* Advance to the next token (or the end of the line) */
				ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			}	
		}
	} else if (strcmp(ptr, "set_clock_groups") == 0) {
		
		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(strcmp(ptr, "-exclusive") != 0) {
			fprintf(stderr, "set_clock_groups must be followed by '-exclusive' on line %d of SDC file", num_lines);
			exit(1);
		}
		
		/* Parse through to the end of the line.  All that should be left on this line are a bunch of 
		 * -group commands, followed by lists of clocks in that group.  An array exclusive_clock_groups will
		 * store the group number of each clock at the index location of that clock in clock_list - i.e. if the clock
		 * at index 5 in clock_list appears after the second -group token, then exclusive_clock_groups[5] = 2. 
		 * Finally, we set timing_constraints to not analyse between clock domains with different group numbers.  */
		for(;;) {
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			if(ptr == NULL) { /* end of line */
				break; /* exit the infinite for loop - but don't return yet - we still have to populate timing_constraints!  */
			}
			if(strcmp(ptr, "-group") == 0) {
			/* add 1 to the group number we're assigning clocks to every time the token -group is hit */
				num_exclusive_groups++;
			} else { 
				/* Presumptively, we have a clock. Calloc the array exclusive_clock_groups if not done already, then *
				 * find the index of the clock named by ptr, and set exclusive_clock_groups[that clock's index] equal to the current group number */
				if(!exclusive_clock_groups) {
					exclusive_clock_groups = (int *) my_calloc(num_netlist_clocks, sizeof(int));
				}
				exclusive_clock_groups[find_clock(ptr, num_lines)] = num_exclusive_groups;
			}
		}

		/* Finally, set every element of timing_constraints for which the two indices have different group numbers to DO_NOT_ANALYSE */
		for(i=0;i<num_netlist_clocks;i++) {
			for(j=0;j<num_netlist_clocks;j++) {
				/* if either source or sink domain is part of group 0 (i.e. not part of an exclusive group), don't touch it */
				if(exclusive_clock_groups[i] != 0 && exclusive_clock_groups[j] != 0) {
					if(exclusive_clock_groups[i] != exclusive_clock_groups[j]) {
						timing_constraints[i][j] = DO_NOT_ANALYSE;
					}
				}
			}
		}
		free(exclusive_clock_groups);
		return FALSE;

	} else if (strcmp(ptr, "set_false_path") == 0) {

		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(strcmp(ptr, "-from") != 0) {
			fprintf(stderr, "set_false_path must be followed by '-from <clock_name>' on line %d of SDC file", num_lines);
			exit(1);
		}

		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		from = find_clock(ptr, num_lines);

		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		if(strcmp(ptr, "-to") != 0) {
			fprintf(stderr, "set_false_path must be followed by '-from <clock_name> -to <clock_name> on line %d of SDC file", num_lines);
			exit(1);
		}
		ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
		to = find_clock(ptr, num_lines);

		/* Set the path from 'from' to 'to' in timing_constraints to not be analysed */
		timing_constraints[from][to] = DO_NOT_ANALYSE;
		
		return FALSE;

	} else {
		fprintf(stderr, "Incorrect or unsupported syntax near start of line %d of SDC file", num_lines);
		exit(1);
	}
}

static boolean check_if_number(char * ptr) {
/* Checks if the character array ptr represents a valid floating-point number.  *
 * To return TRUE, all characters must be digits, although *
 * there can also be no more than one decimal point.       */
	int i, num_decimal_points = 0, len=strlen(ptr);
	for (i=0;i<len;i++) {
		if((ptr[i]<'0'||ptr[i]>'9')) {
			if(ptr[i] != '.') {
				return FALSE;
			}
			num_decimal_points++;
			if(num_decimal_points > 1) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

static int find_clock(char * ptr, int num_lines) {
/* Given a token ptr, find whether it's the name of a clock in the array clock_list.  If it's not, print an error and exit; *
 * if it is, return the clock's index in clock_list. */
	int index;
	for(index=0;index<num_netlist_clocks;index++) {
		if(strcmp(ptr, clock_list[index].name) == 0) {
			return index;
		}
	}
	fprintf(stderr, "clock name %s does not correspond to a net on line %d of SDC file", ptr, num_lines);
	exit(1);
}

static float calculate_constraint(t_sdc_clock source_domain, t_sdc_clock sink_domain) {
	/* Given information from the SDC file about the period and offset of two clocks, *
	* determine the implied setup-time constraint between them via edge counting.     */

	int source_period, sink_period, source_offset, sink_offset, lcm_period, num_source_edges, num_sink_edges, 
		* source_edges, * sink_edges, i, j, time, constraint_as_int;
	float constraint;

	/* If the source and sink domains are the same, the constraint is just the clock period. */
	if((source_domain.period == sink_domain.period) && (source_domain.offset == sink_domain.offset)) {
		constraint = source_domain.period; /* or, equivalently, sink_domain.period */
		return constraint;
	}
	
	/* If we get here, the two constraints are not the same, and so we have to use edge counting. *
	 * First, multiply periods and offsets by 1000 and round down to the nearest integer,		  *
	 * to avoid messy decimals. */

	source_period = (int) source_domain.period * 1000;
	sink_period = (int) sink_domain.period * 1000;
	source_offset = (int) source_domain.offset * 1000;	
	sink_offset = (int) sink_domain.offset * 1000;

	/* Second, find the LCM of the two periods.  *
	* This determines how long it takes before the pattern of the two clocks starts repeating. */
	for(lcm_period = 1; ; lcm_period++) {
		if(lcm_period % source_period == 0 && lcm_period % sink_period == 0) {
			break;
		}
	}

	/* Third, create an array of positive edges for each clock over one LCM clock period. */

	num_source_edges = lcm_period/source_period + 1; 
	num_sink_edges = lcm_period/sink_period + 1;

	source_edges = (int *) my_malloc((num_source_edges + 1) * sizeof(int));
	sink_edges = (int *) my_malloc((num_sink_edges + 1) * sizeof(int));
	
	for(i=0, time=source_offset; i<num_source_edges; i++) {
		source_edges[i] = time;
		time += source_period;
	}

	for(i=0, time=sink_offset; i<num_sink_edges; i++) {
		sink_edges[i] = time;
		time += sink_period;
	}

	/* Fourth, compare every edge in source_edges with every edge in sink_edges      *
	 * The lowest strictly positive difference between a sink edge and a source edge *
	 * gives us the set-up time constraint.											 */

	constraint_as_int = INT_MAX; /* constraint starts off at +ve infinity so that everything will be less than it */

	for(i=0; i<num_source_edges+1; i++) {
		for(j=0; j<num_sink_edges+1; j++) {
			if(sink_edges[j]>source_edges[i]) {
				constraint_as_int = min(constraint_as_int, sink_edges[j]-source_edges[i]);
			}
		}
	}

	/* Fifth, divide by 1000 again and turn the constraint back into a float, and clean up memory. */

	constraint = (float) constraint_as_int/1000;

	free(source_edges);
	free(sink_edges);

	return constraint;
}