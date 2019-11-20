#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "assert.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "read_sdc.h"
#include "read_blif.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "ReadOptions.h"
#include "slre.h"

/***************************** Summary **********************************/

/* Author: Michael Wainberg

Looks for an SDC (Synopsys Design Constraints) file called <circuitname>.sdc
(unless overridden with --sdc_file <filename.sdc> on the command-line, in which 
case it looks for that filename), and parses the timing constraints in that file. 
If it doesn't find a file with that name, it uses default timing constraints 
(which differ depending on whether the circuit has 0, 1, or multiple clocks).

The primary routine, read_sdc, populates a container structure, g_sdc.
One of the two key output data structures within is g_sdc->constrained_clocks, which 
associates each clock given a timing constraint with a name, fanout and whether 
it is a netlist or virtual (external) clock.  From this point on, the only clocks 
we care about are the ones in this array. During timing analysis and data output, 
clocks are accessed using the indexing of this array.

The other key data structure is the "constraint matrix" g_sdc->domain_constraint, which 
has a timing constraint for each pair (source and sink) of clock domains. These 
generally come from finding the smallest difference between the posedges of the 
two clocks over the LCM clock period ("edge counting" - see calculate_constraint()).

Alternatively, entries in g_sdc->domain_constraint can come from a special-case, "override 
constraint" (so named because it overrides the default behaviour of edge counting). 
Override constraints can cut paths (set_clock_groups, set_false_path commands), 
create a multicycle (set_multicycle_path) or even override a constraint with a user-
specified one (set_max_delay). These entries are stored temporarily in g_sdc->cc_constraints 
(cc = clock to clock), which is freed once the timing_constraints echo file is 
created during process_constraints(). 

Flip-flop-level override constraints also exist and are stored in g_sdc->cf_constraints, 
g_sdc->fc_constraints and g_sdc->ff_constraints (depending on whether the source, sink or neither 
of the two is a clock domain).Unlike g_sdc->cc_constraints, they are placed on the timing 
graph during timing analysis instead of going into g_sdc->domain_constraint, and are not 
freed until the end of VPR's execution.

I/O constraints from set_input_delay and set_output_delay are stored in constrained_
inputs and g_sdc->constrained_outputs. These associate each I/O in the netlist given a 
constraint with the clock (often virtual, but could be in the netlist) it was 
constrained on, and the delay through the I/O in that constraint.

The remaining data structures are temporary and local to this file: netlist_clocks, 
netlist_inputs and netlist_outputs, which are used to match names of clocks and I/Os
in the SDC file to those in the netlist; sdc_clocks, which stores info on clock periods 
and offsets from create_clock commands and is the raw info used in edge counting; and 
exclusive_groups, used when parsing set_clock_groups commands into g_sdc->cc_constraints. */

/*********************** Externally-accessible variables **************************/

t_timing_constraints * g_sdc = NULL;

/****************** Types local to this module **************************/

typedef struct s_sdc_clock {
	char * name;
	float period;
	float rising_edge;
	float falling_edge;
} t_sdc_clock;
/* Stores the name, period and offset of each constrained clock. */

typedef struct s_sdc_exclusive_group {
	char ** clock_names;
	int num_clock_names;
} t_sdc_exclusive_group;
/* Used to temporarily separate clock names into exclusive groups when parsing the 
command set_clock_groups -exclusive. */

/****************** Variables local to this module **************************/

static FILE *sdc;
t_sdc_clock * sdc_clocks = NULL; /* List of clock periods and offsets from create_clock commands */

int num_netlist_clocks = 0; /* number of clocks in netlist */
char ** netlist_clocks; /* [0..num_netlist_clocks - 1] array of names of clocks in netlist */

int num_netlist_ios = 0; /* number of clocks in netlist */
char ** netlist_ios; /* [0..num_netlist_clocks - 1] array of names of ios in netlist */

/***************** Subroutines local to this module *************************/

static void alloc_and_load_netlist_clocks_and_ios(void);
static void use_default_timing_constraints(void);
static void count_netlist_clocks_as_constrained_clocks(void);
static boolean get_sdc_tok(char * buf);
static boolean is_number(char * ptr);
static int find_constrained_clock(char * ptr);
static float calculate_constraint(t_sdc_clock source_domain, t_sdc_clock sink_domain);
static void add_override_constraint(char ** from_list, int num_from, char ** to_list, int num_to, 
	float constraint, int num_multicycles, boolean domain_level_from, boolean domain_level_to,
	boolean make_copies);
static int find_cc_constraint(char * source_clock_domain, char * sink_clock_domain);
static boolean regex_match (char *string, char *pattern);
static void count_netlist_ios_as_constrained_ios(char * clock_name, float io_delay);
static void free_io_constraint(t_io *& io_array, int num_ios);
static void free_clock_constraint(t_clock *& clock_array, int num_clocks);

/********************* Subroutine definitions *******************************/

void read_sdc(t_timing_inf timing_inf) {

	char buf[BUFSIZE];
	int source_clock_domain, sink_clock_domain, iinput, ioutput, icc, isource, isink;
	boolean found; 
	
	/* Make sure we haven't called this subroutine before. */
	assert(!g_sdc);

	/* Allocate container structure for SDC constraints. */
	g_sdc = (t_timing_constraints *) my_calloc(1, sizeof(t_timing_constraints));
	
	/* Reset file line number. */
	file_line_number = 0;

	/* If no SDC file is included or specified, or timing analysis is off,
	use default behaviour of cutting paths between domains and optimizing each clock separately */

	if (!timing_inf.timing_analysis_enabled) {
		vpr_printf(TIO_MESSAGE_INFO, "\n");
		vpr_printf(TIO_MESSAGE_INFO, "Timing analysis off; using default timing constraints.\n");
		use_default_timing_constraints();
		return;
	}
	
	if ((sdc = fopen(timing_inf.SDCFile, "r")) == NULL) {
		vpr_printf(TIO_MESSAGE_INFO, "\n");
		vpr_printf(TIO_MESSAGE_INFO, "SDC file '%s' blank or not found.\n", timing_inf.SDCFile);
		use_default_timing_constraints();
		return;
	}
	
	/* Now we have an SDC file. */

	/* Count how many clocks and I/Os are in the netlist. 
	Store the names of each clock and each I/O in netlist_clocks and netlist_ios. 
	The only purpose of these two lists is to compare clock names in the SDC file against them.
	As a result, they will be freed after the SDC file is parsed. */
	alloc_and_load_netlist_clocks_and_ios();

	/* Parse the file line-by-line. */
	found = FALSE;
	while (my_fgets(buf, BUFSIZE, sdc) != NULL) { 
		if (get_sdc_tok(buf)) {
			found = TRUE;
		}
	}
	if (!found) { /* blank file or only comments found */
		vpr_printf(TIO_MESSAGE_INFO, "\n");
		vpr_printf(TIO_MESSAGE_INFO, "SDC file '%s' blank or not found.\n", timing_inf.SDCFile);
		use_default_timing_constraints();
		free(netlist_clocks);
		free(netlist_ios);
		return;
	}
	
	fclose(sdc);

	/* Make sure that all virtual clocks referenced in g_sdc->constrained_inputs and g_sdc->constrained_outputs have been constrained. */
	for (iinput = 0; iinput < g_sdc->num_constrained_inputs; iinput++) {
		if ((find_constrained_clock(g_sdc->constrained_inputs[iinput].clock_name)) == -1) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Input %s is associated with an unconstrained clock %s.\n", 
					g_sdc->constrained_inputs[iinput].file_line_number,
					g_sdc->constrained_inputs[iinput].name, 
					g_sdc->constrained_inputs[iinput].clock_name);
			exit(1);
		}
	}

	for (ioutput = 0; ioutput < g_sdc->num_constrained_outputs; ioutput++) {
		if ((find_constrained_clock(g_sdc->constrained_outputs[ioutput].clock_name)) == -1) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Output %s is associated with an unconstrained clock %s.\n", 
					g_sdc->constrained_inputs[iinput].file_line_number,
					g_sdc->constrained_outputs[ioutput].name, 
					g_sdc->constrained_outputs[ioutput].clock_name);
			exit(1);
		}
	}

	/* Make sure that all clocks referenced in g_sdc->cc_constraints have been constrained. */
	for (icc = 0; icc < g_sdc->num_cc_constraints; icc++) {
		for (isource = 0; isource < g_sdc->cc_constraints[icc].num_source; isource++) {
			if ((find_constrained_clock(g_sdc->cc_constraints[icc].source_list[isource])) == -1) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Token %s is not a constrained clock.\n", 
						g_sdc->cc_constraints[icc].file_line_number,
						g_sdc->cc_constraints[icc].source_list[isource]);
				exit(1);
			}
		}
		for (isink = 0; isink < g_sdc->cc_constraints[icc].num_sink; isink++) {
			if ((find_constrained_clock(g_sdc->cc_constraints[icc].sink_list[isink])) == -1) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Token %s is not a constrained clock.\n",
						g_sdc->cc_constraints[icc].file_line_number, 
						g_sdc->cc_constraints[icc].sink_list[isink]);
				exit(1);
			}
		}
	}

	/* Allocate matrix of timing constraints [0..g_sdc->num_constrained_clocks-1][0..g_sdc->num_constrained_clocks-1] and initialize to 0 */
	g_sdc->domain_constraint = (float **) alloc_matrix(0, g_sdc->num_constrained_clocks-1, 0, g_sdc->num_constrained_clocks-1, sizeof(float));
	
	/* Based on the information from sdc_clocks, calculate constraints for all paths except ones with an override constraint. */
	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			if ((icc = find_cc_constraint(g_sdc->constrained_clocks[source_clock_domain].name, g_sdc->constrained_clocks[sink_clock_domain].name)) != -1) {
				if (g_sdc->cc_constraints[icc].num_multicycles == 0) {
					/* There's a special constraint from set_false_path, set_clock_groups 
					-exclusive or set_max_delay which overrides the default constraint. */
					g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = g_sdc->cc_constraints[icc].constraint;
				} else {
					/* There's a special constraint from set_multicycle_path which overrides the default constraint. 
					This constraint = default constraint (obtained via edge counting) + (num_multicycles - 1) * period of sink clock domain. */
					g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = 
						calculate_constraint(sdc_clocks[source_clock_domain], sdc_clocks[sink_clock_domain]) 
						+ (g_sdc->cc_constraints[icc].num_multicycles - 1) * sdc_clocks[sink_clock_domain].period;
				}
			} else {
				/* There's no special override constraint. */
				/* Calculate the constraint between clock domains by finding the smallest positive 
				difference between a posedge in the source domain and one in the sink domain. */
				g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = 
					calculate_constraint(sdc_clocks[source_clock_domain], sdc_clocks[sink_clock_domain]);
			}
		}
	}

	vpr_printf(TIO_MESSAGE_INFO, "\n");
	vpr_printf(TIO_MESSAGE_INFO, "SDC file '%s' parsed successfully.\n",
				 timing_inf.SDCFile ); 
	vpr_printf(TIO_MESSAGE_INFO, "%d clocks (including virtual clocks), %d inputs and %d outputs were constrained.\n", 
				 g_sdc->num_constrained_clocks, g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
	vpr_printf(TIO_MESSAGE_INFO, "\n");
	
	/* Since all the information we need is stored in g_sdc->domain_constraint, g_sdc->constrained_clocks, 
	and constrained_ios, free other data structures used in this routine */
	free(sdc_clocks);
	free(netlist_clocks);
	free(netlist_ios);
	return;
}

static void use_default_timing_constraints(void) {

	int source_clock_domain, sink_clock_domain;
	
	/* Find all netlist clocks and add them as constrained clocks. */
	count_netlist_clocks_as_constrained_clocks();

	/* We'll use separate defaults for multi-clock and single-clock/combinational circuits. */

	if (g_sdc->num_constrained_clocks <= 1) {
		/* Create one constrained clock with period 0... */
		g_sdc->domain_constraint = (float **) alloc_matrix(0, 0, 0, 0, sizeof(float));
		g_sdc->domain_constraint[0][0] = 0.;
				
		if (g_sdc->num_constrained_clocks == 0) {
			/* We need to create a virtual clock to constrain I/Os on. */
			g_sdc->num_constrained_clocks = 1;
			g_sdc->constrained_clocks = (t_clock *) my_malloc(sizeof(t_clock));
			g_sdc->constrained_clocks[0].name = my_strdup("virtual_io_clock");
			g_sdc->constrained_clocks[0].is_netlist_clock = FALSE;

			vpr_printf(TIO_MESSAGE_INFO, "\n");
			vpr_printf(TIO_MESSAGE_INFO, "Defaulting to: constrain all %d inputs and %d outputs on a virtual external clock.\n", 
					g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
			vpr_printf(TIO_MESSAGE_INFO, "Optimize this virtual clock to run as fast as possible.\n");
		} else {
			vpr_printf(TIO_MESSAGE_INFO, "\n");
			vpr_printf(TIO_MESSAGE_INFO, "Defaulting to: constrain all %d inputs and %d outputs on the netlist clock.\n", 
					g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
			vpr_printf(TIO_MESSAGE_INFO, "Optimize this clock to run as fast as possible.\n");
		}
		
		/* Constrain all I/Os on the single constrained clock (whether real or virtual), with I/O delay 0. */
		count_netlist_ios_as_constrained_ios(g_sdc->constrained_clocks[0].name, 0.);

	} else { /* Multiclock circuit */

		/* Constrain all I/Os on a separate virtual clock. Cut paths between all netlist
		 clocks, but analyse all paths between the virtual I/O clock and netlist clocks
		 and optimize all clocks to go as fast as possible. */

		g_sdc->constrained_clocks = (t_clock *) my_realloc (g_sdc->constrained_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_clock));
		g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = my_strdup("virtual_io_clock");
		g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = FALSE;
		count_netlist_ios_as_constrained_ios(g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name, 0.);

		/* Allocate matrix of timing constraints [0..g_sdc->num_constrained_clocks-1][0..g_sdc->num_constrained_clocks-1] */
		g_sdc->domain_constraint = (float **) alloc_matrix(0, g_sdc->num_constrained_clocks-1, 0, g_sdc->num_constrained_clocks-1, sizeof(float));

		for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
			for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain || source_clock_domain == g_sdc->num_constrained_clocks - 1 
					|| sink_clock_domain == g_sdc->num_constrained_clocks - 1) {
					g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = 0.;
				} else {
					g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = DO_NOT_ANALYSE;
				}
			}
		}
		
		vpr_printf(TIO_MESSAGE_INFO, "\n");
		vpr_printf(TIO_MESSAGE_INFO, "Defaulting to: constrain all %d inputs and %d outputs on a virtual external clock;\n",
				g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
		vpr_printf(TIO_MESSAGE_INFO, "\tcut paths between netlist clock domains; and\n");
		vpr_printf(TIO_MESSAGE_INFO, "\toptimize all clocks to run as fast as possible.\n");
	}
}

static void alloc_and_load_netlist_clocks_and_ios(void) {

	/* Count how many clocks and I/Os are in the netlist. 
	Store the names of each clock and each I/O in netlist_clocks and netlist_ios. */

	int iblock, i, clock_net;
	char * name;
	boolean found;

	for (iblock = 0; iblock < num_logical_blocks; iblock++) {
		if (logical_block[iblock].clock_net != OPEN) {
			clock_net = logical_block[iblock].clock_net;
			assert(clock_net != OPEN);
			name = logical_block[clock_net].name;
			/* Now that we've found a clock, let's see if we've counted it already */
			found = FALSE;
			for (i = 0; !found && i < num_netlist_clocks; i++) {
				if (strcmp(netlist_clocks[i], name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the clock is new and so we dynamically grow the array netlist_clocks by one. */
				netlist_clocks = (char **) my_realloc (netlist_clocks, ++num_netlist_clocks * sizeof(char *));
				netlist_clocks[num_netlist_clocks - 1] = name;
			}
		} else if (logical_block[iblock].type == VPACK_INPAD || logical_block[iblock].type == VPACK_OUTPAD) {
			name = logical_block[iblock].name;
			/* Now that we've found an I/O, let's see if we've counted it already */
			found = FALSE;
			for (i = 0; !found && i < num_netlist_ios; i++) {
				if (strcmp(netlist_ios[i], name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the I/O is new and so we dynamically grow the array netlist_ios by one. */
				netlist_ios = (char **) my_realloc (netlist_ios, ++num_netlist_ios * sizeof(char *));
				netlist_ios[num_netlist_ios - 1] = logical_block[iblock].type == VPACK_OUTPAD ? name + 4 : name; 
				/* the + 4 removes the prefix "out:" automatically prepended to outputs */
			}
		}
	}
}

static void count_netlist_clocks_as_constrained_clocks(void) {
	/* Counts how many clocks are in the netlist, and adds them to the array g_sdc->constrained_clocks. */

	int iblock, i, clock_net;
	char * name;
	boolean found;

	g_sdc->num_constrained_clocks = 0;
	
	for (iblock = 0; iblock < num_logical_blocks; iblock++) {
		if (logical_block[iblock].clock_net != OPEN) {
			clock_net = logical_block[iblock].clock_net;
			assert(clock_net != OPEN);
			name = logical_block[clock_net].name;
			/* Now that we've found a clock, let's see if we've counted it already */
			found = FALSE;
			for (i = 0; !found && i < g_sdc->num_constrained_clocks; i++) {
				if (strcmp(g_sdc->constrained_clocks[i].name, name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the clock is new and so we dynamically grow the array g_sdc->constrained_clocks by one. */
				g_sdc->constrained_clocks = (t_clock *) my_realloc (g_sdc->constrained_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_clock));
				g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = my_strdup(name);
				g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = TRUE;
				/* Fanout will be filled out once the timing graph has been constructed. */
			}
		}
	}
}

static void count_netlist_ios_as_constrained_ios(char * clock_name, float io_delay) {
	/* Count how many I/Os are in the netlist, adds them to the arrays g_sdc->constrained_inputs/
	g_sdc->constrained_outputs with an I/O delay of 0 and constrains them to clock clock_name. */

	int iblock, iinput, ioutput; 
	char * name;
	boolean found;

	for (iblock = 0; iblock < num_logical_blocks; iblock++) {
		if (logical_block[iblock].type == VPACK_INPAD) {
			name = logical_block[iblock].name;
			/* Now that we've found an I/O, let's see if we've counted it already */
			found = FALSE;
			for (iinput = 0; !found && iinput < g_sdc->num_constrained_inputs; iinput++) {
				if (strcmp(g_sdc->constrained_inputs[iinput].name, name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the input is new and so we add it to g_sdc->constrained_inputs. */
				g_sdc->constrained_inputs = (t_io *) my_realloc (g_sdc->constrained_inputs, ++g_sdc->num_constrained_inputs * sizeof(t_io));
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].name = my_strdup(name); 
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].clock_name = my_strdup(clock_name);
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].delay = 0.;
			}
		} else if (logical_block[iblock].type == VPACK_OUTPAD) {
			name = logical_block[iblock].name;
			/* Now that we've found an I/O, let's see if we've counted it already */
			found = FALSE;
			for (ioutput = 0; !found && ioutput < g_sdc->num_constrained_outputs; ioutput++) {
				if (strcmp(g_sdc->constrained_outputs[ioutput].name, name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the output is new and so we add it to g_sdc->constrained_outputs. */
				g_sdc->constrained_outputs = (t_io *) my_realloc (g_sdc->constrained_outputs, ++g_sdc->num_constrained_outputs * sizeof(t_io));
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].name = my_strdup(name + 4); 
				/* the + 4 removes the prefix "out:" automatically prepended to outputs */
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].clock_name = my_strdup(clock_name);
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].delay = 0.;
			}
		}
	}
}

static boolean get_sdc_tok(char * buf) {
/* Figures out which tokens are on this line and takes the appropriate actions. 
   Returns true if anything non-commented is found on this line. */

#define SDC_TOKENS " \t\n{}[]" /* We can ignore braces. */
	
	char * ptr, ** from_list = NULL, ** to_list = NULL, * clock_name;
	float clock_period, rising_edge, falling_edge, max_delay;
	int iclock, iio, num_exclusive_groups = 0,  
		num_from = 0, num_to = 0, num_multicycles, i, j;
	t_sdc_exclusive_group * exclusive_groups = NULL;
	boolean found, domain_level_from = FALSE, domain_level_to = FALSE;

	/* my_strtok splits the string into tokens - little character arrays separated by the SDC_TOKENS 
	defined above. Throughout this code, ptr refers to the tokens we fetch, one at a time. The token 
	changes at each call of my_strtok. We call my_strtok with NULL as the first argument every time 
	AFTER the first, since this picks up tokenizing where we left off. We always wrap each call to 
	my_strtok with a check that ptr is non-null to avoid an exception from passing NULL into strcmp. */


	if ((ptr = my_strtok(buf, SDC_TOKENS, sdc, buf)) == NULL) {/* blank line */
		return FALSE;
	}

	if (strcmp(ptr, "create_clock") == 0) {
		/* Syntax: create_clock -period <float> [-waveform {rising_edge falling_edge}] <netlist clock list or regexes> 
				or create_clock -period <float> [-waveform {rising_edge falling_edge}] -name <virtual clock name>*/

		/* make sure clock has -period specified */
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-period") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Create_clock must be directly followed by '-period'.\n", 
					file_line_number);
			exit(1);
		}
				
		/* Check if the token following -period is actually a number. */
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] A number must follow '-period'.\n", 
					file_line_number);
			exit(1);
		}
		clock_period = (float) strtod(ptr, NULL);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Clock(s) not specified.\n",
					file_line_number);
			exit(1);
		}
		if (strcmp(ptr, "-waveform") == 0) {
			
			/* Get the first float, which is the rising edge, and the second, which is the falling edge. */
			
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] First token following '-waveform' should be rising edge, but is not a number.\n",
						file_line_number);
			}
			rising_edge = (float) strtod(ptr, NULL);

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Second token following '-waveform' should be falling edge, but is not a number.\n",
						file_line_number);
				exit(1);
			}
			falling_edge = (float) strtod(ptr, NULL);

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Clock(s) not specified.\n", 
						file_line_number);
				exit(1);
			} /* We need this extra call to my_strtok to advance the ptr to the right spot. */

		} else {
			/* The clock's rising edge is by default at 0, and the falling edge is at the half-period. */
			rising_edge = 0.;
			falling_edge = clock_period / 2.0;
		}

		if (strcmp(ptr, "-name") == 0) {
			/* For external virtual clocks only (used with I/O constraints).  
			Only one virtual clock can be specified per line, 
			so make sure there's only one token left on this line. */

			/* Get the virtual clock name */
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Virtual clock name not specified.\n", 
						file_line_number);
				exit(1);
			} /* We need this extra call to my_strtok to advance the ptr to the right spot. */

			/* We've found a new clock! */

			/* Store the clock's name, period and edges in the local array sdc_clocks. */
			sdc_clocks = (t_sdc_clock *) my_realloc(sdc_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_sdc_clock));
			sdc_clocks[g_sdc->num_constrained_clocks - 1].name = ptr;
			sdc_clocks[g_sdc->num_constrained_clocks - 1].period = clock_period;
			sdc_clocks[g_sdc->num_constrained_clocks - 1].rising_edge = rising_edge; 
			sdc_clocks[g_sdc->num_constrained_clocks - 1].falling_edge = falling_edge; 

			/* Also store the clock's name, and the fact that it is not a netlist clock, in g_sdc->constrained_clocks. */
			g_sdc->constrained_clocks = (t_clock *) my_realloc (g_sdc->constrained_clocks, g_sdc->num_constrained_clocks * sizeof(t_clock));
			g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = my_strdup(ptr);
			g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = FALSE;
			/* Fanout will be filled out once the timing graph has been constructed. */

			/* The next token should be NULL.  If so, return; if not, print an error message and exit. */
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] More than one virtual clock name is specified after '-name'.\n", 
						file_line_number);
				exit(1);
			}

		} else {
			/* Parse through to the end of the line.  All that should be left on this line are one or more
			 * regular expressions denoting netlist clocks to be associated with this clock period.  An array sdc_clocks will
			 * store the period and offset of each clock at the same index which that clock has in netlist_clocks.  Later,
			 * after everything has been parsed, we take the information from this array to calculate the actual timing constraints
			 * which these periods and offsets imply, and put them in the matrix g_sdc->domain_constraint. */

			do {
				/* See if the regular expression stored in ptr is legal and matches at least one clock net. 
				If it is not legal, it will fail during regex_match.  We check for a match using boolean found. */
				found = FALSE;
				for (iclock = 0; iclock < num_netlist_clocks; iclock++) {
					if (regex_match(netlist_clocks[iclock], ptr)) {
						/* We've found a new clock!  (Note that we can't store ptr as the clock's 
						name since it could be a regex, unlike the virtual clock case).*/
						found = TRUE;

						/* Store the clock's name, period and edges in the local array sdc_clocks. */
						sdc_clocks = (t_sdc_clock *) my_realloc(sdc_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_sdc_clock));
						sdc_clocks[g_sdc->num_constrained_clocks - 1].name = netlist_clocks[iclock];
						sdc_clocks[g_sdc->num_constrained_clocks - 1].period = clock_period;
						sdc_clocks[g_sdc->num_constrained_clocks - 1].rising_edge = rising_edge; 
						sdc_clocks[g_sdc->num_constrained_clocks - 1].falling_edge = falling_edge;

						/* Also store the clock's name, and the fact that it is a netlist clock, in g_sdc->constrained_clocks. */
						g_sdc->constrained_clocks = (t_clock *) my_realloc (g_sdc->constrained_clocks, g_sdc->num_constrained_clocks * sizeof(t_clock));
						g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = my_strdup(netlist_clocks[iclock]);
						g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = TRUE;
						/* Fanout will be filled out once the timing graph has been constructed. */
					}
				}

				if (!found) {
					vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Clock name or regular expression does not correspond to any nets.\n", 
							file_line_number);
					vpr_printf(TIO_MESSAGE_ERROR, "If you'd like to create a virtual clock, use the '-name' keyword.\n");
					exit(1);
				}
			} while ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL); /* Advance to the next token (or the end of the line). */
		}
	
		/* Warn if the clock has non-50% duty cycle. */
		if (fabs(rising_edge - falling_edge) - clock_period/2.0 > EPSILON) {
			vpr_printf(TIO_MESSAGE_WARNING, "Clock %s does not have 50%% duty cycle.\n", 
					sdc_clocks[g_sdc->num_constrained_clocks - 1].name);
		}

		return TRUE; 

	} else if (strcmp(ptr, "set_clock_groups") == 0) {
		/* Syntax: set_clock_groups -exclusive -group {<clock list or regexes>} -group {<clock list or regexes>} [-group {<clock list or regexes>} ...] */

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-exclusive") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_clock_groups must be directly followed by '-exclusive'.\n", 
					file_line_number);
			exit(1);
		}
		
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-group") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_clock_groups '-exclusive' must be followed by lists of clock names or regular expressions each starting with the '-group' command.\n", 
					file_line_number);
			exit(1);
		}

		/* Parse through to the end of the line. All that should be left on this line are a bunch of 
		 -group commands, followed by groups of regexes. We need to ensure that paths are cut between 
		 every clock matching a regex in one group and every clock matching a regex in any other group. */

		do {
			ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf);
			
			/* Create a new entry in exclusive groups */
			exclusive_groups = (t_sdc_exclusive_group *) my_realloc(
				exclusive_groups, ++num_exclusive_groups * sizeof(t_sdc_exclusive_group));
			exclusive_groups[num_exclusive_groups - 1].clock_names = NULL;
			exclusive_groups[num_exclusive_groups - 1].num_clock_names = 0;
			do {
				/* Check the regex ptr against each netlist clock and add it to the clock_names list if it matches. */
				found = FALSE;
				for (iclock = 0; iclock < num_netlist_clocks; iclock++) {
					if (regex_match(netlist_clocks[iclock], ptr)) {
						found = TRUE;
						exclusive_groups[num_exclusive_groups - 1].clock_names = (char **) my_realloc(
							exclusive_groups[num_exclusive_groups - 1].clock_names, ++exclusive_groups[num_exclusive_groups - 1].num_clock_names * sizeof(char *));
						exclusive_groups[num_exclusive_groups - 1].clock_names
							[exclusive_groups[num_exclusive_groups - 1].num_clock_names - 1] = 
							my_strdup(netlist_clocks[iclock]);
					}
				}
				if (!found) {
					/* If no clocks matched, assume ptr is the name of a virtual clock and add it to the list.
					(If it's not a virtual clock, we'll catch it later when we check all override constraints.) */
					exclusive_groups[num_exclusive_groups - 1].clock_names = (char **) my_realloc(
						exclusive_groups[num_exclusive_groups - 1].clock_names, ++exclusive_groups[num_exclusive_groups - 1].num_clock_names * sizeof(char *));
					exclusive_groups[num_exclusive_groups - 1].clock_names
						[exclusive_groups[num_exclusive_groups - 1].num_clock_names - 1] = 
						my_strdup(ptr);
				}
			} while ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL && strcmp(ptr, "-group") != 0);
		} while (ptr);

	if (num_exclusive_groups < 2) {
		vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] At least two '-group' commands required.", 
				file_line_number); 
		exit(1);
	}

	/* Finally, create two DO_NOT_ANALYSE override constraints for each pair of entries
	to cut paths bidirectionally between pairs of clock lists in different groups. 
	Set make_copies to TRUE because we have to use the lists of names in multiple
	override constraints, and it's impossible to free them from multiple places at the
	end without a whole lot of trouble. */

		for (i = 0; i < num_exclusive_groups; i++) {
			for (j = 0; j < num_exclusive_groups; j++) {
				if (i != j) {
					add_override_constraint(exclusive_groups[i].clock_names, exclusive_groups[i].num_clock_names, 
						exclusive_groups[j].clock_names, exclusive_groups[j].num_clock_names, DO_NOT_ANALYSE, 0, TRUE, TRUE, TRUE);
				}
			}
		}

		/* Now that we've copied all the clock name lists
		(2 * num_exlusive_groups - 1) times, free the original lists. */
		for (i = 0; i < num_exclusive_groups; i++) {
			for (j = 0; j < exclusive_groups[i].num_clock_names; j++) {
				free(exclusive_groups[i].clock_names[j]);
			}
			free(exclusive_groups[i].clock_names);
		}
		free (exclusive_groups);

		return TRUE;

	} else if (strcmp(ptr, "set_false_path") == 0) {
		/* Syntax: set_false_path -from <clock list or regexes> -to <clock list or regexes> */

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-from") != 0 || (ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_false_path must be directly followed by '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
			exit(1);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_from = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_false_path must be directly followed by '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to from_list until we hit the -to command. */
			from_list = (char **) my_realloc(from_list, ++num_from * sizeof(char *));
			from_list[num_from - 1] = my_strdup(ptr);

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) { 
				/* We hit the end of the line before finding a -to. */
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_false_path requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
				exit(1);
			}
		} while (strcmp(ptr, "-to") != 0);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_false_path requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
				file_line_number);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_to = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_false_path must be directly followed by '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to to_list until we hit the end of the line. */
			to_list = (char **) my_realloc(to_list, ++num_to * sizeof(char *));
			to_list[num_to - 1] = my_strdup(ptr);
		} while ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL);

		/* Create a constraint between each element in from_list and each element in to_list with value DO_NOT_ANALYSE. 
		Set make_copies to false since, as we only need to use from_list and to_list once, we can just have the
		override constraint entry point to those lists. */
		add_override_constraint(from_list, num_from, to_list, num_to, DO_NOT_ANALYSE, 0, domain_level_from, domain_level_to, FALSE);
		
		/* Finally, set from_list and to_list to NULL since they're both 
		being pointed to by the override constraint entry we just created. */
		from_list = NULL, to_list = NULL;

		return TRUE;

	} else if (strcmp(ptr, "set_max_delay") == 0) {
		/* Syntax: set_max_delay <delay> -from <clock list or regexes> -to <clock list or regexes> */

		/* Basically the same as set_false_path above, except we get a specific delay value for the constraint. */

		/* check if the token following set_max_delay is actually a number*/
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Token following set_max_delay should be a delay value, but is not a number.\n", 
					file_line_number);
			exit(1);
		}
		max_delay = (float) strtod(ptr, NULL);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-from") != 0 || (ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_max_delay requires '-from <clock/flip-flop_list>' after max_delay.\n", 
					file_line_number);
			exit(1);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_from = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_max_delay requires '-from <clock/flip-flop_list>' after max_delay.\n", 
						file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to from_list until we hit the -to command. */
			from_list = (char **) my_realloc(from_list, ++num_from * sizeof(char *));
			from_list[num_from - 1] = my_strdup(ptr);

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) { 
				/* We hit the end of the line before finding a -to. */
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_max_delay requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
						file_line_number);
				exit(1);
			}
		} while (strcmp(ptr, "-to") != 0);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_max_delay requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_to = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_max_delay requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
						file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to to_list until we hit the end of the line. */
			to_list = (char **) my_realloc(to_list, ++num_to * sizeof(char *));
			to_list[num_to - 1] = my_strdup(ptr);
		} while ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL);

		/* Create a constraint between each element in from_list and each element in to_list with value max_delay. */
		add_override_constraint(from_list, num_from, to_list, num_to, max_delay, 0, domain_level_from, domain_level_to, FALSE);
		
		/* Finally, set from_list and to_list to NULL since they're both 
		being pointed to by the override constraint entry we just created. */
		from_list = NULL, to_list = NULL;

		return TRUE;

	} else if (strcmp(ptr, "set_multicycle_path") == 0) {
		/* Syntax: set_multicycle_path -setup -from <clock list or regexes> -to <clock list or regexes> <num_multicycles> */

		/* Basically the same as set_false_path and set_max_delay above, except we have to calculate 
		the default value of the constraint (obtained via edge counting) first, and then set a  
		constraint equal to default constraint + (num_multicycles - 1) * period of sink clock domain. */

		/* check if the token following set_max_delay is actually a number*/
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-setup") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path must be directly followed by '-setup'.\n", 
					file_line_number);
			exit(1);
		}

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-from") != 0 || (ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path requires '-from <clock/flip-flop_list>' after '-setup'.\n",
					file_line_number);
			exit(1);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_from = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path '-setup' must be followed by '-from <clock/flip-flop_list>'.\n",
						file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to from_list until we hit the -to command. */
			from_list = (char **) my_realloc(from_list, ++num_from * sizeof(char *));
			from_list[num_from - 1] = my_strdup(ptr);

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) { 
				/* We hit the end of the line before finding a -to. */
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
						file_line_number);
				exit(1);
			}
		} while (strcmp(ptr, "-to") != 0);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
					file_line_number);
		}

		if (strcmp(ptr, "get_clocks") == 0) {
			domain_level_to = TRUE;
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path requires '-to <clock/flip-flop_list>' after '-from <clock/flip-flop_list>'.\n", 
						file_line_number);
				exit(1);
			}
		}

		do {
			/* Keep adding clock names to to_list until we hit a number (i.e. num_multicycles). */
			to_list = (char **) my_realloc(to_list, ++num_to * sizeof(char *));
			to_list[num_to - 1] = my_strdup(ptr);
		} while (((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) != NULL) && !is_number(ptr));

		if (!ptr) {
			/* We hit the end of the line before finding a number. */
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_multicycle_path requires num_multicycles after '-to <clock/flip-flop_list>'.\n", 
					file_line_number);
			exit(1);
		}

		num_multicycles = (int) strtod(ptr, NULL);

		/* Create an override constraint between from and to. Unlike the previous two commands, set_multicycle_path requires 
		information about the periods and offsets of the clock domains which from and to, which we have to fill in at the end. */
		add_override_constraint(from_list, num_from, to_list, num_to, HUGE_NEGATIVE_FLOAT /* irrelevant - never used */,
			num_multicycles, domain_level_from, domain_level_to, FALSE);
		
		/* Finally, set from_list and to_list to NULL since they're both 
		being pointed to by the override constraint entry we just created. */
		from_list = NULL, to_list = NULL;

		return TRUE;

	} else if (strcmp(ptr, "set_input_delay") == 0) {
		/* Syntax: set_input_delay -clock <virtual or netlist clock> -max <max_delay> [get_ports {<I/O port list or regexes>}] */
		
		/* We want to assign virtual_clock to all input ports in port_list, and 
		set the input delay (from the external device to the FPGA) to max_delay. */

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-clock") != 0 || (ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_input_delay must be directly followed by '-clock <virtual or netlist clock name>'.\n", 
					file_line_number);
			exit(1);
		}

		if (num_netlist_clocks == 1 && strcmp(ptr, "*") == 0) {
			/* Allow the user to wildcard the clock name if there's only one clock (not standard SDC but very convenient). */
			clock_name = netlist_clocks[0];
		} else {
			/* We have no way of error-checking whether this is an actual virtual clock until we finish parsing. */
			clock_name = ptr;
		}
	
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-max") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_input_delay '-clock <virtual or netlist clock name>' must be directly followed by '-max <maximum_input_delay>'.\n", 
					file_line_number);
			exit(1);
		}

		/* check if the token following -max is actually a number*/
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Token following '-max' should be a delay value, but is not a number.\n", 
					file_line_number);
			exit(1);
		}
		max_delay = (float) strtod(ptr, NULL);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "get_ports") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_input_delay requires a [get_ports {...}] command following '-max <max_input_delay>'.\n", 
					file_line_number);
			exit(1);
		}

		/* Parse through to the end of the line.  Add each regular expression match we find to the list of 
			constrained inputs and give each entry the virtual clock name and max_delay we've just parsed.  
			We have no way of error-checking whether these tokens correspond to actual input ports until later. */
		
		for (;;) {
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) { /* end of line */
				return TRUE; 
			}

			found = FALSE;

			for (iio = 0; iio < num_netlist_ios; iio++) {
				/* See if the regular expression stored in ptr is legal and matches at least one input port. 
				If it is not legal, it will fail during regex_match.  We check for a match using boolean found. */
				if (regex_match(netlist_ios[iio], ptr)) {
					/* We've found a new input! */
					g_sdc->num_constrained_inputs++;
					found = TRUE;

					/* Fill in input information in the permanent array g_sdc->constrained_inputs. */
					g_sdc->constrained_inputs = (t_io *) my_realloc (g_sdc->constrained_inputs, g_sdc->num_constrained_inputs * sizeof(t_io));
					g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].name = my_strdup(netlist_ios[iio]);
					g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].clock_name = my_strdup(clock_name);
					g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].delay = max_delay;
					g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].file_line_number = file_line_number; /* global var */
				}
			}

			if (!found) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Output name or regular expression \"%s\" does not correspond to any nets.\n", 
						file_line_number, ptr);
				exit(1);
			}
		}

	} else if (strcmp(ptr, "set_output_delay") == 0) {
		/* Syntax: set_output_delay -clock <virtual or netlist clock> -max <max_delay> [get_ports {<I/O port list or regexes>}] */
		
		/* We want to assign virtual_clock to all output ports in port_list, and 
		set the output delay (from the external device to the FPGA) to max_delay. */

			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-clock") != 0 || (ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_output_delay must be directly followed by '-clock <virtual or netlist clock name>'.\n", 
					file_line_number);
			exit(1);
		}

		if (num_netlist_clocks == 1 && strcmp(ptr, "*") == 0) {
			/* Allow the user to wildcard the clock name if there's only one clock (not standard SDC but very convenient). */
			clock_name = netlist_clocks[0];
		} else {
			/* We have no way of error-checking whether this is an actual virtual clock until we finish parsing. */
			clock_name = ptr;
		}
	
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "-max") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_output_delay -clock <virtual or netlist clock name> must be directly followed by '-max <maximum_output_delay>'.\n", 
					file_line_number);
			exit(1);
		}

		/* check if the token following -max is actually a number*/
		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || !is_number(ptr)) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Token following '-max' should be a delay value, but is not a number.\n", 
					file_line_number);
			exit(1);
		}
		max_delay = (float) strtod(ptr, NULL);

		if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL || strcmp(ptr, "get_ports") != 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] set_output_delay requires a [get_ports {...}] command following '-max <max_output_delay>'.\n", 
					file_line_number);
			exit(1);
		}

		/* Parse through to the end of the line.  Add each regular expression match we find to the list of 
			constrained outputs and give each entry the virtual clock name and max_delay we've just parsed.  
			We have no way of error-checking whether these tokens correspond to actual output ports until later. */
		
		for (;;) {
			if ((ptr = my_strtok(NULL, SDC_TOKENS, sdc, buf)) == NULL) { /* end of line */
				return TRUE; 
			}

			found = FALSE;

			for (iio = 0; iio < num_netlist_ios; iio++) {
				/* See if the regular expression stored in ptr is legal and matches at least one output port. 
				If it is not legal, it will fail during regex_match.  We check for a match using boolean found. */
				if (regex_match(netlist_ios[iio], ptr)) {
					/* We've found a new output! */
					g_sdc->num_constrained_outputs++;
					found = TRUE;

					/* Fill in output information in the permanent array g_sdc->constrained_outputs. */
					g_sdc->constrained_outputs = (t_io *) my_realloc (g_sdc->constrained_outputs, g_sdc->num_constrained_outputs * sizeof(t_io));
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].name = my_strdup(netlist_ios[iio]);
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].clock_name = my_strdup(clock_name);
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].delay = max_delay;
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].file_line_number = file_line_number; /* global var */
				}
			}

			if (!found) {
				vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Output name or regular expression \"%s\" does not correspond to any nets.\n", 
						file_line_number, ptr);
				exit(1);
			}
		}

	} else {
		vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Incorrect or unsupported syntax near start of line.\n", 
				file_line_number);
		exit(1);
	}
}

static boolean is_number(char * ptr) {
/* Checks if the character array ptr represents a valid floating-point number.  *
 * To return TRUE, all characters must be digits, although *
 * there can also be no more than one decimal point.       */
	int i, len, num_decimal_points = 0;
	len = strlen(ptr);
	for (i = 0; i < len; i++) {
		if ((ptr[i] < '0' || ptr[i] > '9')) {
			if (ptr[i] != '.') {
				return FALSE;
			}
			num_decimal_points++;
			if (num_decimal_points > 1) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

static int find_constrained_clock(char * ptr) {
/* Given a string ptr, find whether it's the name of a clock in the array g_sdc->constrained_clocks.  *
 * if it is, return the clock's index in g_sdc->constrained_clocks; if it's not, return -1. */
	int index;
	for (index = 0; index < g_sdc->num_constrained_clocks; index++) {
		if (strcmp(ptr, g_sdc->constrained_clocks[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_cc_constraint(char * source_clock_name, char * sink_clock_name) {
	/* Given a pair of source and sink clock domains, find out if there's an override constraint between them.
	If there is, return the index in g_sdc->cc_constraints; if there is not, return -1. */
	int icc, isource, isink;

	for (icc = 0; icc < g_sdc->num_cc_constraints; icc++) {
		for (isource = 0; isource < g_sdc->cc_constraints[icc].num_source; isource++) {
			if (strcmp(g_sdc->cc_constraints[icc].source_list[isource], source_clock_name) == 0) {
				for (isink = 0; isink < g_sdc->cc_constraints[icc].num_sink; isink++) {
					if (strcmp(g_sdc->cc_constraints[icc].sink_list[isink], sink_clock_name) == 0) {
						return icc;
					}
				}
			}
		}
	}
	return -1;
}

static void add_override_constraint(char ** from_list, int num_from, char ** to_list, int num_to, 
	float constraint, int num_multicycles, boolean domain_level_from, boolean domain_level_to,
	boolean make_copies) {
	/* Add a special-case constraint to override the default, calculated timing constraint, 
	to one of four arrays depending on whether it's coming from/to a flip-flop or an entire clock domain. 

	If make_copies is true, we make a copy of from_list and to_list for this override constraint entry; 
	if false, we just set the override constraint entry to point to the existing list. The latter is 
	more efficient, but it's almost impossible to free multiple identical pointers without freeing
	the same thing twice and causing an error. */

	t_override_constraint ** constraint_array; 
	/* Because we are reallocating the array and possibly changing 
	its	address, we need to modify it through a reference. */

	int num_constraints, i;

	if (domain_level_from) {
		if (domain_level_to) { /* Clock-to-clock constraint */
			constraint_array = &g_sdc->cc_constraints;
			num_constraints = ++g_sdc->num_cc_constraints;
		} else { /* Clock-to-flipflop constraint */
			constraint_array = &g_sdc->cf_constraints;
			num_constraints = ++g_sdc->num_cf_constraints;
		}
	} else {
		if (domain_level_to) { /* Flipflop-to-clock constraint */
			constraint_array = &g_sdc->fc_constraints;
			num_constraints = ++g_sdc->num_fc_constraints;
		} else { /* Flipflop-to-flipflop constraint */
			constraint_array = &g_sdc->ff_constraints;
			num_constraints = ++g_sdc->num_ff_constraints;
		}
	}

	*constraint_array = (t_override_constraint *) my_realloc(*constraint_array, num_constraints * sizeof(t_override_constraint));
	
	if (make_copies) {
		/* Copy from_list and to_list to constraint_array[num_constraints - 1].source_list and .sink_list. */
		(*constraint_array)[num_constraints - 1].source_list = (char **) my_malloc(num_from * sizeof(char *));
		(*constraint_array)[num_constraints - 1].sink_list = (char **) my_malloc(num_to * sizeof(char *));
		for (i = 0; i < num_from; i++) {
			(*constraint_array)[num_constraints - 1].source_list[i] = my_strdup(from_list[i]);
		}
		for (i = 0; i < num_to; i++) {
			(*constraint_array)[num_constraints - 1].sink_list[i] = my_strdup(to_list[i]);
		}
	} else {
		/* Just set constraint array to point to from_list and to_list. */
		(*constraint_array)[num_constraints - 1].source_list = from_list;
		(*constraint_array)[num_constraints - 1].sink_list = to_list;
	}
	(*constraint_array)[num_constraints - 1].num_source = num_from;	
	(*constraint_array)[num_constraints - 1].num_sink = num_to;	
	(*constraint_array)[num_constraints - 1].constraint = constraint;
	(*constraint_array)[num_constraints - 1].num_multicycles = num_multicycles;
	(*constraint_array)[num_constraints - 1].file_line_number = file_line_number; /* global var */
}

static float calculate_constraint(t_sdc_clock source_domain, t_sdc_clock sink_domain) {
	/* Given information from the SDC file about the period and offset of two clocks, *
	 * determine the implied setup-time constraint between them via edge counting.    */

	int source_period, sink_period, source_rising_edge, sink_rising_edge, lcm_period, num_source_edges, num_sink_edges, 
		* source_edges, * sink_edges, i, j, time, constraint_as_int;
	float constraint;

	/* If the source and sink domains have the same period and edges, the constraint is just the common clock period. */
	if (fabs(source_domain.period - sink_domain.period) < EPSILON && 
		fabs(source_domain.rising_edge - sink_domain.rising_edge) < EPSILON &&
		fabs(source_domain.falling_edge - sink_domain.falling_edge) < EPSILON) {
		return source_domain.period; /* or, equivalently, sink_domain.period */
	}

	/* If either period is 0, the constraint is 0. */
	if (source_domain.period < EPSILON || sink_domain.period < EPSILON) {
		return 0.;
	}
	
	 /* Multiply periods and edges by 1000 and round down  *				  
	  * to the nearest integer, to avoid messy decimals.   */

	source_period = static_cast<int>(source_domain.period * 1000);
	sink_period = static_cast<int>(sink_domain.period * 1000);
	source_rising_edge = static_cast<int>(source_domain.rising_edge * 1000);
	sink_rising_edge = static_cast<int>(sink_domain.rising_edge * 1000);	

	/* If we get here, we have to use edge counting.  Find the LCM of the two periods.		   *
	* This determines how long it takes before the pattern of the two clocks starts repeating. */
	for (lcm_period = 1; lcm_period % source_period != 0 || lcm_period % sink_period != 0; lcm_period++)
		;

	/* Create an array of positive edges for each clock over one LCM clock period. */

	num_source_edges = lcm_period/source_period + 1; 
	num_sink_edges = lcm_period/sink_period + 1;

	source_edges = (int *) my_malloc((num_source_edges + 1) * sizeof(int));
	sink_edges = (int *) my_malloc((num_sink_edges + 1) * sizeof(int));
	
	for (i = 0, time = source_rising_edge; i < num_source_edges + 1; i++) {
		source_edges[i] = time;
		time += source_period;
	}

	for (i = 0, time = sink_rising_edge; i < num_sink_edges + 1; i++) {
		sink_edges[i] = time;
		time += sink_period;
	}

	/* Compare every edge in source_edges with every edge in sink_edges.			 *
	 * The lowest STRICTLY POSITIVE difference between a sink edge and a source edge *
	 * gives us the set-up time constraint.											 */

	constraint_as_int = INT_MAX; /* constraint starts off at +ve infinity so that everything will be less than it */

	for (i = 0; i < num_source_edges + 1; i++) {
		for (j = 0; j < num_sink_edges + 1; j++) {
			if (sink_edges[j] > source_edges[i]) {
				constraint_as_int = std::min(constraint_as_int, sink_edges[j] - source_edges[i]);
			}
		}
	}

	/* Divide by 1000 again and turn the constraint back into a float, and clean up memory. */

	constraint = constraint_as_int / 1000.;

	free(source_edges);
	free(sink_edges);

	return constraint;
}

static boolean regex_match (char * string, char * regular_expression) {
	/* Given a string and a regular expression, return TRUE if there's a match, 
	FALSE if not. Print an error and exit if regular_expression is invalid. */

	const char * error;
	
	assert(string && regular_expression);

	/* The regex library reports a match if regular_expression is a substring of string
	AND not equal to string. This is not appropriate for our purposes. For example, 
	we'd get both "clock" and "clock2" matching the regular expression "clock".  
	We have to manually return that there's no match in this special case. */
	if (strstr(string, regular_expression) && strcmp(string, regular_expression) != 0)
		return FALSE;

	if (strcmp(regular_expression, "*") == 0)
		return TRUE; /* The regex library hangs if it is fed "*" as a regular expression. */

	error = slre_match((enum slre_option) 0, regular_expression, string, strlen(string));

	if (!error) 
		return TRUE;
	else if (strcmp(error, "No match") == 0) 
		return FALSE;
	else {
		vpr_printf(TIO_MESSAGE_ERROR, "[SDC line %d] Error matching regular expression \"%s\".\n", 
				file_line_number, regular_expression);
		exit(1);
	}
}

void free_sdc_related_structs(void) {
	if (!g_sdc) return;

	free_override_constraint(g_sdc->cc_constraints, g_sdc->num_cc_constraints); 
	/* Should already have been freed in process_constraints() */

	free_override_constraint(g_sdc->cf_constraints, g_sdc->num_cf_constraints);
	free_override_constraint(g_sdc->fc_constraints, g_sdc->num_fc_constraints);
	free_override_constraint(g_sdc->ff_constraints, g_sdc->num_ff_constraints);
	free_io_constraint(g_sdc->constrained_inputs, g_sdc->num_constrained_inputs);
	free_io_constraint(g_sdc->constrained_outputs, g_sdc->num_constrained_outputs);
	free_clock_constraint(g_sdc->constrained_clocks, g_sdc->num_constrained_clocks);
	free_matrix(g_sdc->domain_constraint, 0, g_sdc->num_constrained_clocks - 1, 0, sizeof(float));
	free(g_sdc);
	g_sdc = NULL;
}

void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints) {
	int i, j;

	if (!constraint_array) return;

	for (i = 0; i < num_constraints; i++) {
		for (j = 0; j < constraint_array[i].num_source; j++) {
			free(constraint_array[i].source_list[j]);
			constraint_array[i].source_list[j] = NULL;
		}
		for (j = 0; j < constraint_array[i].num_sink; j++) {
			free(constraint_array[i].sink_list[j]);
			constraint_array[i].sink_list[j] = NULL;
		}
		free(constraint_array[i].source_list);
		free(constraint_array[i].sink_list);
	}
	free(constraint_array);
	constraint_array = NULL;
}

static void free_io_constraint(t_io *& io_array, int num_ios) {
	int i;

	for (i = 0; i < num_ios; i++) {
		free(io_array[i].name);
		free(io_array[i].clock_name);
	}
	free(io_array);
	io_array = NULL;
}

static void free_clock_constraint(t_clock *& clock_array, int num_clocks) {
	int i;

	for (i = 0; i < num_clocks; i++) {
		free(clock_array[i].name);
	}
	free(clock_array);
	clock_array = NULL;
}
