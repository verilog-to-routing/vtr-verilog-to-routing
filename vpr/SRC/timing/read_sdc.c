#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <cstdarg>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "read_sdc.h"
#include "read_blif.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "ReadOptions.h"
#include "slre.h"
#include "sdc.hpp"

#include "TimingConstraints.hpp"

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

static const char* sdc_file_name = "<default_SDC>.sdc"; /* Name of SDC file */

/***************** Subroutines local to this module *************************/

static void alloc_and_load_netlist_clocks_and_ios(void);
static void use_default_timing_constraints(void);
static void count_netlist_clocks_as_constrained_clocks(void);
static void add_clock(std::string net_name);
static int find_constrained_clock(char * ptr);
static float calculate_constraint(t_sdc_clock source_domain, t_sdc_clock sink_domain);
static void add_override_constraint(char ** from_list, int num_from, char ** to_list, int num_to, 
	float constraint, int num_multicycles, bool domain_level_from, bool domain_level_to,
	bool make_copies);
static int find_cc_constraint(char * source_clock_domain, char * sink_clock_domain);
static bool regex_match (const char *string, const char *pattern);
static void count_netlist_ios_as_constrained_ios(char * clock_name, float io_delay);
static void free_io_constraint(t_io *& io_array, int num_ios);
static void free_clock_constraint(t_clock *& clock_array, int num_clocks);

static bool apply_sdc_constraints(std::shared_ptr<const sdcparse::SdcCommands> sdc_commands);
static bool apply_create_clock(const sdcparse::CreateClock& sdc_create_clock);
static bool apply_set_clock_groups(const sdcparse::SetClockGroups& sdc_set_clock_groups);
static bool apply_set_false_path(const sdcparse::SetFalsePath& sdc_set_false_path);
static bool apply_set_max_delay(const sdcparse::SetMaxDelay& sdc_set_max_delay);
static bool apply_set_multicycle_path(const sdcparse::SetMulticyclePath& sdc_set_multicycle_path);
static bool apply_set_io_delay(const sdcparse::SetIoDelay& sdc_set_io_delay);

static bool is_valid_clock_name(const char* clock_name);
static bool build_from_to_lists(char ***from_list, int *num_from, bool* domain_level_from,
                            char ***to_list, int *num_to, bool* domain_level_to,
                            const sdcparse::StringGroup& from_group, const sdcparse::StringGroup& to_group);

void vpr_sdc_error(const int line_number, const std::string& near_text, const std::string& msg);

/********************* Subroutine definitions *******************************/

void read_sdc(t_timing_inf timing_inf) {
	int source_clock_domain, sink_clock_domain, iinput, ioutput, icc, isource, isink;
	
	/* Make sure we haven't called this subroutine before. */
	VTR_ASSERT(!g_sdc);

	/* Allocate container structure for SDC constraints. */
	g_sdc = (t_timing_constraints *) vtr::calloc(1, sizeof(t_timing_constraints));
	

	/* If no SDC file is included or specified, or timing analysis is off,
	use default behaviour of cutting paths between domains and optimizing each clock separately */

	if (!timing_inf.timing_analysis_enabled) {
		vtr::printf_info("\n");
		vtr::printf_info("Timing analysis off; using default timing constraints.\n");
		use_default_timing_constraints();
		return;
	}
	
	if ((sdc = fopen(timing_inf.SDCFile, "r")) == NULL) {
		vtr::printf_info("\n");
		vtr::printf_info("SDC file '%s' blank or not found.\n", timing_inf.SDCFile);
		use_default_timing_constraints();
		return;
	}
	
	/* Now we have an SDC file. */

	/* Save name of SDC file for error outputs */
	sdc_file_name = timing_inf.SDCFile;

	/* Count how many clocks and I/Os are in the netlist. 
	Store the names of each clock and each I/O in netlist_clocks and netlist_ios. 
	The only purpose of these two lists is to compare clock names in the SDC file against them.
	As a result, they will be freed after the SDC file is parsed. */
	alloc_and_load_netlist_clocks_and_ios();

    /* Setup error handling */
    sdcparse::set_sdc_error_handler(vpr_sdc_error);

	/* Parse the file line-by-line. */
    auto sdc_commands = sdcparse::sdc_parse_file(sdc);

	if (!sdc_commands->has_commands()) { /* blank file or only comments found */
		vtr::printf_info("\n");
		vtr::printf_info("SDC file '%s' blank or not found.\n", timing_inf.SDCFile);
		use_default_timing_constraints();
		free(netlist_clocks);
		free(netlist_ios);
		return;
	}

    apply_sdc_constraints(sdc_commands);
    

	/* Make sure that all virtual clocks referenced in g_sdc->constrained_inputs and g_sdc->constrained_outputs have been constrained. */
	for (iinput = 0; iinput < g_sdc->num_constrained_inputs; iinput++) {
		if ((find_constrained_clock(g_sdc->constrained_inputs[iinput].clock_name)) == -1) {
			vpr_throw(VPR_ERROR_SDC, sdc_file_name, g_sdc->constrained_inputs[iinput].file_line_number, 
					"Input %s is associated with an unconstrained clock %s.\n", 
					g_sdc->constrained_inputs[iinput].name, 
					g_sdc->constrained_inputs[iinput].clock_name);
		}
	}

	for (ioutput = 0; ioutput < g_sdc->num_constrained_outputs; ioutput++) {
		if ((find_constrained_clock(g_sdc->constrained_outputs[ioutput].clock_name)) == -1) {
			vpr_throw(VPR_ERROR_SDC, sdc_file_name, g_sdc->constrained_inputs[iinput].file_line_number,
					"Output %s is associated with an unconstrained clock %s.\n", 
					g_sdc->constrained_outputs[ioutput].name, 
					g_sdc->constrained_outputs[ioutput].clock_name);
		}
	}

	/* Make sure that all clocks referenced in g_sdc->cc_constraints have been constrained. */
	for (icc = 0; icc < g_sdc->num_cc_constraints; icc++) {
		for (isource = 0; isource < g_sdc->cc_constraints[icc].num_source; isource++) {
			if ((find_constrained_clock(g_sdc->cc_constraints[icc].source_list[isource])) == -1) {
				vpr_throw(VPR_ERROR_SDC, sdc_file_name, g_sdc->cc_constraints[icc].file_line_number,
						"Token %s is not a constrained clock.\n", 
						g_sdc->cc_constraints[icc].source_list[isource]);
			}
		}
		for (isink = 0; isink < g_sdc->cc_constraints[icc].num_sink; isink++) {
			if ((find_constrained_clock(g_sdc->cc_constraints[icc].sink_list[isink])) == -1) {
				vpr_throw(VPR_ERROR_SDC, sdc_file_name, g_sdc->cc_constraints[icc].file_line_number, 
						"Token %s is not a constrained clock.\n",
						g_sdc->cc_constraints[icc].sink_list[isink]);
			}
		}
	}

	/* Allocate matrix of timing constraints [0..g_sdc->num_constrained_clocks-1][0..g_sdc->num_constrained_clocks-1] and initialize to 0 */
	g_sdc->domain_constraint = vtr::alloc_matrix<float>(0, g_sdc->num_constrained_clocks-1, 0, g_sdc->num_constrained_clocks-1);
	
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

	vtr::printf_info("\n");
	vtr::printf_info("SDC file '%s' parsed successfully.\n",
			 timing_inf.SDCFile ); 
	vtr::printf_info("%d clocks (including virtual clocks), %d inputs and %d outputs were constrained.\n", 
			 g_sdc->num_constrained_clocks, g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
	vtr::printf_info("\n");
	
	/* Since all the information we need is stored in g_sdc->domain_constraint, g_sdc->constrained_clocks, 
	and constrained_ios, free other data structures used in this routine */
	for (int iclk = 0; iclk < g_sdc->num_constrained_clocks; iclk++) {
        free(sdc_clocks[iclk].name);
    }
	free(sdc_clocks);
	free(netlist_clocks);
    for(int i = 0; i < num_netlist_ios; ++i) {
        free(netlist_ios[i]);
    }
	free(netlist_ios);

    fclose(sdc);

	return;
}

/*
 * Override the default error function in libsdcparse so that it throws
 * vpr style errors.
 */
void vpr_sdc_error(const int line_number, const std::string& /*near_text*/, const std::string& msg) {
    vpr_throw(VPR_ERROR_SDC, get_sdc_file_name(), line_number, msg.c_str());
}

static bool apply_sdc_constraints(std::shared_ptr<const sdcparse::SdcCommands> sdc_commands) {
    //create_clock
    for(const auto& create_clock_cmd : sdc_commands->create_clock_cmds) {
        apply_create_clock(create_clock_cmd);
    }

    //set_clock_groups
    for(const auto& set_clock_groups_cmd : sdc_commands->set_clock_groups_cmds) {
        apply_set_clock_groups(set_clock_groups_cmd);
    }

    //set_false_path
    for(const auto& set_false_path_cmd : sdc_commands->set_false_path_cmds) {
        apply_set_false_path(set_false_path_cmd);
    }

    //set_max_delay
    for(const auto& set_max_delay_cmd : sdc_commands->set_max_delay_cmds) {
        apply_set_max_delay(set_max_delay_cmd);
    }

    //set_multicycle_path
    for(const auto& set_multicycle_path_cmd : sdc_commands->set_multicycle_path_cmds) {
        apply_set_multicycle_path(set_multicycle_path_cmd);
    }

    //set_input_delay
    for(const auto& set_input_delay_cmd : sdc_commands->set_input_delay_cmds) {
        apply_set_io_delay(set_input_delay_cmd);
    }

    //set_output_delay
    for(const auto& set_output_delay_cmd : sdc_commands->set_output_delay_cmds) {
        apply_set_io_delay(set_output_delay_cmd);
    }

    return true;
}

static bool apply_create_clock(const sdcparse::CreateClock& sdc_create_clock) {
    bool found;

    if(sdc_create_clock.is_virtual) { 
        /* Store the clock's name, period and edges in the local array sdc_clocks. */
        sdc_clocks = (t_sdc_clock *) vtr::realloc(sdc_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_sdc_clock));
        sdc_clocks[g_sdc->num_constrained_clocks - 1].name = vtr::strdup(sdc_create_clock.name.c_str());
        sdc_clocks[g_sdc->num_constrained_clocks - 1].period = sdc_create_clock.period;
        sdc_clocks[g_sdc->num_constrained_clocks - 1].rising_edge = sdc_create_clock.rise_edge; 
        sdc_clocks[g_sdc->num_constrained_clocks - 1].falling_edge = sdc_create_clock.fall_edge; 

        /* Also store the clock's name, and the fact that it is not a netlist clock, in g_sdc->constrained_clocks. */
        g_sdc->constrained_clocks = (t_clock *) vtr::realloc (g_sdc->constrained_clocks, g_sdc->num_constrained_clocks * sizeof(t_clock));
        g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = vtr::strdup(sdc_create_clock.name.c_str());
        g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = false;
    } else {
        VTR_ASSERT(!sdc_create_clock.is_virtual);

        for(size_t itarget = 0; itarget < sdc_create_clock.targets.strings.size(); itarget++) {

            /* See if the regular expression stored in ptr is legal and matches at least one clock net. 
            If it is not legal, it will fail during regex_match.  We check for a match using bool found. */
            found = false;
            for (int iclock = 0; iclock < num_netlist_clocks; iclock++) {
                if (regex_match(netlist_clocks[iclock], sdc_create_clock.targets.strings[itarget].c_str())) {
                    /* We've found a new clock!  (Note that we can't store ptr as the clock's 
                    name since it could be a regex, unlike the virtual clock case).*/
                    found = true;

                    /* Store the clock's name, period and edges in the local array sdc_clocks. */
                    sdc_clocks = (t_sdc_clock *) vtr::realloc(sdc_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_sdc_clock));
                    sdc_clocks[g_sdc->num_constrained_clocks - 1].name = netlist_clocks[iclock];
                    sdc_clocks[g_sdc->num_constrained_clocks - 1].period = sdc_create_clock.period;
                    sdc_clocks[g_sdc->num_constrained_clocks - 1].rising_edge = sdc_create_clock.rise_edge; 
                    sdc_clocks[g_sdc->num_constrained_clocks - 1].falling_edge = sdc_create_clock.fall_edge; 

                    /* Also store the clock's name, and the fact that it is a netlist clock, in g_sdc->constrained_clocks. */
                    g_sdc->constrained_clocks = (t_clock *) vtr::realloc (g_sdc->constrained_clocks, g_sdc->num_constrained_clocks * sizeof(t_clock));
                    g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = vtr::strdup(netlist_clocks[iclock]);
                    g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = true;
                    /* Fanout will be filled out once the timing graph has been constructed. */
                }
            }

            if (!found) {
                vpr_throw(VPR_ERROR_SDC, sdc_file_name, sdc_create_clock.file_line_number, 
                        "Clock name or regular expression does not correspond to any nets.\n"
                        "If you'd like to create a virtual clock, use the '-name' keyword.\n");
                return false;
            }
        }
        
    }
    return true;
}

static bool apply_set_clock_groups(const sdcparse::SetClockGroups& sdc_set_clock_groups) {
    bool found;

    int iclock;
    int num_exclusive_groups = 0;
	t_sdc_exclusive_group *exclusive_groups = NULL;

    VTR_ASSERT(sdc_set_clock_groups.clock_groups.size() >= 2); //Should have already been caught by parser
    VTR_ASSERT(sdc_set_clock_groups.cg_type == sdcparse::ClockGroupsType::EXCLUSIVE); //Currently only form supported

    for(size_t igroup = 0; igroup < sdc_set_clock_groups.clock_groups.size(); igroup++) {
        const sdcparse::StringGroup& clock_group = sdc_set_clock_groups.clock_groups[igroup];
        
        /* Create a new entry in exclusive groups */
        exclusive_groups = (t_sdc_exclusive_group *) vtr::realloc(
            exclusive_groups, ++num_exclusive_groups * sizeof(t_sdc_exclusive_group));
        exclusive_groups[num_exclusive_groups - 1].clock_names = NULL;
        exclusive_groups[num_exclusive_groups - 1].num_clock_names = 0;

        for(size_t iclk_name = 0; iclk_name < clock_group.strings.size(); iclk_name++) {
            const char* clk_name = clock_group.strings[iclk_name].c_str();

            /* Check the regex clk_name against each netlist clock and add it to the clock_names list if it matches. */
            found = false;
            for (iclock = 0; iclock < num_netlist_clocks; iclock++) {
                if (regex_match(netlist_clocks[iclock], clk_name)) {
                    found = true;
                    exclusive_groups[num_exclusive_groups - 1].clock_names = (char **) vtr::realloc(
                        exclusive_groups[num_exclusive_groups - 1].clock_names, ++exclusive_groups[num_exclusive_groups - 1].num_clock_names * sizeof(char *));
                    exclusive_groups[num_exclusive_groups - 1].clock_names
                        [exclusive_groups[num_exclusive_groups - 1].num_clock_names - 1] = 
                        vtr::strdup(netlist_clocks[iclock]);
                }
            }
            if (!found) {
                if(!is_valid_clock_name(clk_name)) {
                    vpr_throw(VPR_ERROR_SDC, sdc_file_name, sdc_set_clock_groups.file_line_number, 
                            "Clock name '%s' does not match to any known clock.\n",
                            clk_name);
                } else {
                    /* The clock_name is a valid non-netlist clock (i.e. a virtual clock), so add it to the list.  */
                    exclusive_groups[num_exclusive_groups - 1].clock_names = (char **) vtr::realloc(
                        exclusive_groups[num_exclusive_groups - 1].clock_names, ++exclusive_groups[num_exclusive_groups - 1].num_clock_names * sizeof(char *));
                    exclusive_groups[num_exclusive_groups - 1].clock_names[exclusive_groups[num_exclusive_groups - 1].num_clock_names - 1] = vtr::strdup(clk_name);
                }
            }
        }
    }

    /* Finally, create two DO_NOT_ANALYSE override constraints for each pair of entries
    to cut paths bidirectionally between pairs of clock lists in different groups. 
    Set make_copies to true because we have to use the lists of names in multiple
    override constraints, and it's impossible to free them from multiple places at the
    end without a whole lot of trouble. */

    for (int i = 0; i < num_exclusive_groups; i++) {
        for (int j = 0; j < num_exclusive_groups; j++) {
            if (i != j) {
                add_override_constraint(exclusive_groups[i].clock_names, exclusive_groups[i].num_clock_names, 
                    exclusive_groups[j].clock_names, exclusive_groups[j].num_clock_names, DO_NOT_ANALYSE, 0, true, true, true);
            }
        }
    }

    /* Now that we've copied all the clock name lists
    (2 * num_exlusive_groups - 1) times, free the original lists. */
    for (int i = 0; i < num_exclusive_groups; i++) {
        for (int j = 0; j < exclusive_groups[i].num_clock_names; j++) {
            free(exclusive_groups[i].clock_names[j]);
        }
        free(exclusive_groups[i].clock_names);
    }
    free (exclusive_groups);

    return true;
}


static bool apply_set_false_path(const sdcparse::SetFalsePath& sdc_set_false_path) {
	bool domain_level_from = false, domain_level_to = false;
    int num_from = 0, num_to = 0;
    char **from_list = NULL, **to_list = NULL;

    build_from_to_lists(&from_list, &num_from, &domain_level_from,
                        &to_list, &num_to, &domain_level_to,
                        sdc_set_false_path.from, sdc_set_false_path.to);

    /* Create a constraint between each element in from_list and each element in to_list with value DO_NOT_ANALYSE. 
    Set make_copies to false since, as we only need to use from_list and to_list once, we can just have the
    override constraint entry point to those lists. */
    add_override_constraint(from_list, num_from, to_list, num_to, DO_NOT_ANALYSE, 0, domain_level_from, domain_level_to, false);
    
    /* Finally, set from_list and to_list to NULL since they're both 
    being pointed to by the override constraint entry we just created. */
    from_list = NULL, to_list = NULL;

    return true;
}

static bool apply_set_max_delay(const sdcparse::SetMaxDelay& sdc_set_max_delay) {
    /* Basically the same as apply_set_false_path, except we get a specific delay value for the constraint. */

	bool domain_level_from = false, domain_level_to = false;
    int num_from = 0, num_to = 0;
    char **from_list = NULL, **to_list = NULL;

    build_from_to_lists(&from_list, &num_from, &domain_level_from,
                        &to_list, &num_to, &domain_level_to,
                        sdc_set_max_delay.from, sdc_set_max_delay.to);

    /* Create a constraint between each element in from_list and each element in to_list with value max_delay. */
    add_override_constraint(from_list, num_from, to_list, num_to, sdc_set_max_delay.max_delay,
                            0, domain_level_from, domain_level_to, false);
    
    /* Finally, set from_list and to_list to NULL since they're both 
    being pointed to by the override constraint entry we just created. */
    from_list = NULL, to_list = NULL;

    return true;
}

static bool apply_set_multicycle_path(const sdcparse::SetMulticyclePath& sdc_set_multicycle_path) {
	bool domain_level_from = false, domain_level_to = false;
    int num_from = 0, num_to = 0;
    char **from_list = NULL, **to_list = NULL;

    VTR_ASSERT(sdc_set_multicycle_path.mcp_type == sdcparse::McpType::SETUP); //Currently only form supported

    build_from_to_lists(&from_list, &num_from, &domain_level_from,
                        &to_list, &num_to, &domain_level_to,
                        sdc_set_multicycle_path.from, sdc_set_multicycle_path.to);

    /* Create an override constraint between from and to. Unlike the previous two commands, set_multicycle_path requires 
    information about the periods and offsets of the clock domains which from and to, which we have to fill in at the end. */
    add_override_constraint(from_list, num_from, to_list, num_to, HUGE_NEGATIVE_FLOAT /* irrelevant - never used */,
        sdc_set_multicycle_path.mcp_value, domain_level_from, domain_level_to, false);
    
    /* Finally, set from_list and to_list to NULL since they're both 
    being pointed to by the override constraint entry we just created. */
    from_list = NULL, to_list = NULL;

    return true;
}

static bool apply_set_io_delay(const sdcparse::SetIoDelay& sdc_set_io_delay) {
    bool found;
    const char* io_type;

    const char* clock_name = sdc_set_io_delay.clock_name.c_str();

    //Support for non-standard wildcard clock name for set_input_delay/set_output_dealy
    //commands, provided it is unambiguous (i.e. there is only one netlist clock)
    if(num_netlist_clocks == 1 && strcmp(clock_name, "*") == 0) {
        clock_name = netlist_clocks[0]; 
    }

    //Verify that the provided clock name is valid
    if(!is_valid_clock_name(clock_name)) {
        vpr_throw(VPR_ERROR_SDC, sdc_file_name, sdc_set_io_delay.file_line_number, 
                "Clock name '%s' does not match to any known clock.\n",
                clock_name);
    }
    
    /* 
     * Add each regular expression match we find to the list of constrained 
     * inputs (outputs) and give each entry the appropraite clock name and 
     * max_delay 
     */
    
    const sdcparse::StringGroup& port_group = sdc_set_io_delay.target_ports;
    for (size_t iport = 0; iport < port_group.strings.size(); iport++) {

        found = false;

        for (int iio = 0; iio < num_netlist_ios; iio++) {
            /* See if the regular expression  is legal and matches at least one input port. 
            If it is not legal, it will fail during regex_match.  We check for a match using bool found. */
            if (regex_match(netlist_ios[iio], port_group.strings[iport].c_str())) {
                if(sdc_set_io_delay.io_type == sdcparse::IoDelayType::INPUT) {
                    /* We've found a new input! */
                    g_sdc->num_constrained_inputs++;
                    found = true;

                    /* Fill in input information in the permanent array g_sdc->constrained_inputs. */
                    g_sdc->constrained_inputs = (t_io *) vtr::realloc (g_sdc->constrained_inputs, g_sdc->num_constrained_inputs * sizeof(t_io));
                    g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].name = vtr::strdup(netlist_ios[iio]);
                    g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].clock_name = vtr::strdup(clock_name);
                    g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].delay = sdc_set_io_delay.max_delay;
                    g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].file_line_number = sdc_set_io_delay.file_line_number;
                } else {
                    VTR_ASSERT(sdc_set_io_delay.io_type == sdcparse::IoDelayType::OUTPUT);
					/* We've found a new output! */
					g_sdc->num_constrained_outputs++;
					found = true;

					/* Fill in output information in the permanent array g_sdc->constrained_outputs. */
					g_sdc->constrained_outputs = (t_io *) vtr::realloc (g_sdc->constrained_outputs, g_sdc->num_constrained_outputs * sizeof(t_io));
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].name = vtr::strdup(netlist_ios[iio]);
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].clock_name = vtr::strdup(clock_name);
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].delay = sdc_set_io_delay.max_delay;
					g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].file_line_number = sdc_set_io_delay.file_line_number;
                }
            }
        }

        if (!found) {
            if(sdc_set_io_delay.io_type == sdcparse::IoDelayType::INPUT) {
                io_type = "Input";
            } else {
                VTR_ASSERT(sdc_set_io_delay.io_type == sdcparse::IoDelayType::OUTPUT);
                io_type = "Output";
            }

            vpr_throw(VPR_ERROR_SDC, sdc_file_name, sdc_set_io_delay.file_line_number, 
                    "%s name or regular expression \"%s\" does not correspond to any nets.\n", io_type, port_group.strings[iport].c_str());
            return false;
        }
    }
    return true;
}

static bool is_valid_clock_name(const char* clock_name) {
    bool found = false;
    for(int iclk = 0; iclk < g_sdc->num_constrained_clocks; iclk++) {
        if(strcmp(g_sdc->constrained_clocks[iclk].name, clock_name) == 0) {
            found = true;
            break;
        }
    }
    return found;
}

static bool build_from_to_lists(char ***from_list, int *num_from, bool* domain_level_from,
                            char ***to_list, int *num_to, bool* domain_level_to,
                            const sdcparse::StringGroup& from_group, const sdcparse::StringGroup& to_group) {
    /*
     * Source 'from' objects/clocks
     */

    //Are we working with a set of clock domains?
    if(from_group.group_type == sdcparse::StringGroupType::CLOCK) {
        *domain_level_from = true;
    }
    for(size_t i = 0; i < from_group.strings.size(); i++) {
        /* Keep adding clock names to from_list */
        (*from_list) = (char **) vtr::realloc((*from_list), ++(*num_from) * sizeof(**from_list));
        (*from_list)[(*num_from) - 1] = vtr::strdup(from_group.strings[i].c_str());
    }

    /*
     * Target 'to' object/clocsk
     */
    
    //Are we working with a set of clock domains?

    if(to_group.group_type == sdcparse::StringGroupType::CLOCK) {
        *domain_level_to = true;
    }
    for(size_t i = 0; i < to_group.strings.size(); i++) {
        /* Keep adding clock names to to_list */
        (*to_list) = (char **) vtr::realloc((*to_list), ++(*num_to) * sizeof(**to_list));
        (*to_list)[(*num_to) - 1] = vtr::strdup(to_group.strings[i].c_str());
    }

    return true;
}

static void use_default_timing_constraints(void) {

	int source_clock_domain, sink_clock_domain;
	
	/* Find all netlist clocks and add them as constrained clocks. */
	count_netlist_clocks_as_constrained_clocks();

	/* We'll use separate defaults for multi-clock and single-clock/combinational circuits. */

	if (g_sdc->num_constrained_clocks <= 1) {
		/* Create one constrained clock with period 0... */
		g_sdc->domain_constraint = vtr::alloc_matrix<float>(0, 0, 0, 0);
		g_sdc->domain_constraint[0][0] = 0.;
		
        vtr::printf_info("\n");

		if (g_sdc->num_constrained_clocks == 0) {
			/* We need to create a virtual clock to constrain I/Os on. */
			g_sdc->num_constrained_clocks = 1;
			g_sdc->constrained_clocks = (t_clock *) vtr::malloc(sizeof(t_clock));
			g_sdc->constrained_clocks[0].name = vtr::strdup("virtual_io_clock");
			g_sdc->constrained_clocks[0].is_netlist_clock = false;

            /* Constrain all I/Os on the virtual clock, with I/O delay 0. */
            count_netlist_ios_as_constrained_ios(g_sdc->constrained_clocks[0].name, 0.);

			vtr::printf_info("Defaulting to: constrain all %d inputs and %d outputs on a virtual external clock.\n", 
					g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
			vtr::printf_info("Optimize this virtual clock to run as fast as possible.\n");
		} else {
            /* Constrain all I/Os on the single netlist clock, with I/O delay 0. */
            count_netlist_ios_as_constrained_ios(g_sdc->constrained_clocks[0].name, 0.);

			vtr::printf_info("Defaulting to: constrain all %d inputs and %d outputs on the netlist clock.\n", 
					g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
			vtr::printf_info("Optimize this clock to run as fast as possible.\n");
		}
	} else { /* Multiclock circuit */

		/* Constrain all I/Os on a separate virtual clock. Cut paths between all netlist
		 clocks, but analyse all paths between the virtual I/O clock and netlist clocks
		 and optimize all clocks to go as fast as possible. */

		g_sdc->constrained_clocks = (t_clock *) vtr::realloc (g_sdc->constrained_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_clock));
		g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = vtr::strdup("virtual_io_clock");
		g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = false;
		count_netlist_ios_as_constrained_ios(g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name, 0.);

		/* Allocate matrix of timing constraints [0..g_sdc->num_constrained_clocks-1][0..g_sdc->num_constrained_clocks-1] */
		g_sdc->domain_constraint = vtr::alloc_matrix<float>(0, g_sdc->num_constrained_clocks-1, 0, g_sdc->num_constrained_clocks-1);

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
		
		vtr::printf_info("\n");
		vtr::printf_info("Defaulting to: constrain all %d inputs and %d outputs on a virtual external clock;\n",
				g_sdc->num_constrained_inputs, g_sdc->num_constrained_outputs);
		vtr::printf_info("\tcut paths between netlist clock domains; and\n");
		vtr::printf_info("\toptimize all clocks to run as fast as possible.\n");
	}
}

static void alloc_and_load_netlist_clocks_and_ios(void) {

	/* Count how many clocks and I/Os are in the netlist. 
	Store the names of each clock and each I/O in netlist_clocks and netlist_ios. */

    for(auto blk_id : g_atom_nl.blocks()) {

        AtomBlockType type = g_atom_nl.block_type(blk_id);
        if(type == AtomBlockType::SEQUENTIAL) {
            for(auto pin_id : g_atom_nl.block_clock_pins(blk_id)) {
                AtomNetId clk_net_id = g_atom_nl.pin_net(pin_id);
                VTR_ASSERT(clk_net_id);

                std::string name = g_atom_nl.net_name(clk_net_id);
                /* Now that we've found a clock, let's see if we've counted it already */
                bool found = false;
                for (int i = 0; !found && i < num_netlist_clocks; i++) {
                    if (netlist_clocks[i] == name) {
                        found = true;
                    }
                }
                if (!found) {
                    /* If we get here, the clock is new and so we dynamically grow the array netlist_clocks by one. */
                    netlist_clocks = (char **) vtr::realloc (netlist_clocks, ++num_netlist_clocks * sizeof(char *));
                    netlist_clocks[num_netlist_clocks - 1] = vtr::strdup(name.c_str());
                }
            }
		} else if (type == AtomBlockType::INPAD || type == AtomBlockType::OUTPAD) {

            std::string name = g_atom_nl.block_name(blk_id);
			/* Now that we've found an I/O, let's see if we've counted it already */
			bool found = false;
			for (int i = 0; !found && i < num_netlist_ios; i++) {
				if (netlist_ios[i] == name) {
					found = true;
				}
			}
			if (!found) {
                const char* trimmed_name = (type == AtomBlockType::OUTPAD) ? name.c_str() + 4 : name.c_str();
				/* the + 4 removes the prefix "out:" automatically prepended to outputs */

				/* If we get here, the I/O is new and so we dynamically grow the array netlist_ios by one. */
				netlist_ios = (char **) vtr::realloc (netlist_ios, ++num_netlist_ios * sizeof(char *));
				netlist_ios[num_netlist_ios - 1] = vtr::strdup(trimmed_name); 
			}
		}
	}
}

static void count_netlist_clocks_as_constrained_clocks(void) {
	/* Counts how many clocks are in the netlist, and adds them to the array g_sdc->constrained_clocks. */

	g_sdc->num_constrained_clocks = 0;
	
    for(auto blk_id : g_atom_nl.blocks()) {
        //Check for input clocks
        for(auto pin_id : g_atom_nl.block_clock_pins(blk_id)) {
            AtomNetId clk_net_id = g_atom_nl.pin_net(pin_id);
            VTR_ASSERT(clk_net_id);

            std::string name = g_atom_nl.net_name(clk_net_id);
            add_clock(name);
        }

        //Check for generated clocks
        for(AtomPortId port : g_atom_nl.block_output_ports(blk_id)) {
            const t_model_ports* port_model = g_atom_nl.port_model(port);
            VTR_ASSERT(port_model->dir == OUT_PORT);

            if(port_model->is_clock) {
                //This is a clock generator
                for(AtomPinId pin : g_atom_nl.port_pins(port)) {
                    AtomNetId net = g_atom_nl.pin_net(pin);
                    std::string net_name = g_atom_nl.net_name(net);
                    add_clock(net_name);
                }
            }
        }
	}
}

static void add_clock(std::string net_name) {
    /* Now that we've found a clock, let's see if we've counted it already */
    bool found = false;
    for (int i = 0; !found && i < g_sdc->num_constrained_clocks; i++) {
        if (g_sdc->constrained_clocks[i].name == net_name) {
            found = true;
        }
    }
    if (!found) {
        /* If we get here, the clock is new and so we dynamically grow the array g_sdc->constrained_clocks by one. */
        g_sdc->constrained_clocks = (t_clock *) vtr::realloc (g_sdc->constrained_clocks, ++g_sdc->num_constrained_clocks * sizeof(t_clock));
        g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].name = vtr::strdup(net_name.c_str());
        g_sdc->constrained_clocks[g_sdc->num_constrained_clocks - 1].is_netlist_clock = true;
        /* Fanout will be filled out once the timing graph has been constructed. */
    }
}

static void count_netlist_ios_as_constrained_ios(char * clock_name, float io_delay) {
	/* Count how many I/Os are in the netlist, adds them to the arrays g_sdc->constrained_inputs/
	g_sdc->constrained_outputs with an I/O delay of 0 and constrains them to clock clock_name. */

    for(auto blk_id : g_atom_nl.blocks()) {
        AtomBlockType type = g_atom_nl.block_type(blk_id);

		if (type == AtomBlockType::INPAD) {
            std::string name = g_atom_nl.block_name(blk_id);
			/* Now that we've found an I/O, let's see if we've counted it already */
			bool found = false;
			for (int iinput = 0; !found && iinput < g_sdc->num_constrained_inputs; iinput++) {
				if (g_sdc->constrained_inputs[iinput].name == name) {
					found = true;
				}
			}
			if (!found) {
				/* If we get here, the input is new and so we add it to g_sdc->constrained_inputs. */
				g_sdc->constrained_inputs = (t_io *) vtr::realloc (g_sdc->constrained_inputs, ++g_sdc->num_constrained_inputs * sizeof(t_io));
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].name = vtr::strdup(name.c_str()); 
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].clock_name = vtr::strdup(clock_name);
				g_sdc->constrained_inputs[g_sdc->num_constrained_inputs - 1].delay = io_delay;
			}
		} else if (type == AtomBlockType::OUTPAD) {
            std::string name = g_atom_nl.block_name(blk_id);
			/* Now that we've found an I/O, let's see if we've counted it already */
			bool found = false;
			for (int ioutput = 0; !found && ioutput < g_sdc->num_constrained_outputs; ioutput++) {
				if (g_sdc->constrained_outputs[ioutput].name == name) {
					found = true;
				}
			}
			if (!found) {
				/* If we get here, the output is new and so we add it to g_sdc->constrained_outputs. */
				g_sdc->constrained_outputs = (t_io *) vtr::realloc (g_sdc->constrained_outputs, ++g_sdc->num_constrained_outputs * sizeof(t_io));
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].name = vtr::strdup(name.c_str() + 4); 
				/* the + 4 removes the prefix "out:" automatically prepended to outputs */
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].clock_name = vtr::strdup(clock_name);
				g_sdc->constrained_outputs[g_sdc->num_constrained_outputs - 1].delay = io_delay;
			}
		}
	}
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
	float constraint, int num_multicycles, bool domain_level_from, bool domain_level_to,
	bool make_copies) {
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

	*constraint_array = (t_override_constraint *) vtr::realloc(*constraint_array, num_constraints * sizeof(t_override_constraint));
	
	if (make_copies) {
		/* Copy from_list and to_list to constraint_array[num_constraints - 1].source_list and .sink_list. */
		(*constraint_array)[num_constraints - 1].source_list = (char **) vtr::malloc(num_from * sizeof(char *));
		(*constraint_array)[num_constraints - 1].sink_list = (char **) vtr::malloc(num_to * sizeof(char *));
		for (i = 0; i < num_from; i++) {
			(*constraint_array)[num_constraints - 1].source_list[i] = vtr::strdup(from_list[i]);
		}
		for (i = 0; i < num_to; i++) {
			(*constraint_array)[num_constraints - 1].sink_list[i] = vtr::strdup(to_list[i]);
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
	(*constraint_array)[num_constraints - 1].file_line_number = vtr::get_file_line_number_of_last_opened_file(); /* global var */
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

	source_edges = (int *) vtr::malloc((num_source_edges + 1) * sizeof(int));
	sink_edges = (int *) vtr::malloc((num_sink_edges + 1) * sizeof(int));
	
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
				constraint_as_int = min(constraint_as_int, sink_edges[j] - source_edges[i]);
			}
		}
	}

	/* Divide by 1000 again and turn the constraint back into a float, and clean up memory. */

	constraint = constraint_as_int / 1000.;

	free(source_edges);
	free(sink_edges);

	return constraint;
}

static bool regex_match (const char * string, const char * regular_expression) {
	/* Given a string and a regular expression, return true if there's a match, 
	false if not. Print an error and exit if regular_expression is invalid. */

	const char * error;
	
	VTR_ASSERT(string && regular_expression);

	/* The regex library reports a match if regular_expression is a substring of string
	AND not equal to string. This is not appropriate for our purposes. For example, 
	we'd get both "clock" and "clock2" matching the regular expression "clock".  
	We have to manually return that there's no match in this special case. */
	if (strstr(string, regular_expression) && strcmp(string, regular_expression) != 0)
		return false;

	if (strcmp(regular_expression, "*") == 0)
		return true; /* The regex library hangs if it is fed "*" as a regular expression. */

	error = slre_match((enum slre_option) 0, regular_expression, string, strlen(string));

	if (!error) 
		return true;
	else if (strcmp(error, "No match") == 0) 
		return false;
	else {
		vpr_throw(VPR_ERROR_SDC, sdc_file_name, vtr::get_file_line_number_of_last_opened_file(), 
				"Error matching regular expression \"%s\".\n", regular_expression);
		return false;
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
    vtr::free_matrix(g_sdc->domain_constraint, 0, g_sdc->num_constrained_clocks - 1, 0);
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

const char * get_sdc_file_name(){
	return sdc_file_name;
}

std::unique_ptr<tatum::TimingConstraints> create_timing_constraints(const AtomNetlist& netlist, const AtomMap& atom_map, t_timing_inf timing_inf) {
    if(!g_sdc) {
        read_sdc(timing_inf);
    }

    std::unique_ptr<tatum::TimingConstraints> tc(new tatum::TimingConstraints());

    //Initialize the clocks
    for(int iclk = 0; iclk < g_sdc->num_constrained_clocks; ++iclk) {
        t_clock* clk = &g_sdc->constrained_clocks[iclk];

        //Make the clock
        tatum::DomainId domain = tc->create_clock_domain(clk->name);

        //Find the clock source
        AtomNetId clk_net = netlist.find_net(clk->name);

        if(clk_net) {
            //Real netlist clock
            AtomPinId driver_pin = netlist.net_driver(clk_net);

            tatum::NodeId src_node = atom_map.pin_tnode(driver_pin);

            tc->set_clock_domain_source(src_node, domain);
        } else {
            //Virtual clock - no driver
        }
    }

    //Initialize the clock-to-clock constraints
    for(int isrc_clk = 0; isrc_clk < g_sdc->num_constrained_clocks; ++isrc_clk) {
        tatum::DomainId src_domain = tc->find_clock_domain(g_sdc->constrained_clocks[isrc_clk].name);
        VTR_ASSERT(src_domain);
        for(int isnk_clk = 0; isnk_clk < g_sdc->num_constrained_clocks; ++isnk_clk) {
            tatum::DomainId snk_domain = tc->find_clock_domain(g_sdc->constrained_clocks[isnk_clk].name);
            VTR_ASSERT(snk_domain);

            float constraint = g_sdc->domain_constraint[isrc_clk][isnk_clk];
            if(constraint != DO_NOT_ANALYSE) {
                tc->set_setup_constraint(src_domain, snk_domain, 1e-9*constraint);
            }
        }
    }

    //Initialize the input constraints
    for(int iinput = 0; iinput < g_sdc->num_constrained_inputs; ++iinput) {
        t_io* io_constraint = &g_sdc->constrained_inputs[iinput];

        tatum::DomainId domain = tc->find_clock_domain(io_constraint->clock_name);
        VTR_ASSERT(domain);

        AtomBlockId blk = netlist.find_block(io_constraint->name);
        if(!blk) {
            vtr::printf_warning(__FILE__, __LINE__, "Ignoring input delay timing constraint (primary input '%s' was not found)\n", io_constraint->name);
            continue;
        }

        if(netlist.block_type(blk) != AtomBlockType::INPAD) {
            vtr::printf_warning(__FILE__, __LINE__, "Ignoring input delay timing constraint ('%s' is not a primary input)\n", io_constraint->name);
            continue;
        }

        auto blk_pins = netlist.block_pins(blk);
        if(blk_pins.size() == 0) {
            vtr::printf_warning(__FILE__, __LINE__, "Input delay timing constraint has no effect (primary input '%s' has no fanout)\n", io_constraint->name);
            continue;
        }

        VTR_ASSERT(blk_pins.size() == 1);
        AtomPinId pin = *blk_pins.begin();

        tatum::NodeId node = atom_map.pin_tnode(pin);
 
        //Apply the constraint
        tc->set_input_constraint(node, domain, 1e-9*io_constraint->delay);
    }

    //Initialize the output constraints
    for(int ioutput = 0; ioutput < g_sdc->num_constrained_outputs; ++ioutput) {
        t_io* io_constraint = &g_sdc->constrained_outputs[ioutput];

        tatum::DomainId domain = tc->find_clock_domain(io_constraint->clock_name);
        VTR_ASSERT(domain);

        //VPR prefixes outputs with "out:"
        std::string blk_name = std::string("out:") + io_constraint->name;
        AtomBlockId blk = netlist.find_block(blk_name);
        if(!blk) {
            vtr::printf_warning(__FILE__, __LINE__, "Ignoring output delay timing constraint (primary output '%s' was not found)\n", io_constraint->name);
            continue;
        }

        if(netlist.block_type(blk) != AtomBlockType::OUTPAD) {
            vtr::printf_warning(__FILE__, __LINE__, "Ignoring output delay timing constraint ('%s' is not a primary output)\n", io_constraint->name);
            continue;
        }

        auto blk_pins = netlist.block_pins(blk);
        if(blk_pins.size() == 0) {
            vtr::printf_warning(__FILE__, __LINE__, "Output delay timing constraint has no effect (primary output '%s' has no fanout)\n", io_constraint->name);
            continue;
        }

        VTR_ASSERT(blk_pins.size() == 1);
        AtomPinId pin = *blk_pins.begin();

        tatum::NodeId node = atom_map.pin_tnode(pin);

        tc->set_output_constraint(node, domain, 1e-9*io_constraint->delay);
    }

    //TODO: FF-FF constraint overrides (needs support in Tatum)

    //Mark constant generators
    for(AtomBlockId blk : netlist.blocks()) {
        for(AtomPinId pin : netlist.block_output_pins(blk)) {
            if(netlist.pin_is_constant(pin)) {
                tatum::NodeId node = atom_map.pin_tnode(pin);
                tc->set_constant_generator(node);
            }
        }
    }

    return tc;
}

